/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef FLAT_MULTIMAP_H
#define FLAT_MULTIMAP_H

#include <algorithm>
#include <tuple>
#include <vector>

// Implementation of a multimap on top of contiguous storage. Performs best if all
// insertions are done first, followed by all reads. This class differs from
// std::multimap by not allowing duplicate values for each key.
template <typename K, typename V>
class FlatMultimap
{
public:
    FlatMultimap();

    // Insert a new key-value pair, don't worry about duplicates yet.
    void insert(K key, V value);

    // Return the set of all values matching 'key'.
    //
    // Example range-for usage:
    //     for (const auto &v : map.find(key)) {
    //         ...
    //     }
    // 
    // Example traditional usage:
    //     const auto range = map.find(key);
    //     for (auto i = range.first; i != range.second; ++i) {
    //         ...
    //     }
    struct ValueRange;
    ValueRange find(const K &key);

    void reserve(int capacity);
    void shrink_to_fit();

private:
    void sortAndPrune();

    struct KeyValue;
    using container_type = std::vector<KeyValue>;
    container_type data_;
    bool isDirty_;

// *** IMPLEMENTATION DETAILS ***
public:
    class ValueIterator
    {
    public:
        ValueIterator(typename container_type::const_iterator i) : iter_(std::move(i)) {}
        const V & operator*() { return iter_->value; }
        const V * operator->() { return iter_->value; }
        ValueIterator & operator++() { ++iter_; return *this; }
        friend bool operator==(const ValueIterator &lhs, const ValueIterator &rhs)
        {
            return lhs.iter_ == rhs.iter_;
        }
        friend bool operator!=(const ValueIterator &lhs, const ValueIterator &rhs)
        {
            return !(lhs == rhs);
        }

    private:
        typename container_type::const_iterator iter_;
    };

    struct ValueRange
    {
        ValueIterator first;
        ValueIterator second;

        auto begin() { return first; }
        auto end() { return second; }
    };

private:
    struct KeyValue
    {
        K key;
        V value;
    };

    friend bool operator<(const KeyValue &lhs, const KeyValue &rhs)
    {
        return std::tie(lhs.key, lhs.value) < std::tie(rhs.key, rhs.value);
    }

    friend bool operator<(const KeyValue &lhs, const K &rhs)
    {
        return lhs.key < rhs;
    }

    friend bool operator<(const K &lhs, const KeyValue &rhs)
    {
        return lhs < rhs.key;
    }

    friend bool operator==(const KeyValue &lhs, const KeyValue &rhs)
    {
        return lhs.key == rhs.key && lhs.value == rhs.value;
    }
};

template <typename K, typename V>
FlatMultimap<K, V>::FlatMultimap()
    : data_(),
    isDirty_(false)
{
}

template <typename K, typename V>
void FlatMultimap<K, V>::insert(K key, V value)
{
    data_.push_back(KeyValue{std::move(key), std::move(value)});
    isDirty_ = true;
}

template <typename K, typename V>
typename FlatMultimap<K, V>::ValueRange FlatMultimap<K, V>::find(const K &key)
{
    sortAndPrune();

    using std::cbegin;
    using std::cend;
    const auto range = equal_range(cbegin(data_), cend(data_), key);
    return {range.first, range.second};
}

template <typename K, typename V>
void FlatMultimap<K, V>::reserve(int capacity)
{
    data_.reserve(capacity);
}

template <typename K, typename V>
void FlatMultimap<K, V>::shrink_to_fit()
{
    sortAndPrune();
    data_.shrink_to_fit();
}

template <typename K, typename V>
void FlatMultimap<K, V>::sortAndPrune()
{
    if (!isDirty_) {
        return;
    }

    using std::begin;
    using std::end;
    sort(begin(data_), end(data_));
    data_.erase(unique(begin(data_), end(data_)),
                end(data_));

    isDirty_ = false;
}

#endif
