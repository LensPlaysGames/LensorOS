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

#include <algorithm>
#include <functional>
#include <integers.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <vfs_forward.h>
#include <extensions>
#include <unordered_map>


// WARNING: Changes to data structures in this file likely also need
// reflected in `user/libc/sys/syscalls.h`.


typedef u64 pid_t;

/// NOTE: Each one of these (except invalid) should have a struct
/// defined that the "data" field of the event can be cast to.
enum struct EventType : u32 {
    INVALID,
    // For server-type listening sockets: connections waiting to be accepted.
    // For sockets/pipes: data is available to read.
    READY_TO_READ,
    // For sockets/pipes: space is available in the FIFO to write to.
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

struct Event;
struct Process;
struct EventManager {
    /// A map of event_type -> vector of pids of processes that have
    /// event queues that are listening to this event type.
    // FIXME: Maybe it's a better idea to keep a reference directly to
    // the event queue that is listening, not just the processes. This
    // would mean that A. we don't have to look up the process every
    // time from PID and B. each process's event queue's wouldn't have
    // to be iterated to find the one listening for the fired event.
    // FIXME: The list of pids should be a set, not a vector.
    std::unordered_map<EventType, std::vector<pid_t>> Listeners;

    void register_listener(EventType event_type, pid_t new_listener) {
        if (event_type >= EventType::COUNT) return;
        if (std::find(Listeners[event_type].begin(), Listeners[event_type].end(), new_listener) != Listeners[event_type].end())
            Listeners[event_type].push_back(new_listener);
    }

    bool unregister_listener(EventType event_type, pid_t new_listener) {
        if (event_type >= EventType::COUNT) return false;
        return std::erase(Listeners[event_type], new_listener);
    }

    void notify(const Event& event);

    void notify(const Event& event, Process* process);
    void notify(const Event& event, pid_t pid);
};

extern EventManager gEvents;

union EventFilter {
    // NOTE: THE FIRST NAMED MEMBER MUST BE THE LARGEST!!

    // Used by READY_TO_READ and READY_TO_WRITE event types.
    ProcFD ProcessFD { ProcFD::Invalid };
    /*
    struct PIDFD_T {
        pid_t PID;
        ProcFD FD;
    } PIDFD;
    */

    bool operator== (const EventFilter& other) const {
        return memcmp(this, &other, sizeof(EventFilter)) == 0;
    }
};

#define EVENT_MAX_SIZE 128
struct Event {
    EventType Type = EventType::INVALID;
    EventFilter Filter = {};
    u8 Data[EVENT_MAX_SIZE] = { 0 };
};

/// Both READY_TO_READ and READY_TO_WRITE events have this data sent with them.
struct EventData_ReadyToReadWrite {
    size_t BytesAvailable;
};

enum struct EventQueueHandle : int { Invalid = static_cast<int>(-1) };

template <size_t N>
struct EventQueue {
    // For now, used as a handle to find this particular event queue
    // within a process. In the future, we shouldn't need this, and
    // the handle should just be an index into some data structure,
    // or something.
    EventQueueHandle ID { EventQueueHandle::Invalid };
    pid_t PID { pid_t(-1) };
    std::ring_buffer<Event, N> Events;
    // Yes, this is an array of vectors. The array index is the event
    // type as a size_t. This provides constant O(1) lookup on event
    // type. Then, we have a vector to store all of the filters that
    // are being listened to of that event type.
    std::vector<EventFilter> Filter[(size_t)EventType::COUNT];

    EventQueue() {
        memset(Filter, 0, sizeof(Filter));
    }

    void register_listening(EventType e, EventFilter efilt) {
        if (e >= EventType::COUNT) return;
        Filter[(size_t)e].push_back(efilt);
        // Add PID to kernel event queue for this event type
        gEvents.register_listener(e, PID);
    }

    void unregister_listening(EventType e, EventFilter efilt) {
        if (e >= EventType::COUNT) return;
        std::erase(Filter[(size_t)e], efilt);
        // Remove PID from kernel event queue for this event type if there are
        // no filters left.
        if (!Filter[(size_t)e].size())
            gEvents.unregister_listener(e, PID);
    }

    bool listens(EventType e, EventFilter efilt) const {
        if (e >= EventType::COUNT) return false;
        return Filter[(size_t)e].size() != 0 && std::find(Filter[(size_t)e].begin(), Filter[(size_t)e].end(), efilt) != Filter[(size_t)e].end();
    }

    void push(const Event& e) {
        Events.push_back(e);
    }

    Event pop() {
        if (Events.size()) return Events.pop_front();
        return { EventType::INVALID, {}, { 0 } };
    }

    bool has_events() {
        return Events.size();
    }
};

#endif /* LENSOR_OS_EVENT_H */
