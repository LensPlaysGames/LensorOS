#ifndef LIBC_TESTING_HH
#define LIBC_TESTING_HH

#include <sys/syscalls.h>

/// Prevent a variable from being optimised away.
#define used(var) __asm__ __volatile__ ("" :: "m"(var))

/// Ensure a condition is true.
struct $$ensure_helper {
    int _line;
    explicit $$ensure_helper(int line) : _line(line) {}

    /// We use "," because it has the lowest precedence of all operators.
    void operator ,(bool condition) const {
        if (!condition) {
            /// Use syscall(), concatenate the string, and convert the line number
            /// manually to minimise dependencies. We can’t exactly test e.g.
            /// memcpy() or strcat() if we use them here...
            char buf[256] = {
                ' ', ' ', ' ', ' ',
                '\033', '[', '3', '1', 'm',
                'E', 'n', 's', 'u', 'r', 'e', ' ',
                'f', 'a', 'i', 'l', 'e', 'd', ' ',
                'o', 'n', ' ',
                'l', 'i', 'n', 'e', ' ',
            };
            __SIZE_TYPE__ start = 31;
            {
                char num[16];
                __SIZE_TYPE__ i = 0;
                auto line = _line;
                do {
                    num[i++] = char('0' + (line % 10));
                    line /= 10;
                } while (line);
                while (i) {
                    buf[start++] = num[--i];
                }
            }
            buf[start++] = '\033';
            buf[start++] = '[';
            buf[start++] = '0';
            buf[start++] = 'm';
            buf[start++] = '\n';

            syscall(SYS_write, 2, buf, start);
            syscall(SYS_exit, 1);
        }
    }
};

#define ensure $$ensure_helper(__LINE__) ,

/// Bootstrap comparison operations.
inline bool eq(const void* a, const void* b, __SIZE_TYPE__ size) {
    for (__SIZE_TYPE__ i = 0; i < size; i++) {
        if (((const char*)a)[i] != ((const char*)b)[i]) {
            return false;
        }
    }

    return true;
}



#endif //LIBC_TESTING_HH
