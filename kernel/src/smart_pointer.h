#ifndef LENSOR_OS_SMART_POINTER_H
#define LENSOR_OS_SMART_POINTER_H

#include <integers.h>

template <typename T>
class SmartPtr {
public:
    SmartPtr() : ptr(nullptr) {}
    SmartPtr(T* ptr) : ptr(ptr) {}

    ~SmartPtr() {
        if (ptr != nullptr)
            delete ptr;
    }

    T* get() {
        return this->ptr;
    }

    T* operator -> () {
        return this->ptr;
    }

    T& operator * () {
        return *(this->ptr);
    }

private:
    T* ptr;
};

template <typename T>
class SmartPtr<T[]> {
public:
    SmartPtr() : ptr(nullptr) {}
    SmartPtr(T* ptr, u64 size) : ptr(ptr), size(size) {}
    SmartPtr(T* ptr, int size) : ptr(ptr), size((u64)size) {}

    ~SmartPtr() {
        if (ptr != nullptr)
            delete[] ptr;
    }

    T* get() {
        return this->ptr;
    }

    T* operator -> () {
        return this->ptr;
    }

    T& operator * () {
        return *(this->ptr);
    }

    T& operator [] (int index) {
        if (index < 0) {
            // TODO: Throw exception, or something.
            return *(this->ptr);
        }
        if ((u64)index >= size) {
            // TODO: Throw exception...
            return *(this->ptr);
        }
        return this->ptr[index];            
    }

private:
    T* ptr;
    u64 size;
};

#endif /* LENSOR_OS_SMART_POINTER_H */
