/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <event.h>

#include <bit>
#include <integers.h>
#include <stddef.h>
#include <stdint.h>
#include <scheduler.h>
#include <vector>

void EventManager::notify(const Event& event) {
    if (event.Type >= EventType::COUNT) return;
    for (auto pid : Listeners[event.Type]) {
        Process* process = Scheduler::process(pid);
        if (!process) continue;
        // For each event queue in the process, check if it's filter has this
        // event type enabled.
        // If it does, we push this event to the event queue.
        // If it doesn't, we move on.
        // If none of the event queue's had this filter, then that means the
        // book-keeping went wrong and we should remove this process from this
        // Listeners[event.Type] vector.

        bool found = false;
        for (auto queue : process->EventQueues) {
            if (!queue.listens(event.Type)) continue;
            if (event.Type == EventType::READY_TO_READ || event.Type == EventType::READY_TO_WRITE) {
                auto* event_data = std::bit_cast<EventData_ReadyToReadWrite*>(&event.Data);
                event_data->ProcessFD = process->sysfd_to_procfd(event_data->SystemFD);
            }
            queue.push(event);
            found = true;
        }
        if (!found) unregister_listener(event.Type, pid);
    }
}
