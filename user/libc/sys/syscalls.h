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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <bits/decls.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __lensor__
#define SYS_open    0
#define SYS_close   1
#define SYS_read    2
#define SYS_write   3
#define SYS_poke    4
#define SYS_exit    5
#define SYS_map     6
#define SYS_unmap   7
#define SYS_time    8
#define SYS_waitpid 9
#define SYS_fork    10
#define SYS_exec    11
#define SYS_repfd   12
#define SYS_pipe    13
#define SYS_seek    14
#define SYS_pwd     15
#define SYS_dup     16
#define SYS_uart    17
#define SYS_socket  18
#define SYS_bind    19
#define SYS_listen  20
#define SYS_connect 21
#define SYS_accept  22
#define SYS_MAXSYSCALL 22
#else
#define SYS_read  0
#define SYS_write 1
#define SYS_open  2
#define SYS_close 3
#define SYS_exit  60
#define SYS_MAXSYSCALL 325
#endif


/// ===========================================================================
///  Syscall functions.
/// ===========================================================================
__BEGIN_DECLS__
#define __a uintptr_t

/// Syscall argument registers.
#if defined(__lensor__)
#define _R1 "D"
#define _R2 "S"
#define _R3 "d"
#define _R4 "rcx"
#define _R5 "r8"
#define _R6 "r9"
#define _SYSCALL "int $0x80"
#elif defined(__linux__)
#define _R1 "D"
#define _R2 "S"
#define _R3 "d"
#define _R4 "r10"
#define _R5 "r8"
#define _R6 "r9"
#define _SYSCALL "syscall"
#else
#error "Unsupported platform."
#endif

#define _DEFINE_SYSCALL(_N, _Args, ...)                                                 \
    __attribute__((__always_inline__, __artificial__)) inline __a __syscall##_N _Args { \
        __a __result;                                                                   \
        __asm__ __volatile__                                                            \
            (_SYSCALL "\n"                                                              \
             : "=a"(__result)                                                           \
             : "a"(__n) __VA_OPT__(, ) __VA_ARGS__                                      \
             : "memory"                                                                 \
        );                                                                              \
        return __result;                                                                \
    }

#define _DEFINE_SYSCALL_ARGS(...) (__a __n __VA_OPT__(, ) __VA_ARGS__)

_DEFINE_SYSCALL(0, _DEFINE_SYSCALL_ARGS())
_DEFINE_SYSCALL(1, _DEFINE_SYSCALL_ARGS(__a __1), _R1(__1))
_DEFINE_SYSCALL(2, _DEFINE_SYSCALL_ARGS(__a __1, __a __2), _R1(__1), _R2(__2))
_DEFINE_SYSCALL(3, _DEFINE_SYSCALL_ARGS(__a __1, __a __2, __a __3), _R1(__1), _R2(__2), _R3(__3))

__attribute__((__always_inline__, __artificial__))
inline __a __syscall4(__a __n, __a __1, __a __2, __a __3, __a __4) {
    __a __result;
    __asm__ __volatile__
        ("movq %0, %%" _R4 "\n"
         _SYSCALL "\n"
         : "=a"(__result)
         : "a"(__n), _R1(__1), _R2(__2), _R3(__3), "r"(__4)
         : "memory", _R4);
    return __result;
}

__attribute__((__always_inline__, __artificial__))
inline __a __syscall5(__a __n, __a __1, __a __2, __a __3, __a __4, __a __5) {
    __a __result;
    __asm__ __volatile__
        ("movq %0, %%" _R4 "\n"
         "movq %1, %%" _R5 "\n"
         _SYSCALL "\n"
         : "=a"(__result)
         : "a"(__n), _R1(__1), _R2(__2), _R3(__3), "r"(__4), "r"(__5)
         : "memory", _R4, _R5);
    return __result;
}

