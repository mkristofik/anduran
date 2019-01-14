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
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "FlatMultimap.h"
#include "rapidjson/document.h"

#include <string>
#include <type_traits>
#include <vector>

// Fetch a JSON integer array and force all values to be type T.
template <typename T, size_t N>
void jsonGetArray(rapidjson::Value &obj, const char (&name)[N], std::vector<T> &outVec);

// Cast all elements of the given container to type T before inserting into a JSON
// array.  Add the array to the document.
template <typename T, size_t N, typename C>
void jsonSetArray(rapidjson::Document &doc, const char (&name)[N], const C &cont);

// Fetch a nested JSON object containing arrays. Insert the elements into a
// multimap using the array names as the keys.
template <typename T, size_t N>
void jsonGetMultimap(rapidjson::Value &obj,
                     const char (&name)[N],
                     FlatMultimap<std::string, T> &outMap);

// Store a FlatMultimap as a JSON object with one array for each key.  Add this
// object to the document.
template <typename T, size_t N>
void jsonSetMultimap(rapidjson::Document &doc,
                     const char (&name)[N],
                     FlatMultimap<std::string, T> &srcMap);

// Helper functions for reading and writing JSON files.
rapidjson::Document jsonReadFile(const char *filename);
void jsonWriteFile(const char *filename, const rapidjson::Document &doc);


template <typename T, size_t N>
void jsonGetArray(rapidjson::Value &obj, const char (&name)[N], std::vector<T> &outVec)
{
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>,
                  "Only supports fetching arrays with integer type");

    if (!obj.HasMember(name)) {
        return;
    }

    const rapidjson::Value &jsonArray = obj[name];
    outVec.reserve(jsonArray.Size());
    for (const auto &v : jsonArray.GetArray()) {
        outVec.push_back(static_cast<T>(v.GetInt()));
    }
}

template <typename T, size_t N, typename C>
void jsonSetArray(rapidjson::Document &doc, const char (&name)[N], const C &cont)
{
    using namespace rapidjson;
    using std::size;

    Value ary(kArrayType);
    Document::AllocatorType &alloc = doc.GetAllocator();
    ary.Reserve(size(cont), alloc);
    for (const auto &val : cont) {
        ary.PushBack(static_cast<T>(val), alloc);
    }

    doc.AddMember(Value(name), ary, alloc);
}

template <typename T, size_t N>
void jsonGetMultimap(rapidjson::Value &obj,
                     const char (&name)[N],
                     FlatMultimap<std::string, T> &outMap)
{
    static_assert(std::is_integral_v<T>,
                  "Only supports de-serializing maps of string to integer type");

    if (!obj.HasMember(name)) {
        return;
    }

    rapidjson::Value &subobj = obj[name];
    for (auto m = subobj.MemberBegin(); m != subobj.MemberEnd(); ++m) {
        const char *aryName = m->name.GetString();
        for (const auto &v : subobj[aryName].GetArray()) {
            outMap.insert(aryName, v.GetInt());
        }
    }
}

template <typename T, size_t N>
void jsonSetMultimap(rapidjson::Document &doc,
                     const char (&name)[N],
                     FlatMultimap<std::string, T> &srcMap)
{
    using namespace rapidjson;

    Value objs(kObjectType);
    Document::AllocatorType &alloc = doc.GetAllocator();

    Value ary(kArrayType);
    std::string curObject;
    for (const auto &o : srcMap) {
        if (o.key != curObject) {
            if (!ary.Empty()) {
                objs.AddMember(Value(curObject.c_str(), alloc), ary, alloc);
                // AddMember() appears to clear 'ary' so we need to reset it.
                ary.SetArray();
            }
            curObject = o.key;
        }
        ary.PushBack(o.value, alloc);
    }

    if (!ary.Empty()) {
        objs.AddMember(Value(curObject.c_str(), alloc), ary, alloc);
    }

    doc.AddMember(Value(name), objs, alloc);
}

#endif
