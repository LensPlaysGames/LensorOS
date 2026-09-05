#ifndef LENSOROS_DEFINES_KQUEUE_EVENTS_H
#define LENSOROS_DEFINES_KQUEUE_EVENTS_H

#include <lensor/files.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EVENT_DATA_SIZE 128

typedef uint32_t EventFlags;

#ifndef __cplusplus
typedef enum EventType {
    EVENTTYPE_INVALID,
    // For server-type listening sockets: connections waiting to be accepted.
    // For sockets/pipes: data is available to read.
    EVENTTYPE_READY_TO_READ,
    // For sockets/pipes: space is available in the FIFO to write to.
    EVENTTYPE_READY_TO_WRITE,
    EVENTTYPE_KEYBOARD,
    EVENTTYPE_MOUSE,
    EVENTTYPE_COUNT
} EventType;
typedef union EventFilter {
    ProcFD ProcessFD;
} EventFilter;
typedef struct Event {
    EventType Type;
    EventFilter Filter;
    EventFlags Flags;
    uint8_t Data[EVENT_DATA_SIZE];
} Event;
#else
// ================================================================
// C++ Interface
// ================================================================

/// NOTE: Each one of these (except invalid) should have a struct
/// defined that the "data" field of the event can be cast to.
enum struct EventType : u32 {
    INVALID,
    // TODO: Reduce this to just "FILE_READY" and have ready for read/write be
    // a flag.
    // For server-type listening sockets: connections waiting to be accepted.
    // For sockets/pipes: data is available to read.
    READY_TO_READ,
    // For sockets/pipes: space is available in the FIFO to write to.
    READY_TO_WRITE,

    // Human Input
    KEYBOARD,
    MOUSE,  // NOTE: touch, joystick, etc

    COUNT
};
union EventFilter {
    // NOTE: THE FIRST NAMED MEMBER MUST BE THE LARGEST!!

    // Used by READY_TO_READ and READY_TO_WRITE event types.
    ProcFD ProcessFD{ProcFD::Invalid};
    /*
    struct PIDFD_T {
        pid_t PID;
        ProcFD FD;
    } PIDFD;
    */

    bool operator==(const EventFilter& other) const {
        return memcmp(this, &other, sizeof(EventFilter)) == 0;
    }
};
struct Event {
    // An event type signifies what has happened, categorically.
    EventType Type = EventType::INVALID;
    // The filter narrows down the subject that the event is happening to.
    // For READY_TO_READ, this would be a file descriptor.
    EventFilter Filter = {};
    EventFlags Flags = {};
    u8 Data[EVENT_DATA_SIZE] = {0};
};
#endif

// For use in the changelist
typedef enum EventFlags_Change {
    EVENTFLAGS_CHANGE_ADD_REMOVE = 1 << 0,
    EVENTFLAGS_CHANGE_CANARY
} EventFlags_Change;

/// Both READY_TO_READ and READY_TO_WRITE events have this data sent with them.
typedef struct EventData_ReadyToReadWrite {
    size_t BytesAvailable;
} EventData_ReadyToReadWrite;
typedef struct EventData_KeyboardInput {
    size_t value;
    bool press;
} EventData_KeyboardInput;
typedef struct EventData_MouseInput {
    int32_t delta_x;
    int32_t delta_y;
    int32_t wheel_delta;
} EventData_MouseInput;

#endif  // LENSOROS_DEFINES_KQUEUE_EVENTS_H