__attribute__((__always_inline__, __artificial__))
inline __a __syscall6(__a __n, __a __1, __a __2, __a __3, __a __4, __a __5, __a __6) {
    __a __result;
    __asm__ __volatile__
        ("movq %0, %%" _R4 "\n"
         "movq %1, %%" _R5 "\n"
         "movq %2, %%" _R6 "\n"
         _SYSCALL "\n"
         : "=a"(__result)
         : "a"(__n), _R1(__1), _R2(__2), _R3(__3), "r"(__4), "r"(__5), "r"(__6)
         : "memory", _R4, _R5, _R6);
    return __result;
}

#undef __a
#undef _R1
#undef _R2
#undef _R3
#undef _R4
#undef _R5
#undef _R6

#define SOCK_ADDR_MAX_SIZE 16
typedef struct sockaddr {
    enum {
        UNBOUND,
        // 16 bytes; simply a unique identifier which is memcmp'd
        // Used by LENSOR type sockets.
        LENSOR16,
    } type;
    uint8_t data[SOCK_ADDR_MAX_SIZE];
} sockaddr;

typedef struct tm {
    int seconds;                  // seconds,  0--59
    int minutes;                  // minutes,  0--59
    int hours;                    // hours, 0 to 23
    int day_of_month;             // day of the month, 1--31
    int month;                    // month, 0--11
    int years_since_1900;         // The number of years since 1900
    int day_of_week;              // day of the week, 0--6
    int day_of_year;              // day in the year, 0--365
    int is_daylight_savings_time; // daylight saving time
} tm;

__END_DECLS__


/// ===========================================================================
///  C Interface.
/// ===========================================================================
#ifndef __cplusplus

/// Abandon all hope, ye who enter here.
typedef struct {int __unused;}* __empty_type_t;
#define _VA_FIRST(_First, ...) _First
#define _VA_REST(_X, ...) __VA_ARGS__
#define _EMPTY_VAL ((__empty_type_t)0)

#define __syscall_at_least5(...)                                \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
     __empty_type_t: __syscall5,                                \
     default:        __syscall6)

#define __syscall_at_least4(...)                                \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
     __empty_type_t: __syscall4,                                \
     default:        __syscall_at_least5(_VA_REST(__VA_ARGS__)))

#define __syscall_at_least3(...)                                \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
     __empty_type_t: __syscall3,                                \
     default:        __syscall_at_least4(_VA_REST(__VA_ARGS__)))

#define __syscall_at_least2(...)                                \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
     __empty_type_t: __syscall2,                                \
     default:        __syscall_at_least3(_VA_REST(__VA_ARGS__)))

#define __syscall_at_least1(...)                                \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
     __empty_type_t: __syscall1,                                \
     default:        __syscall_at_least2(_VA_REST(__VA_ARGS__)))

#define syscall(_Sys, ...)                                      \
_Generic((_VA_FIRST(__VA_ARGS__ __VA_OPT__(,) _EMPTY_VAL)),     \
    __empty_type_t: __syscall0,                                 \
    default:        __syscall_at_least1(_VA_REST(__VA_ARGS__))) \
    (_Sys __VA_OPT__(, (uintptr_t)) __VA_ARGS__)


typedef uint64_t ProcessFileDescriptor;
typedef ProcessFileDescriptor ProcFD;

