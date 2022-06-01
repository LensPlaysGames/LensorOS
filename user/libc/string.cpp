#include "string.h"

#include "errno.h"

/// Copying
void* memcpy(void* dst, const void* src, size_t n) {
    void* original_dst = dst;
    asm volatile
        ("rep movsb"
         : "+D"(dst), "+S"(src), "+c"(n)
         : // No inputs
         : "memory");
    return original_dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    unsigned char tmp[n];
    memcpy(tmp, src, n);
    memcpy(dst, tmp, n);
    return dst;
}

char* strcpy(char* dst, const char* src) {
    while (*src != '\0')
        *dst++ = *src++;
    *dst = '\0';
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    while (n--) {
        *dst++ = *src++;
        if (*src == '\0'){
            while (n--)
                *dst++ = '\0';
            break;
        }
    }
    return dst;
}


/// Concatenation
char* strcat(char* dst, const char* src) {
    while (*dst++);
    dst--;
    while ((*dst++ = *src++));
    return dst;
}

char* strncat(char* dst, const char* src, size_t n) {
    while (*dst++);
    dst--;
    while ((*dst++ = *src++) && n--);
    return dst;
}


/// Comparison
int memcmp(const void* a, const void* b, size_t n) {
    auto* s1 = (const unsigned char*)a;
    auto* s2 = (const unsigned char*)b;
    while (--n)
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    return 0;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 == *s2++)
        if (*s1++ == 0)
            return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)--s2;
}

int strcoll(const char* s1, const char* s2) {
    // TODO: Implement me!
    return 1;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0)
        return 0;
    do {
        if (*s1 != *s2++)
            return *(const unsigned char*)s1 - *(const unsigned char*)--s2;
        if (*s1++ == 0)
            break;
    } while (--n);
    return 0;
}

size_t strxfrm(char* dst, const char* src, size_t n) {
    // TODO: Implement me!
    return 1;
}


/// Searching
void* memchr(const void* ptr, int value, size_t n) {
    auto* block = (const unsigned char*)ptr;
    for (size_t i = 0; i < n; ++i)
        if (block[i] == (unsigned char)value)
            return (void*)((size_t)ptr + i);
    return NULL;
}

char* strchr(const char* str, int chr) {
    while (*str) {
        if (*str == chr) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

size_t strcspn(const char* src, const char* reject) {
    if (reject[0] == '\0')
        return 0;
    bool go_on = true;
    size_t ret = 0;
    size_t len = strlen(reject);
    while (*src++ && go_on) {
        go_on = false;
        for (size_t i = 0; i < len; ++i)
            if (*src != reject[i]) {
                go_on = true;
                break;
            }
        ret++;
    }
    return ret;
}

char* strpbrk(const char* src, const char* accept) {
    if (accept[0] == '\0')
        return NULL;
    size_t len = strlen(accept);
    while (*src++)
        for (size_t i = 0; i < len; ++i)
            if (*src == accept[i])
                break;
    if (*src == '\0')
        return NULL;
    return (char*)src;
}

char* strrchr(const char* str, int chr) {
    char* ret = NULL;
    while (*str++)
        if (*str == chr)
            ret = (char*)str;
    return ret;
}

size_t strspn(const char* src, const char* accept) {
    if (accept[0] == '\0')
        return 0;
    bool go_on = true;
    size_t ret = 0;
    size_t len = strlen(accept);
    while (*src++ && go_on) {
        go_on = false;
        for (size_t i = 0; i < len; ++i)
            if (*src == accept[i]) {
                go_on = true;
                break;
            }
        ret++;
    }
    return ret;
}

int strstr_compare(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2)
            return 0;
        s1++;
        s2++;
    }
    return (*s2 == '\0');
}

