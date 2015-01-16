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

#include "SearchContextImpl.hpp"
#include <algorithm>

class SearchContextLinear::Impl
{
public:
    Impl(const Point* points_begin, const Point* points_end);
    ~Impl();

    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    std::vector<Point> points;
    std::vector<Point*> result_points;
};

SearchContextLinear::Impl::Impl(const Point* points_begin, const Point* points_end)
{
    if(points_begin < points_end)
    {
        points.insert(points.begin(), points_begin, points_end);
    }

    std::sort(points.begin(), points.end(), [](const Point& p1, const Point& p2) { return p1.rank < p2.rank; });
}

SearchContextLinear::Impl::~Impl()
{
}

int32_t SearchContextLinear::Impl::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    result_points.clear();
    result_points.reserve(count);

    if(points.size() == 0) { return 0; }

    for(auto& point : points)
    {
        if(contains(rect, point)) 
        {
            result_points.push_back(&point);
            if(result_points.size() >= count) { break; }
        }
    }

    std::sort(result_points.begin(), result_points.end(), [](const Point* p1, const Point* p2) { return p1->rank < p2->rank; });

    for(auto result : result_points)
    {
            *out_points = *result;
            out_points++;
    }

    return static_cast<int32_t>(result_points.size());
}


//
//
//
SearchContextLinear::SearchContextLinear(const Point* points_begin, const Point* points_end) 
    : m_impl(new SearchContextLinear::Impl(points_begin, points_end))
{
}

SearchContextLinear::~SearchContextLinear() 
{
}

int32_t SearchContextLinear::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
