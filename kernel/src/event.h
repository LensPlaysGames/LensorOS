#ifndef LENSOR_OS_EVENT_H
#define LENSOR_OS_EVENT_H

#include <functional>
#include <integers.h>
#include <stddef.h>
#include <stdint.h>
#include <scheduler.h>
#include <vector>

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
    ProcFD FD;
    size_t BytesAvailable;
};

struct EventManager {
    /// A map of event_type -> vector of pids of processes that have
    /// event queues that are listening to this event type.
    /// FIXME: Maybe it's a better idea to keep a reference directly to
    /// the event queue that is listening, not just the processes. This
    /// would mean that A. we don't have to look up the process every
    /// time from PID and B. each process's event queue's wouldn't have
    /// to be iterated to find the one listening for the fired event.
    std::unordered_map<EventType, std::vector<pid_t>> Listeners;

    void register_listener(EventType event_type, pid_t new_listener) {
        Listeners[event_type].push_back(new_listener);
    }

    bool unregister_listener(EventType event_type, pid_t new_listener) {
        return std::erase(Listeners[event_type], new_listener);
    }

    void notify(Event event) {
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

            //bool found = false;
            //for (auto queue : process->EventQueues) {
            //    if (!queue.listens(event.Type)) continue;
            //    queue.push(event);
            //    found = true;
            //}
            //if (!found) unregister_listener(event.Type, pid);
        }
    }
};

#endif /* LENSOR_OS_EVENT_H */