char* strstr(const char* haystack, const char* needle) {
    while (*haystack != '\0')
    {
        if ((*haystack == *needle) && strstr_compare(haystack, needle))
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}

/// This is not thread safe, at all!
char* strtok(char* str, const char* delim) {
    static char *lastToken = NULL;
    if (str == NULL ) {
        str = lastToken;
        if (str == NULL)
            return NULL;
    }
    else str += strspn(str, delim);
    char* tmp = strpbrk(str, delim);
    if (tmp) {
        *tmp = '\0';
        lastToken = tmp + 1;
    }
    else lastToken = NULL;
    return str;
}


/// Other
// This isn't really part of any standard, but it is
// expected to be there by quite a few programs anyway.
const void* memmem
(
 const void* haystack
 , size_t haystacklen
 , const void* needle
 , size_t needlelen
 )
{
    if (needlelen > haystacklen) return NULL;
    // Stop searching at the last possible position for a match,
    // which is haystack[ haystacklen - needlelen + 1 ].
    haystacklen -= needlelen - 1;
    while (haystacklen)
    {
        // Find the first byte in a potential match
        unsigned char* z = (unsigned char*)memchr((unsigned char*)haystack, *(unsigned char*)needle, haystacklen);
        if (!z) return NULL;
        // Check if there is enough space for there to actually be a match.
        ptrdiff_t delta = z - (unsigned char*)haystack;
        ptrdiff_t remaining = (ptrdiff_t)haystacklen - delta;
        if (remaining < 1) return NULL;
        // Advance pointer and update the amount of haystack remaining.
        haystacklen -= delta;
        haystack = z;
        // Did we find a match?
        if (!memcmp( haystack, needle, needlelen )) return haystack;
        // Ready for next loop
        haystack = (unsigned char*)haystack + 1;
        haystacklen -= 1;
    }
    return NULL;
}

void* memset(void* ptr, int value, size_t n) {
    for (size_t i = 0; i < n; ++i)
        *(unsigned char*)((size_t)ptr + i) = value;
    return ptr;
}

constexpr size_t errno_strings_size = 35;
static const char* errno_strings[] = {
    "An error number of zero usually means success, but I'm hesitant to put that here.",
    "Not permitted (EPERM)",
    "No file or directory (ENOENT)",
    "Process does not exist (ESRCH)",
    "System call was interrupted (EINTR)",
    "Input/Output error (EIO)",
    "No device or address (ENXIO)",
    "Too large (E2BIG)",
    "`exec` syscall error (ENOEXEC)",
    "Bad file (EBADF)",
    "No child processes exist (ECHILD)",
    "Try again (EAGAIN)",
    "No memory (ENOMEM)",
    "Access violation (EACCES)",
    "Bad address (EFAULT)",
    "Not a block device, but a block device is required (ENOTBLK)",
    "Busy (EBUSY)",
    "File exists (EEXIST)",
    "Device cross-link (EXDEV)",
    "No device (ENODEV)",
    "Not a directory (ENOTDIR)",
    "Is a directory (EISDIR)",
    "Invalid (EINVAL)",
    "File table overflow (ENFILE)",
    "Too many open files (EMFILE)",
    "No teletype, not a typewriter (ENOTTY)",
    "Text file is busy (ETXTBSY)",
    "File is too big (EFBIG)",
    "No space left on the device (ENOSPC)",
    "Illegal seek (ESPIPE)",
    "Read-only file system (EROFS)",
    "Too many links (EMLINK)",
    "Broken pipe (EPIPE)",
    "Math argument out of domain of func (EDOM)",
    "Math result not representable (ERANGE)",
};

char* strerror(int errnum) {
    if (errnum > errno_strings_size || errnum < 0)
        return (char*)"Invalid error number.";
    return (char*)errno_strings[errnum];
}

size_t strlen(const char* str) {
    size_t len { 0 };
    while (*(str++))
        ++len;
    return len;
}

size_t strnlen(const char* str, size_t maxlen) {
    size_t len { 0 };
    for (; len < maxlen && *str; str++)
        len++;
    return len;
}
