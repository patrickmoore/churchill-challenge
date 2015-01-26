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

    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    typedef RTree<Point, rstar<25>> rtree_t;

    static const size_t partition_size = 315000;
    std::vector<rtree_t> m_trees;
    //std::vector<Point> points;
    //std::vector<FastPoint> fastpoints;
    std::vector<Point> m_results;
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

    std::sort(points.begin(), points.end(), [](const Point& p1, const Point& p2){ return p1.rank < p2.rank; });

    initialize(mbr);
    for(auto& p : points)
    {
        extend_bounds(mbr, p);
    }

    m_trees.reserve(points.size() / partition_size + 1);

/*////////
    fastpoints.insert(fastpoints.begin(), points.begin(), points.end());
/*/////////*/

    auto beginIt = points.begin();
    auto endIt = points.end();

    auto startIt = beginIt;
    auto lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));

    while(startIt != endIt)
    {
        m_trees.push_back(rtree_t(startIt, lastIt));
        startIt = lastIt != endIt ? lastIt : endIt;
        lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));
    }

}

SearchContextRTree::Impl::~Impl()
{
}

int32_t SearchContextRTree::Impl::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    m_results.clear();
    m_results.reserve(count);

    if(!intersects(rect, mbr)) { return 0; }

    auto& reporter = min_constrained_inserter(m_results);
    for(auto& tree : m_trees)
    {
        tree.query(rect, reporter);
        if(m_results.size() >= count) { break; }
    }

    std::sort(m_results.begin(), m_results.end());

    memcpy(out_points, m_results.data(), sizeof(Point)*m_results.size());

/*/////
    LARGE_INTEGER frequency;
    LARGE_INTEGER t1,t2;
    double unaligned_elapsedTime;
    double aligned_elapsedTime;
    QueryPerformanceFrequency(&frequency);

    Rect r = { -10000.0, -10000., 10000., 10000.};
    bool d1, d2;
    Sleep(5);
    int dummy2=0;
    {
        QueryPerformanceCounter(&t1);
        for(auto& p : fastpoints)
        {
            for(int i=0;i<100;i++)
            d2=contains(r, p);
        }
        QueryPerformanceCounter(&t2);
        aligned_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
    }

    Sleep(5);

    int dummy1=0;
    {
        QueryPerformanceCounter(&t1);
        for(auto& p : points)
        {
            for(int i=0;i<100;i++)
            d1 = contains(r, p);
        }
        QueryPerformanceCounter(&t2);
        unaligned_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
    }


    std::cout << d1 << d2;

    std::cout << std::endl << "Unaligned: " << unaligned_elapsedTime << "    Aligned: " << aligned_elapsedTime << std::endl;


/*/////////*/

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
