
#if 1
#include "SearchContextImpl.hpp"
#include "iterators.hpp"

#include <algorithm>
#include <iterator>

#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/index/detail/rtree/utilities/statistics.hpp>
#include <boost/geometry/index/detail/rtree/utilities/print.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

BOOST_GEOMETRY_REGISTER_POINT_2D(Point, float, cs::cartesian, x, y);


class SearchContextBoostGeometry::Impl
{
public:
    Impl(const Point* points_begin, const Point* points_end);
    ~Impl();

    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    static const std::size_t max_capacity =  256;
    static const std::size_t min_capacity =  max_capacity / 2;

    typedef bg::model::point<float, 2, bg::cs::cartesian> point_t;
    typedef bg::model::box<point_t> box_t;

    std::vector<Point> results;

    static const size_t bucket_size = 10000;
    typedef bgi::rtree<Point, bgi::rstar<max_capacity, min_capacity>> rtree_t;
    std::vector<rtree_t> m_trees;
};

SearchContextBoostGeometry::Impl::Impl(const Point* points_begin, const Point* points_end)
{
    std::vector<Point> points(points_begin, points_end);
    if(points.size() == 0) { return; }

    std::sort(points.begin(), points.end(), [](const Point& p1, const Point& p2){ return p1.rank < p2.rank; });

    m_trees.reserve(points.size() / bucket_size + 1);

    auto startIt = points.begin();
    auto lastIt = startIt + std::min(bucket_size, static_cast<size_t>(points.end() - startIt));

    while(startIt != points.end())
    {
        m_trees.push_back(rtree_t(startIt, lastIt));

        startIt = lastIt;
        lastIt = startIt + std::min(bucket_size, static_cast<size_t>(points.end() - startIt));
    }
}

SearchContextBoostGeometry::Impl::~Impl()
{
}

int32_t SearchContextBoostGeometry::Impl::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    if(m_trees.size() == 0) { return 0; }

    box_t region(point_t(rect.lx, rect.ly), point_t(rect.hx, rect.hy));
    size_t num_results = 0;

    results.clear();
    results.reserve(count);
    
    for(auto it = m_trees.begin(); it != m_trees.end() && num_results < count; ++it)
    {
        num_results += it->query(bgi::covered_by(region), min_constrained_inserter(results));
    }

    std::sort(results.begin(), results.end(), [](const Point& p1, const Point& p2){ return p1 < p2; });

    for(auto& result : results)
    {
        *out_points = result;
        out_points++;
    }

    return static_cast<int32_t>(results.size());
}


//
//
//
SearchContextBoostGeometry::SearchContextBoostGeometry(const Point* points_begin, const Point* points_end) 
    : m_impl(new SearchContextBoostGeometry::Impl(points_begin, points_end))
{
}

SearchContextBoostGeometry::~SearchContextBoostGeometry() 
{
}

int32_t SearchContextBoostGeometry::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
#endif
