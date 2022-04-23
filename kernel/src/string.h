#ifndef LENSOR_OS_STRING_H
#define LENSOR_OS_STRING_H

#include <cstr.h>
#include <integers.h>
#include <memory.h>
#include <memory/heap.h>

constexpr u64 STRING_INITIAL_LENGTH = 10;

class String {
public:
    String() {
        Length = STRING_INITIAL_LENGTH;
        Buffer = new u8[Length];
        memset(Buffer, 0, Length);
    }

    String(const char* cstr) {
        Length = strlen(cstr) - 1;
        Buffer = new u8[Length];
        memcpy((void*)cstr, Buffer, Length);
    }

    ~String() {
        delete[] Buffer;
    }

    u64 length() { return Length; }
    u8* bytes() { return Buffer; }

    // Copy assignment
    String& operator = (const String& other) {
        if (this == &other)
            return *this;

        delete[] Buffer;
        Buffer = new u8[other.Length];
        Length = other.Length;
        memcpy(other.Buffer, Buffer, Length);
        return *this;
    }

    // Move assignment
    String& operator = (String&& other) noexcept {
        if (this == &other)
            return *this;

        delete[] Buffer;
        // FIXME: Need atomic exchange here?
        Buffer = other.Buffer;
        other.Buffer = nullptr;
        Length = other.Length;
        other.Length = 0;
        return *this;
    }

    // Copy and swap assignment
    String& operator = (String other) noexcept {
        u8* buffer = Buffer;
        Buffer = other.Buffer;
        other.Buffer = buffer;
        u64 length = Length;
        Length = other.Length;
        other.Length = length;
        // Rely on other.~String to clean up.
        return *this;
    }

private:
    u8* Buffer;
    u64 Length;
};

inline String& operator << (String& str, const u8& obj) {
    (void)str;
    (void)obj;
    return str;
}

#endif /* LENSOR_OS_STRING_H */
