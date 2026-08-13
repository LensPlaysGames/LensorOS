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

#ifndef LENSOR_OS_LINKED_LIST_H
#define LENSOR_OS_LINKED_LIST_H

#include <bits/terminate.h>
#include <debug.h>
#include <memory/heap.h>

template <typename T>
class SinglyLinkedList;

template <typename T>
class SinglyLinkedListNode {
    using DataType = T;

    friend SinglyLinkedList<DataType>;

    DataType Data;
    SinglyLinkedListNode* Next{nullptr};

   public:
    explicit SinglyLinkedListNode(const DataType& value, SinglyLinkedListNode* next = nullptr)
        : Data(value), Next(next) {}

    DataType& value() { return Data; }
    const DataType& value() const { return Data; }
    SinglyLinkedListNode* next() { return Next; }
};

template <typename T>
class SinglyLinkedList {
    using DataType = T;
    using Node = SinglyLinkedListNode<DataType>;

   public:
    struct Iterator {
        Node* node;

        Iterator(Node* n) : node(n) {}

        Iterator& operator++() {
            if (node) node = node->next();
            return *this;
        }
        Iterator operator++(int) {
            if (node) return {node->next()};
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return node == other.node;
        }
        bool operator!=(const Iterator& other) const {
            return !(operator==(other));
        }

        DataType& operator*() const {
            return node->value();
        }
    };

    Iterator begin() {
        return {Head};
    }
    Iterator end() {
        return {nullptr};
    }

    DataType& front() {
        if (Head) return Head->value();
        return {};
    }
    DataType& back() {
        if (Head) return Head->value();
        return {};
    }

    ~SinglyLinkedList() {
        while (Head) {
            Node* tmp = Head;
            Head = Head->Next;
            delete tmp;
        }
    }

    void add(const DataType& value) {
        auto* newHead = new Node(value, Head);
        if (!newHead) __terminate_with_message("Failed to allocate memory for linked list.");
        Head = newHead;
        if (Tail == nullptr) Tail = Head;
        Length += 1;
    }

    void add_end(const DataType& value) {
        // Handle empty list case.
        if (Head == nullptr)
            add(value);
        else {
            auto* newTail = new Node(value, nullptr);
            if (!newTail) __terminate_with_message("Failed to allocate memory for linked list.");
            // Prevent nullptr dereference.
            if (Tail == nullptr) Tail = Head;
            // Place new node at end of list.
            Tail->Next = newTail;
            Tail = Tail->next();
            Length += 1;
        }
    }

    DataType& at(u64 index) {
        Node* it{Head};
        Node* out{nullptr};
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
        if (index >= Length or Head == nullptr)
            return false;

        // Handle head removal
        if (index == 0) {
            Node* old = Head;
            Head = Head->next();
            Length -= 1;
            delete old;

            if (Length == 0)
                Tail = nullptr;

            return true;
        }

        Node* prev = Head;
        for (u64 i = 0; i < index - 1; ++i)
            prev = prev->next();

        // NOTE: prev cannot be nullptr if Length was valid.
        Node* current = prev->next();

        prev->Next = current->Next;
        Length -= 1;
        delete current;

        if (current == Tail)
            Tail = prev;

        return true;
    }

    u64 length() const { return Length; }

    Node* head() { return Head; }
    const Node* head() const { return Head; }

    Node* tail() { return Tail; }
    const Node* tail() const { return Tail; }

    DataType& operator[](u64 index) {
        return at(index);
    }

    const DataType& operator[](u64 index) const {
        return at(index);
    }

   private:
    u64 Length{0};
    Node* Head{nullptr};
    Node* Tail{nullptr};
};
#endif
