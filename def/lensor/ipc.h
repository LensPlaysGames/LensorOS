#ifndef LENSOROS_DEFINES_IPC_H
#define LENSOROS_DEFINES_IPC_H

#include <stdint.h>

#define IPC_KEYBOARD_MAGIC 0xf8
typedef struct ipc_keyboard_t {
    uint8_t magic;
    uint8_t is_pressed;
    uint16_t value;
} ipc_keyboard_t;

#define IPC_MOUSE_POSITION_MAGIC 0xf9
typedef struct ipc_mouse_position_t {
    uint8_t magic;
    int32_t local_x;
    int32_t local_y;
} ipc_mouse_position_t;

#define IPC_MOUSE_DELTA_MAGIC 0xfa
typedef struct ipc_mouse_delta_t {
    uint8_t magic;
    int32_t delta_x;
    int32_t delta_y;
} ipc_mouse_delta_t;

#endif /* LENSOROS_DEFINES_IPC_H */
