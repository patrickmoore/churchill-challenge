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
#include <iterator>
#include <assert.h>

#include "TaskStack.hpp"
#include "point_search.h"

#include <iostream>
#include <windows.h>
#undef min
#undef max

template <size_t MaxElements>
struct default_min_elements
{
    static const size_t raw_value = (MaxElements * 4) / 10;
    static const size_t value = 1 <= raw_value ? raw_value : 1;
};

template <size_t MaxElements, size_t MinElements = default_min_elements<MaxElements>::value>
struct rtree_parameters
{
    static const size_t max_elements = MaxElements;
    static const size_t min_elements = MinElements;

    static size_t get_max_elements() { return MaxElements; }
    static size_t get_min_elements() { return MinElements; }
};

template <typename Value, typename Parameters>
class RTree
{
public:
    typedef std::vector<int> indexer_type;

    template <typename Iterator>
    explicit RTree(const Iterator points_begin, const Iterator points_end, const Parameters& parameters = Parameters()) 
        : m_parameters(parameters), m_values_count(0), m_height(0)
    { 
        build(points_begin, points_end); 
    }

    template<typename OutIter>
    void query(const Rect& region, OutIter& out_it)
    {
        if(m_values_count == 0) { return; }

        if(!intersects(region, m_root.mbr)) { return; }

        /*
        LARGE_INTEGER frequency;
        LARGE_INTEGER t1,t2;
        double rec_elapsedTime;
        double it_elapsedTime;
        QueryPerformanceFrequency(&frequency);

        {
            QueryPerformanceCounter(&t1);
            query_recursive(region, out_it);
            out_it.clear();
            query_recursive(region, out_it);
            out_it.clear();
            query_recursive(region, out_it);
            out_it.clear();
            query_recursive(region, out_it);
            out_it.clear();
            query_recursive(region, out_it);
            out_it.clear();
            QueryPerformanceCounter(&t2);
            rec_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
        }

        {
            out_it.clear();
            QueryPerformanceCounter(&t1);
            query_iterative(region, out_it);
            out_it.clear();
            query_iterative(region, out_it);
            out_it.clear();
            query_iterative(region, out_it);
            out_it.clear();
            query_iterative(region, out_it);
            out_it.clear();
            query_iterative(region, out_it);
            out_it.clear();
            QueryPerformanceCounter(&t2);
            it_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
        }

        std::cout << std::endl << "rec: " << rec_elapsedTime << "    it: " << it_elapsedTime << std::endl;
        /**/

        // note: profiling shows iterative is ~.1.5-2.5 microsecs faster per query
        query_iterative(region, out_it);
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
    struct Node
    {
        Node() 
            : rank(std::numeric_limits<int32_t>::max())
        {
            initialize(mbr);
        }

        bool is_leaf() const { return !leaf.empty(); }

        int32_t rank;
        Rect mbr;

        std::vector<Node> nodes;
        std::vector<Value> leaf;
    };

    struct subtree_elements_counts
    {
        subtree_elements_counts(std::size_t max_count_, std::size_t min_count_) : max_count(max_count_), min_count(min_count_) {}
        std::size_t max_count;
        std::size_t min_count;
    };

    template <typename Iterator>
    void build(const Iterator points_begin, const Iterator points_end)
    {
        m_values_count = std::distance(points_begin, points_end);
        if(m_values_count == 0) { return; }

        // Used to index into the points vector. Its faster to sort the indexer at the expense of using more memory.
        indexer_type indexer(m_values_count);
        int increment = 0;
        std::generate(indexer.begin(), indexer.end(), [&increment]()->int { return increment++; } );

        for(auto it = points_begin; it != points_end; ++it)
        {
            extend_bounds(m_root.mbr, *it);
        }

        const auto elements_count = calculate_subtree_elements_counts(m_values_count, m_parameters, m_height);

        nodesToSearch.reserve(elements_count.max_count);

        generate_subtree(indexer.begin(), indexer.end(), points_begin, m_root.mbr, m_values_count, elements_count, m_root, m_parameters);

        sort_subtree(m_root, [](Node const& n1, Node const& n2) { return n1.rank < n2.rank; } );
    }

    template <typename EIt, typename IdxIt> inline static
    void generate_subtree(EIt first, EIt last, const IdxIt indexer, const Rect& super_mbr, std::size_t values_count, 
                            subtree_elements_counts const& subtree_counts, Node& subtree, Parameters const& parameters)
    {
        assert(static_cast<std::size_t>(std::distance(first, last)) == values_count);

        subtree.nodes.emplace_back();
        auto& n = subtree.nodes.back();

        if (subtree_counts.max_count <= 1)
        {
            assert(values_count <= parameters.get_max_elements());

            n.leaf.reserve(values_count);
            for(;first != last; ++first)
            {
                auto& e = indexer[*first];
                n.leaf.push_back(e);
                extend_bounds(n.mbr, e);
                if(n.rank > e.rank) { n.rank = e.rank; }
            }

            return;
        }

        auto next_subtree_counts = subtree_counts;
        next_subtree_counts.max_count /= parameters.get_max_elements();
        next_subtree_counts.min_count /= parameters.get_max_elements();

        auto nodes_count = calculate_nodes_count(values_count, subtree_counts);
        n.nodes.reserve(nodes_count);

        partition_subtree(first, last, indexer, super_mbr, values_count, subtree_counts, next_subtree_counts,
            n, parameters);
    }

    template <typename EIt, typename IdxIt> inline static
    void partition_subtree(EIt first, EIt last, const IdxIt& indexer, const Rect& super_mbr,
                           std::size_t values_count,
                           subtree_elements_counts const& subtree_counts,
                           subtree_elements_counts const& next_subtree_counts,
                           Node & elements, Parameters const& parameters)
    {
        assert(0 < std::distance(first, last) && static_cast<std::size_t>(std::distance(first, last)) == values_count);

        assert(subtree_counts.min_count <= values_count);

        if (values_count <= subtree_counts.max_count)
        {
            generate_subtree(first, last, indexer, super_mbr, values_count, next_subtree_counts, elements, parameters);
            extend_bounds(elements.mbr, elements.nodes.back().mbr);
            if(elements.rank > elements.nodes.back().rank) { elements.rank = elements.nodes.back().rank; }
            return;
        }
        
        auto median_count = calculate_median(values_count, subtree_counts);
        auto median = first + median_count;

        auto greatest_dim_index = get_longest_edge_dimension(super_mbr);

        auto first_med_mbr = super_mbr; 
        auto med_last_mbr = super_mbr;
        if(greatest_dim_index == 0)
        {
            std::nth_element(first, median, last, [&](int i1, int i2) { return indexer[i1].x < indexer[i2].x; });
            first_med_mbr.hx = median != first ? indexer[*median].x : indexer[*first].x;
            med_last_mbr.lx = indexer[*median].x;
        }
        else
        {
            std::nth_element(first, median, last, [&](int i1, int i2) { return indexer[i1].y < indexer[i2].y; });
            first_med_mbr.hy = median != first ? indexer[*median].y : indexer[*first].y;
            med_last_mbr.ly = indexer[*median].y;
        }
        
        partition_subtree(first, median, indexer, first_med_mbr,
                          median_count, subtree_counts, next_subtree_counts,
                          elements, parameters);
        partition_subtree(median, last, indexer, med_last_mbr,
                          values_count - median_count, subtree_counts, next_subtree_counts,
                          elements, parameters);
    }

    inline static
    std::size_t get_longest_edge_dimension(const Rect& r)
    {
        return r.hx - r.lx > r.hy - r.ly ? 0 : 1;
    }

   
    inline static
    subtree_elements_counts calculate_subtree_elements_counts(std::size_t elements_count, Parameters const& parameters, std::size_t& height)
    {
        subtree_elements_counts res(1, 1);
        height = 0;

        auto smax = parameters.get_max_elements();
        for ( ; smax < elements_count ; smax *= parameters.get_max_elements(), ++height )
            res.max_count = smax;

        res.min_count = parameters.get_min_elements() * (res.max_count / parameters.get_max_elements());

        return res;
    }

    inline static
    std::size_t calculate_nodes_count(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        auto n = count / subtree_counts.max_count;
        auto r = count % subtree_counts.max_count;

        if ( 0 < r && r < subtree_counts.min_count )
        {
            auto count_minus_min = count - subtree_counts.min_count;
            n = count_minus_min / subtree_counts.max_count;
            r = count_minus_min % subtree_counts.max_count;
            ++n;
        }

        if ( 0 < r )
            ++n;

        return n;
    }

    inline static
    std::size_t calculate_median(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        auto n = count / subtree_counts.max_count;
        auto r = count % subtree_counts.max_count;
        auto median_count = (n / 2) * subtree_counts.max_count;

        if ( 0 != r )
        {
            if ( subtree_counts.min_count <= r )
            {
                median_count = ((n+1)/2) * subtree_counts.max_count;
            }
            else 
            {
                auto count_minus_min = count - subtree_counts.min_count;
                n = count_minus_min / subtree_counts.max_count;
                r = count_minus_min % subtree_counts.max_count;
                if ( r == 0 )                               
                {
                    
                    median_count = ((n+1)/2) * subtree_counts.max_count;     
                }
                else
                {
                    if ( n == 0 )                                       
                        median_count = r;                              
                    else
                        median_count = ((n+2)/2) * subtree_counts.max_count;
                }
            }
        }

        return median_count;
    }

    template <typename Pred>
    void sort_subtree(Node& subtree, Pred& pred)
    {
        if(!subtree.is_leaf())
        {
            std::sort(subtree.nodes.begin(), subtree.nodes.end(), pred);
            for(auto& n : subtree.nodes)
            {
                sort_subtree(n, pred);
            }
        }
        else
        {
            std::sort(subtree.leaf.begin(), subtree.leaf.end());
        }
    }

    template<typename OutIter>
    void query_recursive(const Rect& region, OutIter& out_it)
    {
        recursive_search(m_root, region, out_it);
    }

    template<typename OutIter>
    void recursive_search(const Node& subtree_node, const Rect& region, OutIter& out_it)
    {
        for(const auto& node : subtree_node.nodes)
        {
            if(node.rank < out_it.get_max_rank())
            {
                if(intersects(region, node.mbr))
                {
                    if(contains(region, node.mbr))
                    {
                        add_leafs(node, region, out_it);
                    }
                    else if(node.is_leaf())
                    {
                        for(const auto& p : node.leaf)
                        {
                            if(within(region, p))
                            {
                                if(!out_it.can_add(p)) { break; }
                                *out_it = p;
                            }
                        }
                    }
                    else
                    {
                        recursive_search(node, region, out_it);
                    }
                }
            }
        }
    }

    template<typename OutIter>
    void add_leafs(const Node& subtree_node, const Rect& region, OutIter& out_it)
    {
        if(subtree_node.is_leaf())
        {
            auto& leaf = subtree_node.leaf;
            for(const auto& p : leaf)
            {
                if(!out_it.can_add(p)) { return; }
                *out_it = p;
            }

            return;
        }

        for(const auto& node : subtree_node.nodes)
        {
            if(node.rank < out_it.get_max_rank())
            {
                add_leafs(node, region, out_it);
            }
        }
    }

    template<typename OutIter>
    void query_iterative(const Rect& region, OutIter& out_it)
    {
        nodesToSearch.push_back(&m_root.nodes.front()); // TODO: fix this. one extra level due to generate_subtree algorithm and me not wanting to do a deep copy of the tree.

        while(!nodesToSearch.empty())
        {
            auto& subtree = *nodesToSearch.back();
            nodesToSearch.pop_back();

            auto& nodes = subtree.nodes;
            for(auto& node : nodes)
            {
                if(node.rank > out_it.get_max_rank()) { break; }

                const auto& mbr = node.mbr;
                if(intersects(region, mbr))
                {
                    if(contains(region, mbr))
                    {
                        const auto stackPos = nodesToSearch.size();

                        nodesToSearch.push_back(&node);

                        while(stackPos < nodesToSearch.size())
                        {
                            auto& contained_node = *nodesToSearch.back();
                            nodesToSearch.pop_back();

                            if(contained_node.is_leaf())
                            {
                                for(auto& n : contained_node.leaf)
                                {
                                    if(n.rank > out_it.get_max_rank()) { break; }
                                    *out_it = n;
                                }
                            }
                            else
                            {
                                for(auto& n : contained_node.nodes)
                                {
                                    if(node.rank > out_it.get_max_rank()) { break; }
                                    nodesToSearch.push_back(&n);
                                }
                            }
                        }
                    }
                    else if(node.is_leaf())
                    {
                        for(const auto& p : node.leaf)
                        {
                            if(p.rank > out_it.get_max_rank()) { break; }
                            if(within(region, p))
                            {
                                *out_it = p;
                            }
                        }
                    }
                    else
                    {
                        nodesToSearch.push_back(&node);
                    }
                }
            }
        }
    }

private:
    Node m_root;
    Parameters m_parameters;
    TaskStack<Node*> nodesToSearch; // gains .1ms over vector
    size_t m_values_count;
    size_t m_height;
};
