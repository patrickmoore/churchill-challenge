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

//
//
//
class SearchContextRTree::Impl
{
public:
    Impl(const Point* points_begin, const Point* points_end);
    ~Impl();

    int32_t search_impl(const Rect& rect, const int32_t count, Point* out_points);

private:
    typedef RTree<Point, rtree_parameters<25>> rtree_t;

    static const size_t partition_size = 315000;
    std::vector<rtree_t> m_trees;
    std::vector<Point> m_results;
    std::vector<Point> m_y_sorted;
    std::vector<Point> m_x_sorted;
    Rect mbr;
};

SearchContextRTree::Impl::Impl(const Point* points_begin, const Point* points_end)
{
    std::vector<Point> points;

    if(points_begin < points_end)
    {
        points.insert(points.begin(), points_begin, points_end);
        points.erase(std::remove_if(points.begin(), points.end(), [](const Point& p){ return (abs(p.x) > 1.0e9 || abs(p.y) > 1.0e9); }  ), points.end());
    }

    m_x_sorted = points;
    m_y_sorted = points;

    std::sort(points.begin(), points.end());

    std::sort(m_x_sorted.begin(), m_x_sorted.end(), [](Point const& p1, Point const& p2) { return p1.x < p2.x; } );
    std::sort(m_y_sorted.begin(), m_y_sorted.end(), [](Point const& p1, Point const& p2) { return p1.y < p2.y; } );

    initialize(mbr);
    for(auto& p : points)
    {
        extend_bounds(mbr, p);
    }

    m_trees.reserve(points.size() / partition_size + 1);

    auto beginIt = points.begin();
    auto endIt = points.end();

    auto startIt = beginIt;
    auto lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));

    while(startIt != endIt)
    {
        m_trees.emplace_back(startIt, lastIt);

        startIt = lastIt != endIt ? lastIt : endIt;
        lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));
    }

}

SearchContextRTree::Impl::~Impl()
{
}

//#define PROFILE 1

#if defined(PROFILE)
static double max_elapsed = 0.0;
static int maxed_iteration = 0;
Rect max_rect;
static int maxed_returned_results = 0;
#endif

int32_t SearchContextRTree::Impl::search_impl(const Rect& rect, const int32_t count, Point* out_points)
{
    m_results.clear();
    m_results.reserve(count);

    if(!intersects(rect, mbr)) { return 0; }

#if defined(PROFILE)
    LARGE_INTEGER frequency;
    LARGE_INTEGER t1,t2;
    double elapsedTime;
    QueryPerformanceFrequency(&frequency);

    QueryPerformanceCounter(&t1);
#endif
    int iteration = 0;

    auto& reporter = min_constrained_inserter(m_results);

    if(rect.hx - rect.lx < .0001)
    {
        auto compare = [](Point const& p, float v) { return p.x < v; };
        auto first = std::lower_bound(m_x_sorted.begin(), m_x_sorted.end(), rect.lx, compare);
        for(; first != m_x_sorted.end() && first->x < rect.hx; ++first)
        {
            auto py = first->y;
            if(py >= rect.ly && py <= rect.hy)
            {
                *reporter = *first;
            }
        }
    }
    else if(rect.hy - rect.ly < .0001)
    {
        auto compare = [](Point const& p, float v) { return p.y < v; };
        auto first = std::lower_bound(m_y_sorted.begin(), m_y_sorted.end(), rect.ly, compare);
        for(; first != m_y_sorted.end() && first->y < rect.hy; ++first)
        {
            auto px = first->x;
            if(px >= rect.lx && px <= rect.hx)
            {
                *reporter = *first;
            }
        }
    }
    else
    {
        for(auto& tree : m_trees)
        {
            tree.query(rect, reporter);
            iteration++;
            if(m_results.size() >= count) { break; }
        }
    }

#if defined(PROFILE)
    QueryPerformanceCounter(&t2);
    elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart * 1000;
    if(elapsedTime > max_elapsed)
    {
        max_elapsed = elapsedTime;
        maxed_iteration = iteration-1;
        max_rect = rect;
        maxed_returned_results = static_cast<int>(m_results.size());
    }

    std::cout.precision(12);
    std::cout << "it: " << elapsedTime << "     max: " << max_elapsed << std::endl;
    std::cout << "    iteration: " << maxed_iteration << "    rect: " << max_rect.lx << " " << max_rect.hx << " " <<  max_rect.ly << " " << max_rect.hy << std::endl;
    std::cout << "    results: " << maxed_returned_results << std::endl;
#endif

    std::sort(m_results.begin(), m_results.end());

    memcpy(out_points, m_results.data(), sizeof(Point)*m_results.size());

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

int32_t SearchContextRTree::search_impl(const Rect& rect, const int32_t count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
