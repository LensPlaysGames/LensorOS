#ifndef LENSOR_OS_LINKED_LIST_H
#define LENSOR_OS_LINKED_LIST_H

#include <debug.h>
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
        auto* newHead = new Node(value, Head);
        if (newHead != nullptr) {
            Head = newHead;
            if (Tail == nullptr)
                Tail = Head;
            Length += 1;
        }
        else dbgmsg_s("Failed to allocate memory for linked list.");
    }

    void add_end(const DataType& value) {
        // Handle empty list case.
        if (Head == nullptr)
            add(value);
        else {
            auto* newTail = new Node(value, nullptr);
            if (newTail != nullptr) {
                // Prevent nullptr dereference.
                if (Tail == nullptr)
                    Tail = Head;
                // Place new node at end of list.
                Tail->Next = newTail;
                Tail = Tail->next();
                Length += 1;
            }
            else dbgmsg_s("Failed to allocate memory for linked list.");
        }
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

        // Handle head removal
        if (index == 0) {
            if (Head != nullptr) {
                Node* old = Head;
                Head = Head->next();
                Length -= 1;
                delete old;
            }
            // If head is nullptr, ensure tail is as well.
            else Tail = nullptr;
            return true;
        }

        Node* prev = Head;
        Node* current = Head;
        Node* next = Head->next();
        u64 i = 1;
        while (current) {
            current = current->next();
            if (current)
                next = current->next();

            if (i >= index)
                break;

            prev = current;
            i++;
        }
        // Set Tail if removing last item.
        if (next == nullptr)
            Tail = prev;

        prev->Next = next;
        Length -= 1;
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
