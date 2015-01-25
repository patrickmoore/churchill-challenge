/*
 * Copyright (c) 2015 Patrick Moore
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/* Given 10 million uniquely ranked points on a 2D plane, design a datastructure and an algorithm that can find the 20
most important points inside any given rectangle. The solution has to be reasonably fast even in the worst case, while
also not using an unreasonably large amount of memory.
Create an x64 Windows DLL, that can be loaded by the test application. The DLL and the functions will be loaded
dynamically. The DLL should export the following functions: "create", "search" and "destroy". The signatures and
descriptions of these functions are given below. You can use any language or compiler, as long as the resulting DLL
implements this interface. */

/* This standard header defines the sized types used. */
#include <stdint.h>

/* The following structs are packed with no padding. */
#pragma pack(push, 1)

/* Defines a point in 2D space with some additional attributes like id and rank. */
struct Point
{
    int8_t id;
    int32_t rank;

    float x;
    float y;
};

/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
    float lx;
    float ly;
    float hx;
    float hy;
};
#pragma pack(pop)

template <typename OtherPoint>
inline bool operator<(const Point& lhs, const OtherPoint& rhs) { return lhs.rank < rhs.rank; }
template <typename OtherPoint>
inline bool operator<=(const Point& lhs, const OtherPoint& rhs) { return lhs.rank <= rhs.rank; }
template <typename OtherPoint>
inline bool operator>(const Point& lhs, const OtherPoint& rhs) { return lhs.rank > rhs.rank; }
template <typename OtherPoint>
inline bool operator>=(const Point& lhs, const OtherPoint& rhs) { return lhs.rank >= rhs.rank; }


inline bool intersects(const Rect& a, const Rect& b)
{
    return a.lx <= b.hx && a.hx >= b.lx && a.ly <= b.hy && a.hy >= b.ly;
}

// Rect A contains Rect B
inline bool contains(const Rect& a, const Rect& b)
{
    return a.lx <= b.lx && a.ly <= b.ly && a.hx >= b.hx && a.hy >= b.hy;
}

// Rect A contains Point B
template<typename Point>
inline bool contains(const Rect& a, const Point& b)
{
    return a.lx <= b.x && a.hx >= b.x && a.ly <= b.y && a.hy >= b.y;
}

template<typename Point>
inline bool within(const Rect& a, const Point& b)
{
    return b.x >= a.lx && b.x <= a.hx && b.y >= a.ly && b.y <= a.hy;
}

struct FastPoint
{
    float x;
    float y;
    int rank;
    //Point* packed_point;

    FastPoint(const FastPoint& p) : x(p.x), y(p.y), rank(p.rank)/*, packed_point(p.packed_point)*/ {}
    FastPoint(Point& p) : x(p.x), y(p.y), rank(p.rank) /*, packed_point(&p)*/ {}

    FastPoint& operator=(const FastPoint& p) 
    {
        x = p.x;
        y = p.y;
        rank = p.rank;
        //packed_point = p.packed_point;

        return *this;
    }

    FastPoint& operator=(Point& p) 
    {
        x = p.x;
        y = p.y;
        rank = p.rank;
        //packed_point = &p;

        return *this;
    }

    bool within(const Rect& r) const 
    {
        return x >= r.lx && x <= r.hx && y >= r.ly && y <= r.hy;
    }
};

template <typename OtherPoint>
inline bool operator<(const FastPoint& lhs, const OtherPoint& rhs) { return lhs.rank < rhs.rank; }
template <typename OtherPoint>
inline bool operator<=(const FastPoint& lhs, const OtherPoint& rhs) { return lhs.rank <= rhs.rank; }
template <typename OtherPoint>
inline bool operator>(const FastPoint& lhs, const OtherPoint& rhs) { return lhs.rank > rhs.rank; }
template <typename OtherPoint>
inline bool operator>=(const FastPoint& lhs, const OtherPoint& rhs) { return lhs.rank >= rhs.rank; }

/* Declaration of the struct that is used as the context for the calls. */
class SearchContext;

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
//typedef SearchContext* (__stdcall* T_create)(const Point* points_begin, const Point* points_end);

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
//typedef int32_t (__stdcall* T_search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
//typedef SearchContext* (__stdcall* T_destroy)(SearchContext* sc);
