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

#include "point_search.h"
#include <vector>
#include <memory>


template<class T>
class SearchContextImpl
{
public:
    int32_t search(const Rect rect, const int32_t count, Point* out_points) 
    {
        return static_cast<T*>(this)->search_impl(rect, count, out_points);
    }
};

class SearchContextLinear : public SearchContextImpl<SearchContextLinear>
{
public:
    SearchContextLinear(const Point* points_begin, const Point* points_end);
    ~SearchContextLinear();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class SearchContextKdTree : public SearchContextImpl<SearchContextKdTree>
{
public:
    SearchContextKdTree(const Point* points_begin, const Point* points_end);
    ~SearchContextKdTree();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class SearchContextBoostGeometry : public SearchContextImpl<SearchContextBoostGeometry>
{
public:
    SearchContextBoostGeometry(const Point* points_begin, const Point* points_end);
    ~SearchContextBoostGeometry();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

