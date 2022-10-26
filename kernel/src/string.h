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

#ifndef LENSOR_OS_STRING_H
#define LENSOR_OS_STRING_H

#include <cstr.h>
#include <integers.h>
#include <memory.h>
#include <memory/heap.h>

constexpr u64 STRING_INITIAL_LENGTH = 10;

class String {
public:
    // Default constructor
    String() : Length(STRING_INITIAL_LENGTH) {
        Buffer = new u8[Length+1];
        memset(Buffer, 0, Length);
    }

    // Copy constructor
    String(const String& original) {
        Length = original.length();
        Buffer = new u8[Length+1];
        Buffer[Length] = '\0';
        memcpy(original.bytes(), Buffer, Length);
    }

    String(const char* cstr) {
        Length = strlen(cstr)-1;
        Buffer = new u8[Length+1];
        memcpy((void*)cstr, Buffer, Length+1);
    }

    String(const char* cstr, u64 byteCount) {
        Length = byteCount;
        Buffer = new u8[Length+1];
        memcpy((void*)cstr, Buffer, Length);
        Buffer[Length] = '\0';
    }

    ~String() {
        delete[] Buffer;
    }

    u64 length() { return Length; }
    u64 length() const { return Length; }

    u8* bytes() const { return Buffer; }

    enum class Side {
        Left,
        Right,
    };

    // NOTE: Character at index is included in right side.
    String& chop(u64 index, Side side) {
        if (index > length() - 1)
            return *this;

        if (side == String::Side::Left) {
            Length = index;
            u8* oldBuffer = Buffer;
            Buffer = new u8[Length + 1];
            memcpy((void*)oldBuffer, Buffer, Length);
            Buffer[Length] = '\0';
            delete[] oldBuffer;
        }
        else {
            Length = strlen(&data()[index]) - 1;
            u8* oldBuffer = Buffer;
            Buffer = new u8[Length+1];
            memcpy((void*)&oldBuffer[index], Buffer, Length+1);
            delete[] oldBuffer;
        }
        return *this;
    }

    const char* data() const { return (const char*)Buffer; }

    /// Make a copy of the current contents of the string on the heap.
    /// NOTE: Free responsiblity is transferred to the caller upon returning.
    const char* data_copy() const {
        u8* copy = new u8[Length+1];
        memcpy(Buffer, copy, Length);
        copy[Length] = '\0';
        return (const char*)copy;
    }

    // Copy assignment
    String& operator = (const String& other) {
        if (this == &other)
            return *this;

        Length = other.Length;
        delete[] Buffer;
        Buffer = new u8[Length+1];
        Buffer[Length] = '\0';
        memcpy(other.Buffer, Buffer, Length);
        return *this;
    }

    // Move assignment
    String& operator = (String&& other) noexcept {
        if (this == &other)
            return *this;

        delete[] Buffer;
        // FIXME: Need atomic exchange here for Buffer pointer swap...
        Buffer = other.Buffer;
        other.Buffer = nullptr;
        Length = other.Length;
        other.Length = 0;
        return *this;
    }

    String& operator + (const String& other) {
        if (this == &other)
            return *this;

        u64 oldLength = Length;
        Length += other.Length;
        u8* newBuffer = new u8[Length+1];
        memcpy(&Buffer[0], &newBuffer[0], oldLength);
        memcpy(&other.Buffer[0], &newBuffer[oldLength], other.Length);
        u8* oldBuffer = Buffer;
        Buffer = newBuffer;
        delete[] oldBuffer;
        return *this;
    }

    String& operator + (const char* cstr) {
        if (cstr == nullptr)
            return *this;

        u64 stringLength = strlen(cstr);
        if (stringLength <= 1)
            return *this;

        u64 oldLength = Length;
        Length += stringLength - 1;
        u8* newBuffer = new u8[Length + 1];
        memcpy(&Buffer[0], &newBuffer[0], oldLength);
        memcpy((void*)cstr, &newBuffer[oldLength], stringLength - 1);
        newBuffer[Length] = '\0';
        u8* oldBuffer = Buffer;
        Buffer = newBuffer;
        delete[] oldBuffer;
        return *this;
    }

    String& operator += (const String& rhs) {
        this->operator+(rhs);
        return *this;
    }

    String& operator += (const char* cstr) {
        this->operator+(cstr);
        return *this;
    }

    u8& operator [] (u64 index) const {
        if (index >= Length)
            return Buffer[Length-1];

        return Buffer[index];
    }

    u8& operator [] (u64 index) {
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
