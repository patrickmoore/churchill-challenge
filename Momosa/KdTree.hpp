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

#include <algorithm>
#include <vector>
#include <stack>
#include <iterator>
#include <assert.h>

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)

#include "point_search.h"

struct KdTask
{
    explicit KdTask(int node_index_) : node_index(node_index_) {}
    explicit KdTask(int node_index_, int first_, int last_, int parent_, int depth_, int dim_) : node_index(node_index_), first(first_), last(last_), parent(parent_), depth(depth_), dim(dim_) {}

    int node_index;
    int first;
    int last;
    int parent;
    int depth;
    int dim;
};

// KdPoint exists for aligned and cache friendly searching through buckets.
struct KdPoint
{
    float x;
    float y;

    KdPoint(const KdPoint& p) : x(p.x), y(p.y) {}
    KdPoint(const Point& p) : x(p.x), y(p.y) {}

    KdPoint& operator=(const KdPoint& p) 
    {
        x = p.x;
        y = p.y;

        return *this;
    }

    KdPoint& operator=(const Point& p) 
    {
        x = p.x;
        y = p.y;

        return *this;
    }

    bool within(const Rect& r) const 
    {
        return x >= r.lx && x <= r.hx && y >= r.ly && y <= r.hy;
    }
};


class KdNode
{
public:
    KdNode() 
    : parent(-1)
    , left(-1) 
    , right(-1) 
    {
        mbr.lx = std::numeric_limits<float>::max();
        mbr.ly = std::numeric_limits<float>::max(); 
        mbr.hx = std::numeric_limits<float>::lowest();
        mbr.hy = std::numeric_limits<float>::lowest();
    }

    bool is_leaf() { return !bucket.empty(); }

    std::vector<Point> bucket;
    std::vector<KdPoint> fast_bucket; // Note: needs to be indexed the same as bucket
    Rect mbr;
    int32_t parent;
    int32_t left;
    int32_t right;
};

class KdTree
{
public:
    // TODO: calculate optimum size based on point set. Current value seems to be the quickest for 10M points.
    static const int bucket_size = 128;
    static const int stack_size = 128;

    KdTree() {} 

    explicit KdTree(const std::vector<Point>::iterator points_begin, const std::vector<Point>::iterator points_end) 
    { 
        m_taskstack.reserve(stack_size);

        build(points_begin, points_end); 
    }

    void extendBounds(int index, std::vector<int>& indexer)
    {
        assert(index >= 0);

        if(index < 0) { return; }

        auto& node = m_nodes[index];
        assert(node.is_leaf());

        for(const auto& point : node.bucket)
        {
            if(node.mbr.lx > point.x) node.mbr.lx = point.x;
            if(node.mbr.hx < point.x) node.mbr.hx = point.x;
            if(node.mbr.ly > point.y) node.mbr.ly = point.y;
            if(node.mbr.hy < point.y) node.mbr.hy = point.y;
        }

        extendBounds(node.parent, node.mbr, indexer);
    }

    void extendBounds(int node_index, const Rect& mbr, std::vector<int>& indexer)
    {
        auto index = node_index;
        while(index >= 0)
        {
            auto& node = m_nodes[index];
            assert(!node.is_leaf());

            if(node.mbr.lx > mbr.lx) node.mbr.lx = mbr.lx;
            if(node.mbr.hx < mbr.hx) node.mbr.hx = mbr.hx;
            if(node.mbr.ly > mbr.ly) node.mbr.ly = mbr.ly;
            if(node.mbr.hy < mbr.hy) node.mbr.hy = mbr.hy;

            index = node.parent;
        }
    }

