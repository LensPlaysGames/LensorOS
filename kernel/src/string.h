#ifndef LENSOR_OS_STRING_H
#define LENSOR_OS_STRING_H

#include <cstr.h>
#include <integers.h>
#include <memory.h>
#include <memory/heap.h>

constexpr u64 STRING_INITIAL_LENGTH = 10;

class String {
public:
    String() : Length(STRING_INITIAL_LENGTH) {
        Buffer = new u8[Length];
        memset(Buffer, 0, Length);
    }

    String(const char* cstr) {
        Length = strlen(cstr) - 1;
        Buffer = new u8[Length];
        memcpy((void*)cstr, Buffer, Length);
    }

    String(const String& original) {
        Length = original.length();
        Buffer = new u8[Length];
        memcpy(original.bytes(), Buffer, Length);
    }

    ~String() {
        delete[] Buffer;
    }

    u64 length() { return Length; }
    u64 length() const { return Length; }

    u8* bytes() const { return Buffer; }

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

    String& operator + (const String& other) {
        if (this == &other)
            return *this;

        // FIXME: This code would fail with multiple threads.
        //        delete, switch context, access deleted buffer, etc...
        //        Need atomic exchange.
        u64 oldLength = Length;
        Length += other.Length;
        u8* temporaryBuffer = new u8[oldLength];
        memcpy(Buffer, temporaryBuffer, oldLength);
        delete[] Buffer;
        Buffer = new u8[Length];
        memcpy(temporaryBuffer, Buffer, oldLength);
        memcpy(other.Buffer, &Buffer[oldLength], other.Length);
        return *this;
    }

    String& operator += (const String& rhs) {
        this->operator+(rhs);
        return *this;
    }

    u8 operator [] (u64 index) {
        if (index >= Length)
            return Buffer[Length-1];

        return Buffer[index];
    }

private:
    u8* Buffer { nullptr };
    u64 Length { 0 };
};

inline String& operator << (String& lhs, const String& rhs) {
    lhs += rhs;
    return lhs;
}

inline bool operator == (const String& lhs, const String& rhs) {
    if (lhs.length() != rhs.length())
        return false;

    return !memcmp(lhs.bytes(), rhs.bytes(), lhs.length());
}

inline bool operator != (const String& lhs, const String& rhs) {
    return !(lhs == rhs);
}

#endif /* LENSOR_OS_STRING_H */
