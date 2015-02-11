
#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <assert.h>

#include "point_util.h"

#include <intrin.h>
 #pragma intrinsic(_mm_cvt_ss2si)
 #pragma intrinsic(_mm_set_sd)

inline
int fround(float val)
{
    return _mm_cvt_ss2si(_mm_set_ss(val));
}

inline
float minss (float a, float b)
{
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a),_mm_set_ss(b)));
    return a;
}

inline
float maxss ( float a, float b )
{
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a),_mm_set_ss(b)));
    return a;
}

inline
float clamp (float val, float minval, float maxval)
{
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(minval)), _mm_set_ss(maxval)));
    return val;
}

class HashGridSpatialIndex
{
public:
    template <typename Iterator>
    HashGridSpatialIndex(Iterator first, Iterator last, Rect const& mbr)
        : m_num_bins(0)
        , m_num_entries(0)
        , m_num_objects(0)
    {
        extend_mbr(first, last, m_hashgrid);

        partition_hashgrid(first, last, m_hashgrid, 1);

        sort_bin(m_hashgrid);
    }

    template<typename OutIter>
    void query(Rect const& region, OutIter out)
    {
        search(m_hashgrid, region, out);
    }

private:
    struct Bin
    {
        Bin() 
            : rank(std::numeric_limits<int32_t>::max())
        {
            initialize(mbr);
        }

        bool is_leaf() const { return !leaf.empty(); }

        int32_t rank;
        Rect mbr;

        std::unordered_map<int64_t, Bin> nodes;
        std::vector<Point> leaf;
    };


private:
    template <typename Iterator>
    void extend_mbr(Iterator first, Iterator last, Bin& hashgrid)
    {
        for(;first != last; ++first)
        {
            extend_bounds(hashgrid.mbr, *first);
        }
    }

    template <typename Iterator>
    void partition_hashgrid(Iterator first, Iterator last, Bin& bin, int height)
    {
        if(std::distance(first, last) < max_bin_size) { return; }

        const auto& mbr = bin.mbr;

        for(; first != last; ++first)
        {
            const auto& point = *first;
            const auto x_key = fround((point.x - mbr.lx) * (num_bins / (mbr.hx - mbr.lx)));
            const auto y_key = fround((point.y - mbr.ly) * (num_bins / (mbr.hy - mbr.ly)));
            assert(x_key >= 0 && y_key >= 0);

            const auto key = generate_key(x_key, y_key);

            auto& b = bin.nodes[key];
            b.leaf.push_back(point);
            extend_bounds(b.mbr, point);
            bin.rank = std::min(bin.rank, point.rank);
        }

        if(height > max_height) { return; }

        for(auto& n : bin.nodes)
        {
            auto& b = n.second;
            if(b.leaf.size() > max_bin_size)
            {
                partition_hashgrid(b.leaf.begin(), b.leaf.end(), b, height + 1);
                b.leaf.clear();
                b.leaf.shrink_to_fit();
            }
        }
    }

    void sort_bin(Bin& hashgrid)
    {
        if(hashgrid.is_leaf())
        {
            concurrency::parallel_sort(hashgrid.leaf.begin(), hashgrid.leaf.end());
            return;
        }

        for(auto& b : hashgrid.nodes)
        {
            sort_bin(b.second);
        }
    }

    template<typename OutIter> inline static
    void search(Bin const& hashgrid, Rect const& region, OutIter out)
    {
        if(hashgrid.nodes.empty() && hashgrid.leaf.empty()) { return; }

        const auto& mbr = hashgrid.mbr;

        if(!intersects(mbr, region)) { return; }

        if(hashgrid.is_leaf())
        {
            for (auto& p : hashgrid.leaf)
            {
                if (p.rank > out.get_max_rank()) { return; }
                if (contains(region, p))
                {
                    *out = p;
                }
            }

            return;
        }

        const auto range_x = mbr.hx - mbr.lx;
        const auto range_y = mbr.hy - mbr.ly;

        const auto min_x = clamp((region.lx - mbr.lx) / range_x, 0.0, 1.0) * num_bins;
        const auto max_x = clamp((region.hx - mbr.lx) / range_x, 0.0, 1.0) * num_bins;
        const auto min_y = clamp((region.ly - mbr.ly) / range_y, 0.0, 1.0) * num_bins;
        const auto max_y = clamp((region.hy - mbr.ly) / range_y, 0.0, 1.0) * num_bins;

        if(min_x == max_x || min_y == max_y) { return; }

        const auto min_x_key = fround(min_x);
        const auto max_x_key = fround(max_x);
        const auto min_y_key = fround(min_y);
        const auto max_y_key = fround(max_y);

        // TODO: order points in one big array with indexes to the points in bins. Get
        // indexes to the beginning / end of the x and y and iterate through the big array
        for(auto x_key = min_x_key; x_key <= max_x_key; ++x_key)
        {
            for(auto y_key = min_y_key; y_key <= max_y_key; ++y_key)
            {
                const auto key = generate_key(x_key, y_key);

                const auto it = hashgrid.nodes.find(key);
                if(it != hashgrid.nodes.end())
                {
                    auto& b = it->second;
                    if(b.rank > out.get_max_rank()) { break; }

                    search(b, region, out); // TODO: make this iterative
                }
            }
        }
    }


    inline static
    int64_t generate_key(int64_t key1, int64_t key2)
    {
        return (key2 << 32) | ((key1 & 0xFFFF) << 16) | ((key1 & 0xFFFF) >> 16);
    }

private:
    static const int num_bins = 100;
    static const int max_bin_size = 20000;
    static const int max_height = 1;

    Bin m_hashgrid;

    std::size_t m_num_bins;
    std::size_t m_num_objects;
    std::size_t m_num_entries;
};


