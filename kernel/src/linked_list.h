#ifndef LENSOR_OS_LINKED_LIST_H
#define LENSOR_OS_LINKED_LIST_H

#include "memory/heap.h"

template <typename T>
class SinglyLinkedList;

template <typename T>
class SinglyLinkedListNode {
    typedef T DataType;

    friend SinglyLinkedList<DataType>;
    
public:
    SinglyLinkedListNode(DataType value, SinglyLinkedListNode* next = nullptr)
        : Data(value), Next(next) {}

    DataType value()              { return Data; }
    SinglyLinkedListNode* next()  { return Next; }
    
private:    
    DataType Data;
    SinglyLinkedListNode* Next { nullptr };
};


template <typename T>
class SinglyLinkedList {
    typedef T DataType;
    typedef SinglyLinkedListNode<DataType> Node;

public:
    ~SinglyLinkedList() {
        Node* tmp { nullptr };
        while (Head) {
            tmp = Head;
            Head = Head->Next;
            delete tmp;
        }
    }

    void add(DataType value) {
        if (Tail == nullptr) {
            Head = new Node(value);
            Tail = Head;
        }
        else Head = new Node(value, Head);
        Length += 1;
    }

    DataType at(u64 index) {
        Node* it { Head };
        Node* out { nullptr };
        while (index-- && it) {
            out = it;
            it = it->next();
        }
        
        // FIXME: Bad method of avoiding null dereference (do error propagation instead)
        // This way of avoiding a null dereference is not the best,
        // as it assumes that `DataType` has a simple default constructor.
        if (out == nullptr)
            return {};

        return out->value();
    }

    template <typename Callback>
    void for_each(Callback onEachNode) {
        Node* it = Head;
        while (it) {
            onEachNode(it);
            it = it->next();
        }
    }

    u64 length() { return Length; }
    Node* head() { return Head; }
    Node* tail() { return Tail; }

private:
    u64 Length { 0 };
    Node* Head { nullptr };
    Node* Tail { nullptr };
};
#endif
