/*
    Copyright (C) 2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>

// Improvements over std::priority_queue: lazy updates, clear(), and using a
// min-heap.
//
// requires: T is GreaterThanComparable
template <typename T>
class PriorityQueue
{
public:
    PriorityQueue();

    void push(const T &elem);
    T pop();
    void clear();
    bool empty() const;

private:
    std::vector<T> q_;
    bool isDirty_;
};


template <typename T>
PriorityQueue<T>::PriorityQueue()
    : q_(),
    isDirty_(false)
{
}

template <typename T>
void PriorityQueue<T>::push(const T &elem)
{
    q_.push_back(elem);
    isDirty_ = true;
}

template <typename T>
T PriorityQueue<T>::pop()
{
    assert(!empty());

    if (isDirty_) {
        make_heap(std::begin(q_), std::end(q_), std::greater<T>());
        isDirty_ = false;
    }

    auto elem = q_.front();
    pop_heap(std::begin(q_), std::end(q_), std::greater<T>());
    q_.pop_back();
    return elem;
}

template <typename T>
void PriorityQueue<T>::clear()
{
    q_.clear();
    isDirty_ = false;
}

template <typename T>
bool PriorityQueue<T>::empty() const
{
    return q_.empty();
}

#endif
