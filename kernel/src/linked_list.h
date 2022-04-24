#ifndef LENSOR_OS_LINKED_LIST_H
#define LENSOR_OS_LINKED_LIST_H

#include <memory/heap.h>

template <typename T>
class SinglyLinkedList;

template <typename T>
class SinglyLinkedListNode {
    typedef T DataType;

    friend SinglyLinkedList<DataType>;
    
public:
    explicit SinglyLinkedListNode(const DataType& value
                                  , SinglyLinkedListNode* next = nullptr)
        : Data(value), Next(next) {}

    DataType& value()             { return Data; }
    const DataType& value() const { return Data; }
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
        while (Head) {
            Node* tmp = Head;
            Head = Head->Next;
            delete tmp;
        }
    }

    void add(const DataType& value) {
        if (Tail == nullptr) {
            Head = new Node(value);
            Tail = Head;
        }
        else Head = new Node(value, Head);
        Length += 1;
    }

    void add_end(const DataType& value) {
        if (Tail == nullptr) {
            Head = new Node(value);
            Tail = Head;
        }
        else {
            Tail->Next = new Node(value, Head);
            Tail = Tail->Next;
        }
        Length += 1;
    }

    DataType& at(u64 index) {
        Node* it { Head };
        Node* out { nullptr };
        index += 1;
        while (it && index--) {
            out = it;
            it = it->next();
        }
        // FIXME: No avoidance of null dereference in
        //        case of empty list (do error propagation!).
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

    bool remove(u64 index) {
        if (index >= Length)
            return false;

        if (index == 0) {
            Head = Head->next();
            return true;
        }

        Node* prev = Head;
        Node* current = Head;
        u64 i = 1;
        while (current) {
            current = current->next();
            if (i == index)
                break;

            prev = current;
            i++;
        }
        prev->Next = current->next();
        Length--;
        delete current;
        return true;
    }

    u64 length() const { return Length; }

    Node* head() { return Head; }
    const Node* head() const { return Head; }

    Node* tail() { return Tail; }
    const Node* tail() const { return Tail; }

    DataType& operator [] (u64 index) {
        return at(index);
    }

    const DataType& operator [] (u64 index) const {
        return at(index);
    }

private:
    u64 Length { 0 };
    Node* Head { nullptr };
    Node* Tail { nullptr };
};
#endif
