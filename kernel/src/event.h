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

#ifndef LENSOR_OS_EVENT_H
#define LENSOR_OS_EVENT_H

#include <functional>
#include <integers.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <vfs_forward.h>
#include <extensions>
#include <unordered_map>

typedef u64 pid_t;

/// NOTE: Each one of these (except invalid) should have a struct
/// defined that the "data" field of the event can be cast to.
enum struct EventType : u32 {
    INVALID,
    // For sockets/pipes
    READY_TO_READ,
    READY_TO_WRITE,
    COUNT
};

// Allow event type enum to be used as the key for a map.
namespace std {
template<> struct hash<EventType> {
    using argument_type = EventType;
    using result_type = size_t;
    result_type operator() (argument_type __key) const noexcept {
        return result_type(__key);
    }
};
}

#define EVENT_MAX_SIZE 128
struct Event {
    EventType Type;
    u8 Data[EVENT_MAX_SIZE];
};

/// Both READY_TO_READ and READY_TO_WRITE events have this data sent with them.
struct EventData_ReadyToReadWrite {
    size_t BytesAvailable;
    SysFD SystemFD;
    // Assigned by notify, as it has access to the process being notified
    // and it's file table.
    ProcFD ProcessFD;
};

template <size_t N>
struct EventQueue {
    // For now, used as a handle to find this particular event queue
    // within a process. In the future, we shouldn't need this, and
    // the handle should just be an index into some data structure,
    // or something.
    usz ID { 0 };
    pid_t PID { pid_t(-1) };
    std::ring_buffer<Event, N> Events;
    std::vector<bool> Filter { (size_t)EventType::COUNT };

    EventQueue() {
        memset(Filter.data(), 0, (size_t)EventType::COUNT);
    }

    void register_listening(EventType e) {
        Filter[(size_t)e] = true;
        // TODO: Add PID to kernel event queue for this event type
        //gEvents.register_listener(e, PID);
    }

    void unregister_listening(EventType e) {
        Filter[(size_t)e] = false;
        // TODO: Remove PID from kernel event queue for this event type
        //gEvents.unregister_listener(e, PID);
    }

    bool listens(EventType e) {
        if (e >= EventType::COUNT) return false;
        return Filter[(size_t)e];
    }

    void push(const Event& e) {
        Events.push_back(e);
    }

    Event pop() {
        return Events.pop_front();
    }
};

struct EventManager {
    /// A map of event_type -> vector of pids of processes that have
    /// event queues that are listening to this event type.
    /// FIXME: Maybe it's a better idea to keep a reference directly to
    /// the event queue that is listening, not just the processes. This
    /// would mean that A. we don't have to look up the process every
    /// time from PID and B. each process's event queue's wouldn't have
    /// to be iterated to find the one listening for the fired event.
    /// Either way, it should be a set and not a vector.
    std::unordered_map<EventType, std::vector<pid_t>> Listeners;

    void register_listener(EventType event_type, pid_t new_listener) {
        if (event_type >= EventType::COUNT) return;
        Listeners[event_type].push_back(new_listener);
    }

    bool unregister_listener(EventType event_type, pid_t new_listener) {
        if (event_type >= EventType::COUNT) return false;
        return std::erase(Listeners[event_type], new_listener);
    }

    void notify(const Event& event);
};

#endif /* LENSOR_OS_EVENT_H */
