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

#ifndef LENSOR_OS_SMART_POINTER_H
#define LENSOR_OS_SMART_POINTER_H

#include <integers.h>

template <typename T>
class SmartPtr {
public:
    SmartPtr() : Pointer(nullptr) {}
    SmartPtr(T* ptr) : Pointer(ptr) {}

    ~SmartPtr() {
        if (Pointer != nullptr)
            delete Pointer;
    }

    T* get() {
        return this->Pointer;
    }

    T* operator -> () {
        return this->Pointer;
    }

    T& operator * () {
        return *(this->Pointer);
    }

private:
    T* Pointer;
};

template <typename T>
class SmartPtr<T[]> {
public:
    SmartPtr() : Pointer(nullptr), Size(0) {}
    SmartPtr(T* ptr, u64 size) : Pointer(ptr), Size(size) {}
    SmartPtr(T* ptr, int size) : Pointer(ptr), Size((u64)size) {}

    ~SmartPtr() {
        if (Pointer != nullptr)
            delete[] Pointer;
    }

    T* get() {
        return this->Pointer;
    }

    T* operator -> () {
        return this->Pointer;
    }

    T& operator * () {
        return *(this->Pointer);
    }

    T& operator [] (int index) {
        if (index < 0) {
            // TODO: Throw exception, or something.
            return *(this->Pointer);
        }
        if ((u64)index >= Size) {
            // TODO: Something about it.
            return *(this->Pointer);
        }
        return this->Pointer[index];
    }

    u64 size() const { return Size; }

private:
    T* Pointer;
    u64 Size { 0 };
};

#endif /* LENSOR_OS_SMART_POINTER_H */