ProcFD sys_open(const char* path) {
    return (ProcFD)syscall(SYS_open, (uintptr_t)path);
}
void sys_close(ProcFD fd) {
    syscall(SYS_close, (uintptr_t)fd);
}
int sys_read(ProcFD fd, uint8_t* buffer, uint64_t byteCount) {
    return (int)syscall(SYS_read, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)byteCount);
}
int sys_write(ProcFD fd, uint8_t* buffer, uint64_t byteCount) {
    return (int)syscall(SYS_write, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)byteCount);
}
void sys_poke() {
    syscall(SYS_poke);
}
void sys_exit(int status) {
    syscall(SYS_exit, (uintptr_t)status);
}
void* sys_map(void* address, size_t size, uint64_t flags) {
    return (void*)syscall(SYS_map, (uintptr_t)address, (uintptr_t)size, (uintptr_t)flags);
}
void sys_unmap(void* address) {
    syscall(SYS_unmap, (uintptr_t)address);
}
void sys_time(tm* time) {
    syscall(SYS_time, (uintptr_t)time);
}
int sys_waitpid(pid_t pid) {
    return (int)syscall(SYS_waitpid, (uintptr_t)pid);
}
pid_t sys_fork() {
    return (pid_t)syscall(SYS_fork);
}
void sys_exec(const char *path, const char **args) {
    syscall(SYS_exec, (uintptr_t)path, (uintptr_t)args);
}
void sys_repfd(ProcessFileDescriptor fd, ProcessFileDescriptor replaced) {
    syscall(SYS_repfd, (uintptr_t)fd, (uintptr_t)replaced);
}
void sys_pipe(ProcessFileDescriptor *fds) {
    syscall(SYS_pipe, (uintptr_t)fds);
}
/// SEEK_CUR == 0 == OFFSET is based off of current offset (relative).
/// SEEK_END == 1 == OFFSET is from end of file (must be negative).
/// SEEK_SET == 2 == OFFSET is from beginning of file (must be positive).
int sys_seek(ProcessFileDescriptor fd, ssize_t offset, int whence) {
    return (int)syscall(SYS_seek, (uintptr_t)fd, (uintptr_t)offset, (uintptr_t)whence);
}
bool sys_pwd(char *buffer, size_t numBytes) {
    return (bool)syscall(SYS_pwd, (uintptr_t)buffer, (uintptr_t)numBytes);
}
ProcFD sys_dup(ProcessFileDescriptor fd) {
    return (ProcFD)syscall(SYS_dup, (uintptr_t)fd);
}
void sys_uart(void* buffer, size_t size) {
    syscall(SYS_uart, (uintptr_t)buffer, (uintptr_t)size);
}
ProcFD sys_socket(int domain, int type, int protocol) {
    return (ProcFD)syscall(SYS_socket, (uintptr_t)domain, (uintptr_t)type, (uintptr_t)protocol);
}
int sys_bind(ProcFD socketFD, const sockaddr* address, size_t addressLength) {
    return (int)syscall(SYS_bind, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}
int sys_listen(ProcFD socketFD, int backlog) {
    return (int)syscall(SYS_listen, (uintptr_t)socketFD, (uintptr_t)backlog);
}
int sys_connect(ProcFD socketFD, const sockaddr* address, size_t addressLength) {
    return (int)syscall(SYS_connect, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}
ProcFD sys_accept(ProcFD socketFD, const sockaddr* address, size_t* addressLength) {
    return (ProcFD)syscall(SYS_accept, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}

/// ===========================================================================
///  C++ Interface.
/// ===========================================================================
#else

#include <type_traits>

namespace std {
namespace __detail {

/// Perform a system call.
template <
    typename _Ret = uintptr_t,
    typename... _Args,
    typename = _Requires<
        bool_constant<sizeof...(_Args) <= 6>,
        _Or<_Number<_Args>,
            _Pointer<_Args>>...>
>
[[__gnu__::__always_inline__, __gnu__::__artificial__]]
inline _Ret syscall(uintptr_t __sys, _Args&& ...__args) {
    __if   (sizeof...(_Args) == 0) return _Ret(__syscall0(__sys));
    __elif (sizeof...(_Args) == 1) return _Ret(__syscall1(__sys, (uintptr_t)__args...));
    __elif (sizeof...(_Args) == 2) return _Ret(__syscall2(__sys, (uintptr_t)__args...));
    __elif (sizeof...(_Args) == 3) return _Ret(__syscall3(__sys, (uintptr_t)__args...));
    __elif (sizeof...(_Args) == 4) return _Ret(__syscall4(__sys, (uintptr_t)__args...));
    __elif (sizeof...(_Args) == 5) return _Ret(__syscall5(__sys, (uintptr_t)__args...));
    __elif (sizeof...(_Args) == 6) return _Ret(__syscall6(__sys, (uintptr_t)__args...));
    __else __builtin_unreachable();

}

} // namespace __detail

using ProcessFileDescriptor = uint64_t;
using ProcFD = ProcessFileDescriptor;

inline ProcFD sys_open(const char* path) {
    return std::__detail::syscall<ProcFD>(SYS_open, path);
}
inline void sys_close(ProcFD fd) {
    std::__detail::syscall(SYS_close, (uintptr_t)fd);
}
inline int sys_read(ProcFD fd, uint8_t* buffer, uint64_t byteCount) {
    return std::__detail::syscall<int>(SYS_read, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)byteCount);
}
inline int sys_write(ProcFD fd, uint8_t* buffer, uint64_t byteCount) {
    return std::__detail::syscall<int>(SYS_write, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)byteCount);
}
inline void sys_poke() {
    std::__detail::syscall(SYS_poke);
}
inline void sys_exit(int status) {
    std::__detail::syscall(SYS_exit, (uintptr_t)status);
}
inline void* sys_map(void* address, size_t size, uint64_t flags) {
    return std::__detail::syscall<void*>(SYS_map, (uintptr_t)address, (uintptr_t)size, (uintptr_t)flags);
}
inline void sys_unmap(void* address) {
    std::__detail::syscall(SYS_unmap, (uintptr_t)address);
}
inline void sys_time(tm* time) {
    std::__detail::syscall(SYS_time, (uintptr_t)time);
}
inline int sys_waitpid(pid_t pid) {
    return std::__detail::syscall<int>(SYS_waitpid, (uintptr_t)pid);
}
inline pid_t sys_fork() {
    return std::__detail::syscall<pid_t>(SYS_fork);
}
inline void sys_exec(const char *path, const char **args) {
    std::__detail::syscall(SYS_exec, (uintptr_t)path, (uintptr_t)args);
}
inline void sys_repfd(ProcessFileDescriptor fd, ProcessFileDescriptor replaced) {
    std::__detail::syscall(SYS_repfd, (uintptr_t)fd, (uintptr_t)replaced);
}
inline void sys_pipe(ProcessFileDescriptor *fds) {
    std::__detail::syscall(SYS_pipe, (uintptr_t)fds);
}
/// SEEK_CUR == 0 == OFFSET is based off of current offset  (relative).
/// SEEK_END == 1 == OFFSET is from end of file (must be negative).
/// SEEK_SET == 2 == OFFSET is from beginning of file (must be positive).
inline int sys_seek(ProcessFileDescriptor fd, ssize_t offset, int whence) {
    return std::__detail::syscall<int>(SYS_seek, (uintptr_t)fd, (uintptr_t)offset, (uintptr_t)whence);
}
inline bool sys_pwd(char *buffer, size_t numBytes) {
    return std::__detail::syscall<bool>(SYS_pwd, (uintptr_t)buffer, (uintptr_t)numBytes);
}
inline ProcFD sys_dup(ProcessFileDescriptor fd) {
    return std::__detail::syscall<ProcFD>(SYS_dup, (uintptr_t)fd);
}
inline void sys_uart(void* buffer, size_t size) {
    std::__detail::syscall(SYS_uart, (uintptr_t)buffer, (uintptr_t)size);
}
inline ProcFD sys_socket(int domain, int type, int protocol) {
    return std::__detail::syscall<ProcFD>(SYS_socket, (uintptr_t)domain, (uintptr_t)type, (uintptr_t)protocol);
}
inline int sys_bind(ProcFD socketFD, const sockaddr* address, size_t addressLength) {
    return std::__detail::syscall<int>(SYS_bind, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}
inline int sys_listen(ProcFD socketFD, int backlog) {
    return std::__detail::syscall<int>(SYS_listen, (uintptr_t)socketFD, (uintptr_t)backlog);
}
inline int sys_connect(ProcFD socketFD, const sockaddr* address, size_t addressLength) {
    return std::__detail::syscall<int>(SYS_connect, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}
inline ProcFD sys_accept(ProcFD socketFD, const sockaddr* address, size_t* addressLength) {
    return std::__detail::syscall<ProcFD>(SYS_accept, (uintptr_t)socketFD, (uintptr_t)address, (uintptr_t)addressLength);
}

} // namespace std

using std::__detail::syscall;

#endif /* #if defined (__cplusplus) */

#endif /* #ifndef _SYSCALLS_H */
