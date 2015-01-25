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
#include "RTree.hpp"
#include "iterators.hpp"

#include <chrono>
#include <iostream>

#include <windows.h>
#undef min
#undef max

//
//
//
class SearchContextRTree::Impl
{
public:
    Impl(const Point* points_begin, const Point* points_end);
    ~Impl();

    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    typedef RTree<Point, rstar<16>> rtree_t;

    static const size_t bucket_size = 24000;
    std::vector<std::unique_ptr<rtree_t>> m_trees;
    std::vector<Point> points;
    //std::vector<FastPoint> fastpoints;
    std::vector<Point> m_results;
};

SearchContextRTree::Impl::Impl(const Point* points_begin, const Point* points_end)
{
    //std::vector<Point> points;

    if(points_begin < points_end)
    {
        points.insert(points.begin(), points_begin, points_end);
        points.erase(std::remove_if(points.begin(), points.end(), [](const Point& p){ return (abs(p.x) > 1.0e9 || abs(p.y) > 1.0e9); }  ), points.end());
    }

    std::sort(points.begin(), points.end(), [](const Point& p1, const Point& p2){ return p1.rank < p2.rank; });

    m_trees.reserve(points.size() / bucket_size + 1);

    //fastpoints.insert(fastpoints.begin(), points.begin(), points.end());
/*////////
    LARGE_INTEGER frequency;
    LARGE_INTEGER t1,t2;
    double unaligned_elapsedTime;
    double aligned_elapsedTime;
    QueryPerformanceFrequency(&frequency);

    int dummy1=0;
    {
        QueryPerformanceCounter(&t1);
        for(auto& p : points)
        {
            dummy1 += p.rank;
        }
        QueryPerformanceCounter(&t2);
        unaligned_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
    }

    int dummy2=0;
    {
        QueryPerformanceCounter(&t1);
        int i = 0;
        for(auto& p : fastpoints)
        {
            dummy2 += points[i].rank;
            i++;
        }
        QueryPerformanceCounter(&t2);
        aligned_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
    }

    //std::cout << dummy1 << dummy2;

    //std::cout << std::endl << "Unaligned: " << unaligned_elapsedTime << "    Aligned: " << aligned_elapsedTime << std::endl;


*/////////
    auto beginIt = points.begin();
    auto endIt = points.end();

    auto startIt = beginIt;
    auto lastIt = startIt + std::min(bucket_size, static_cast<size_t>(endIt - startIt));

    while(startIt != endIt)
    {
        m_trees.emplace_back(new rtree_t(startIt, lastIt));
        startIt = lastIt != endIt ? lastIt : endIt;
        lastIt = startIt + std::min(bucket_size, static_cast<size_t>(endIt - startIt));
    }

}

SearchContextRTree::Impl::~Impl()
{
}

int32_t SearchContextRTree::Impl::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    m_results.clear();
    m_results.reserve(count);

    for(auto it = m_trees.begin(); it != m_trees.end() && m_results.size() < count; ++it)
    {
        (*it)->query(rect, min_constrained_inserter(m_results));
    }

    std::sort(m_results.begin(), m_results.end(), [](const Point& p1, const Point& p2){ return p1 < p2; });

    for(auto& result : m_results)
    {
        *out_points = result;
        out_points++;
    }

    return static_cast<int32_t>(m_results.size());
}

//
//
//
SearchContextRTree::SearchContextRTree(const Point* points_begin, const Point* points_end) 
    : m_impl(new SearchContextRTree::Impl(points_begin, points_end))
{
}

SearchContextRTree::~SearchContextRTree() 
{
}

int32_t SearchContextRTree::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