    void build(const std::vector<Point>::iterator points_begin, const std::vector<Point>::iterator points_end)
    {
        std::vector<Point> points(points_begin, points_end);

        const auto num_points = points.size();
        if(num_points == 0) { return; }

        // Used to index into the points vector. Its faster to sort the indexer at the expense of using more memory.
        std::vector<int> indexer(num_points);
        int increment = 0;
        std::generate(indexer.begin(), indexer.end(), [&increment]()->int { return increment++; } );

        auto num_leafs = num_points / bucket_size + 1;
        const auto node_count = next_pow_of_2(num_leafs) * 2;
        m_nodes.reserve(node_count);
        m_nodes.push_back(KdNode());

        m_taskstack.push_back(KdTask(0, 0, static_cast<int>(num_points), -1, 0, 0));

        while(!m_taskstack.empty())
        {
            auto task = m_taskstack.back();
            m_taskstack.pop_back();

            auto& node = m_nodes[task.node_index];
            node.parent = task.parent;
            
            if(task.last - task.first <= bucket_size)
            {
                auto items = task.last - task.first;
                assert(items > 0);

                node.bucket.reserve(items);
                node.fast_bucket.reserve(items);

                auto it_end = indexer.begin() + task.last;
                for(auto it = indexer.begin() + task.first; it != it_end; ++it)
                {
                    node.bucket.push_back(points[*it]);
                    node.fast_bucket.push_back(points[*it]);
                }

                extendBounds(task.node_index, indexer);
            }
            else
            {
                // todo: use mean instead of median as is described at http://infolab.stanford.edu/~nsample/pubs/samplehaines.pdf
                // Looks like it will speed up creation, and possibly speed up searching.
                const auto median = (task.first + task.last) / 2;
                if(task.dim == 0)
                {
                    std::nth_element(indexer.begin() + task.first, indexer.begin() + median, indexer.begin() + task.last, 
                        [&](int i1, int i2) { return points[i1].x < points[i2].x; });
                }
                else
                {
                    std::nth_element(indexer.begin() + task.first, indexer.begin() + median, indexer.begin() + task.last, 
                        [&](int i1, int i2) { return points[i1].y < points[i2].y; });
                }

                node.left = static_cast<int>(m_nodes.size());
                m_nodes.push_back(KdNode());

                node.right = static_cast<int>(m_nodes.size());
                m_nodes.push_back(KdNode());

                auto dim = (task.dim + 1) % 2;
                m_taskstack.push_back(KdTask(node.right, median, task.last, task.node_index, task.depth+1, dim));
                m_taskstack.push_back(KdTask(node.left, task.first, median, task.node_index, task.depth+1, dim));
            }
        }
    }

    template<typename OutIter>
    size_t query(const Rect& region, OutIter out_it)
    {
        if(m_nodes.size() == 0) { return 0; }

        int num_found = 0;

        m_taskstack.emplace_back((0));

        while(!m_taskstack.empty())
        {
            auto task = m_taskstack.back();
            m_taskstack.pop_back();

            auto& node = m_nodes[task.node_index];

            if(contains(region, node.mbr))
            {
                auto stackPos = m_taskstack.size();
                
                m_taskstack.emplace_back((task.node_index));

                while(stackPos < m_taskstack.size())
                {
                    auto task = m_taskstack.back();
                    m_taskstack.pop_back();

                    auto& contained_node = m_nodes[task.node_index];

                    if(contained_node.is_leaf())
                    {
                        for(const auto& p : contained_node.bucket)
                        {
                            *out_it = p;
                            *out_it++;
                            num_found++;
                        }
                    }
                    else
                    {
                        m_taskstack.emplace_back((contained_node.right));
                        m_taskstack.emplace_back((contained_node.left));
                    }
                }
            }
            else if(node.is_leaf())
            {
                int p_index = 0;
                for(const auto& p : node.fast_bucket)
                {
                    if(p.within(region)) // Hotspot
                    {
                        *out_it = node.bucket[p_index];
                        *out_it++;
                        num_found++;
                    }
                    ++p_index;
                }
            }
            else
            {
                if(intersects(region, m_nodes[node.right].mbr))
                {
                    m_taskstack.emplace_back((node.right));
                }
                if(intersects(region, m_nodes[node.left].mbr))
                {
                    m_taskstack.emplace_back((node.left));
                }
            }
        }

        return num_found;
    }

    size_t get_num_points()
    {
        size_t num_points = 0; 
        for(auto& n : m_nodes)
        {
            num_points += n.bucket.size();
        }

        return num_points;
    }

private:
    size_t next_pow_of_2(size_t i)
    {
        unsigned long index = 0;
        _BitScanReverse(&index, static_cast<unsigned long>(i-1));

        return (size_t(1) << (index + 1));
    }

private:
    std::vector<KdNode> m_nodes;
    std::vector<KdTask> m_taskstack;
};
