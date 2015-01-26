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

template <size_t MaxElements>
struct default_min_elements_s
{
    static const size_t raw_value = (MaxElements * 4) / 10;
    static const size_t value = 1 <= raw_value ? raw_value : 1;
};

template <size_t MaxElements, size_t MinElements = default_min_elements_s<MaxElements>::value>
struct rstar
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
    typedef Parameters parameters_type;
    typedef std::vector<int> indexer_type;

    template <typename Iterator>
    explicit RTree(const Iterator points_begin, const Iterator points_end, const parameters_type& parameters = parameters_type()) 
        : m_parameters(parameters), m_values_count(0), m_height(0)
    { 
        build(points_begin, points_end); 
    }

    template<typename OutIter>
    void query(const Rect& region, OutIter out_it)
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
            query_recursive(region, out_it);
            query_recursive(region, out_it);
            query_recursive(region, out_it);
            query_recursive(region, out_it);
            QueryPerformanceCounter(&t2);
            rec_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
        }

        {
            out_it.clear();
            QueryPerformanceCounter(&t1);
            query_iterative(region, out_it);
            query_iterative(region, out_it);
            query_iterative(region, out_it);
            query_iterative(region, out_it);
            query_iterative(region, out_it);
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

        // Wastes space and isn't sexy, but avoids virtual calls / pointer dereference
        std::vector<Node> nodes;
        std::vector<Value> leaf;

    };

    struct subtree_elements_counts
    {
        subtree_elements_counts(std::size_t ma, std::size_t mi) : maxc(ma), minc(mi) {}
        std::size_t maxc;
        std::size_t minc;
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

        Rect mbr;
        initialize(mbr);
        for(auto it = points_begin; it != points_end; ++it)
        {
            extend_bounds(mbr, *it);
        }

        m_root.mbr = mbr;

        const auto elements_count = calculate_subtree_elements_counts(m_values_count, m_parameters, m_height);

        nodesToSearch.reserve(elements_count.maxc);

        per_level(indexer.begin(), indexer.end(), points_begin, mbr, m_values_count, elements_count, m_parameters, m_root);
        sort_subtree(m_root, [](Node const& n1, Node const& n2) { return n1.rank < n2.rank; } );
    }

    template <typename EIt, typename IdxIt> inline static
    void per_level(EIt first, EIt last, const IdxIt indexer, const Rect& hint_mbr, std::size_t values_count, subtree_elements_counts const& subtree_counts,
                               parameters_type const& parameters, Node& subtree)
    {
        assert(0 < std::distance(first, last) && static_cast<std::size_t>(std::distance(first, last)) == values_count);

        subtree.nodes.emplace_back();
        auto& n = subtree.nodes.back();

        if ( subtree_counts.maxc <= 1 )
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
        next_subtree_counts.maxc /= parameters.get_max_elements();
        next_subtree_counts.minc /= parameters.get_max_elements();

        auto nodes_count = calculate_nodes_count(values_count, subtree_counts);
        n.nodes.reserve(nodes_count);

        per_level_packets(first, last, indexer, hint_mbr, values_count, subtree_counts, next_subtree_counts,
            n, parameters);
    }

    template <typename EIt, typename IdxIt> inline static
    void per_level_packets(EIt first, EIt last, const IdxIt& indexer, const Rect& hint_mbr,
                           std::size_t values_count,
                           subtree_elements_counts const& subtree_counts,
                           subtree_elements_counts const& next_subtree_counts,
                           Node & elements, parameters_type const& parameters)
    {
        assert(0 < std::distance(first, last) && static_cast<std::size_t>(std::distance(first, last)) == values_count);

        assert( subtree_counts.minc <= values_count);

        if ( values_count <= subtree_counts.maxc )
        {
            per_level(first, last, indexer, hint_mbr, values_count, next_subtree_counts, parameters, elements);
            extend_bounds(elements.mbr, elements.nodes.back().mbr);
            if(elements.rank > elements.nodes.back().rank) { elements.rank = elements.nodes.back().rank; }
            return;
        }
        
        auto median_count = calculate_median_count(values_count, subtree_counts);
        auto median = first + median_count;

        auto greatest_dim_index = get_longest_edge_dimension(hint_mbr);

        auto left_mbr = hint_mbr; 
        auto right_mbr = hint_mbr;
        if(greatest_dim_index == 0)
        {
            std::nth_element(first, median, last, [&](int i1, int i2) { return indexer[i1].x < indexer[i2].x; });
            left_mbr.hx = indexer[*median].x;
            right_mbr.lx = left_mbr.hx;
        }
        else
        {
            std::nth_element(first, median, last, [&](int i1, int i2) { return indexer[i1].y < indexer[i2].y; });
            left_mbr.hy = indexer[*median].y;
            right_mbr.ly = left_mbr.hy;
        }
        
        per_level_packets(first, median, indexer, left_mbr,
                          median_count, subtree_counts, next_subtree_counts,
                          elements, parameters);
        per_level_packets(median, last, indexer, right_mbr,
                          values_count - median_count, subtree_counts, next_subtree_counts,
                          elements, parameters);
    }

    inline static
    std::size_t get_longest_edge_dimension(const Rect& r)
    {
        return r.hx - r.lx > r.hy - r.ly ? 0 : 1;
    }

   
    inline static
    subtree_elements_counts calculate_subtree_elements_counts(std::size_t elements_count, parameters_type const& parameters, std::size_t& height)
    {
        subtree_elements_counts res(1, 1);
        height = 0;

        auto smax = parameters.get_max_elements();
        for ( ; smax < elements_count ; smax *= parameters.get_max_elements(), ++height )
            res.maxc = smax;

        res.minc = parameters.get_min_elements() * (res.maxc / parameters.get_max_elements());

        return res;
    }

    inline static
    std::size_t calculate_nodes_count(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        auto n = count / subtree_counts.maxc;
        auto r = count % subtree_counts.maxc;

        if ( 0 < r && r < subtree_counts.minc )
        {
            auto count_minus_min = count - subtree_counts.minc;
            n = count_minus_min / subtree_counts.maxc;
            r = count_minus_min % subtree_counts.maxc;
            ++n;
        }

        if ( 0 < r )
            ++n;

        return n;
    }

    // Taken from Boost.Geometry
    inline static
    std::size_t calculate_median_count(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        // e.g. for max = 5, min = 2, count = 52, subtree_max = 25, subtree_min = 10

        auto n = count / subtree_counts.maxc; // e.g. 52 / 25 = 2
        auto r = count % subtree_counts.maxc; // e.g. 52 % 25 = 2
        auto median_count = (n / 2) * subtree_counts.maxc; // e.g. 2 / 2 * 25 = 25

        if ( 0 != r ) // e.g. 0 != 2
        {
            if ( subtree_counts.minc <= r ) // e.g. 10 <= 2 == false
            {
                //BOOST_ASSERT_MSG(0 < n, "unexpected value");
                median_count = ((n+1)/2) * subtree_counts.maxc; // if calculated ((2+1)/2) * 25 which would be ok, but not in all cases
            }
            else // r < subtree_counts.second  // e.g. 2 < 10 == true
            {
                auto count_minus_min = count - subtree_counts.minc; // e.g. 52 - 10 = 42
                n = count_minus_min / subtree_counts.maxc; // e.g. 42 / 25 = 1
                r = count_minus_min % subtree_counts.maxc; // e.g. 42 % 25 = 17
                if ( r == 0 )                               // e.g. false
                {
                    // n can't be equal to 0 because then there wouldn't be any element in the other node
                    //BOOST_ASSERT_MSG(0 < n, "unexpected value");
                    median_count = ((n+1)/2) * subtree_counts.maxc;     // if calculated ((1+1)/2) * 25 which would be ok, but not in all cases
                }
                else
                {
                    if ( n == 0 )                                        // e.g. false
                        median_count = r;                                // if calculated -> 17 which is wrong!
                    else
                        median_count = ((n+2)/2) * subtree_counts.maxc; // e.g. ((1+2)/2) * 25 = 25
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
    void query_recursive(const Rect& region, OutIter out_it)
    {
        recursive_search(m_root, region, out_it);
    }

    template<typename OutIter>
    void recursive_search(const Node& subtree_node, const Rect& region, OutIter out_it)
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
    void add_leafs(const Node& subtree_node, const Rect& region, OutIter out_it)
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
    void query_iterative(const Rect& region, OutIter out_it)
    {
        nodesToSearch.push_back(&m_root);

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
    parameters_type m_parameters;
    std::vector<Node*> nodesToSearch;
    size_t m_values_count;
    size_t m_height;
};
