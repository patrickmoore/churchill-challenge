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

#include <limits>
#include "point_search.h"

template<typename Point> inline bool operator<(Point const& lhs, Point const& rhs) { return lhs.rank < rhs.rank; }
template<typename Point> inline bool operator>(Point const& lhs, Point const& rhs) { return lhs.rank > rhs.rank; }
template<typename Point> inline bool operator<=(Point const& lhs, Point const& rhs) { return lhs.rank <= rhs.rank; }
template<typename Point> inline bool operator>=(Point const& lhs, Point const& rhs) { return lhs.rank >= rhs.rank; }

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
template<typename Point> inline bool contains(const Rect& a, const Point& b)
{
    return a.lx <= b.x && a.hx >= b.x && a.ly <= b.y && a.hy >= b.y;
}

template<std::size_t I> inline 
bool within(const Rect& a, const Rect& b)
{
    return get_dim_coord_lo<I>(a) <= get_dim_coord_lo<I>(b) && get_dim_coord_hi<I>(a) >= get_dim_coord_hi<I>(b);
}

template<std::size_t I, typename Point> inline 
bool within(const Rect& a, const Point& b)
{
    return get_dim_coord_lo<I>(a) <= get_dim_coord<I>(b) && get_dim_coord_hi<I>(a) >= get_dim_coord<I>(b);
}

static inline void initialize(Rect& r)
{
    r.lx = std::numeric_limits<float>::max();
    r.ly = std::numeric_limits<float>::max(); 
    r.hx = std::numeric_limits<float>::lowest();
    r.hy = std::numeric_limits<float>::lowest();
}

template<typename Point>
inline void extend_bounds(Rect& r, const Point& point)
{
    if(r.lx > point.x) r.lx = point.x;
    if(r.hx < point.x) r.hx = point.x;
    if(r.ly > point.y) r.ly = point.y;
    if(r.hy < point.y) r.hy = point.y;
}

inline void extend_bounds(Rect& a, const Rect& b)
{
    if(a.lx > b.lx) a.lx = b.lx;
    if(a.hx < b.hx) a.hx = b.hx;
    if(a.ly > b.ly) a.ly = b.ly;
    if(a.hy < b.hy) a.hy = b.hy;
}

inline std::size_t get_longest_edge(const Rect& r)
{
    return r.hx - r.lx > r.hy - r.ly ? 0 : 1;
}

template<std::size_t I> inline typename std::enable_if<I == 0, float>::type get_dim_coord_lo(Rect const& r) { return r.lx; }
template<std::size_t I> inline typename std::enable_if<I == 0, float&>::type get_dim_coord_lo(Rect& r) { return r.lx; }

template<std::size_t I> inline typename std::enable_if<I == 1, float>::type get_dim_coord_lo(Rect const& r) { return r.ly; }
template<std::size_t I> inline typename std::enable_if<I == 1, float&>::type get_dim_coord_lo(Rect& r) { return r.ly; }

template<std::size_t I> inline typename std::enable_if<I == 0, float>::type get_dim_coord_hi(Rect const& r) { return r.hx; }
template<std::size_t I> inline typename std::enable_if<I == 0, float&>::type get_dim_coord_hi(Rect& r) { return r.hx; }

template<std::size_t I> inline typename std::enable_if<I == 1, float>::type get_dim_coord_hi(Rect const& r) { return r.hy; }
template<std::size_t I> inline typename std::enable_if<I == 1, float&>::type get_dim_coord_hi(Rect& r) { return r.hy; }

template<std::size_t I, typename Point> inline typename std::enable_if<I == 0, float>::type get_dim_coord(Point const& p) { return static_cast<float>(p.x); }
template<std::size_t I, typename Point> inline typename std::enable_if<I == 1, float>::type get_dim_coord(Point const& p) { return static_cast<float>(p.y); }