
#pragma warning( disable : 4996 )

#include "SearchContextImpl.hpp"
#include "KdTree.hpp"
#include "iterators.hpp"

#include <future>


//
//
//
class SearchContextKdTree::Impl
{
public:
    Impl(const Point* points_begin, const Point* points_end);
    ~Impl();

    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    // TODO: calculate optimum size based on point set. Current value seems to be the quickest for 10M points.
    static const size_t bucket_size = 16383;
    std::vector<KdTree> m_trees;
    std::vector<Point> m_results;
};

SearchContextKdTree::Impl::Impl(const Point* points_begin, const Point* points_end)
{
    std::vector<Point> points;

    if(points_begin < points_end)
    {
        points.insert(points.begin(), points_begin, points_end);
    }

    std::sort(points.begin(), points.end(), [](const Point& p1, const Point& p2){ return p1.rank < p2.rank; });

    m_trees.reserve(points.size() / bucket_size + 1);

    auto startIt = points.begin();
    auto lastIt = startIt + std::min(bucket_size, static_cast<size_t>(points.end() - startIt));

    while(startIt != points.end())
    {
        m_trees.push_back(KdTree(startIt, lastIt));
        startIt = lastIt != points.end() ? lastIt : points.end();
        lastIt = startIt + std::min(bucket_size, static_cast<size_t>(points.end() - startIt));
    }

}

SearchContextKdTree::Impl::~Impl()
{
}

int32_t SearchContextKdTree::Impl::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    m_results.clear();
    m_results.reserve(count);

    for(auto it = m_trees.begin(); it != m_trees.end() && m_results.size() < count; ++it)
    {
        it->query(rect, min_constrained_inserter(m_results));
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
SearchContextKdTree::SearchContextKdTree(const Point* points_begin, const Point* points_end) 
    : m_impl(new SearchContextKdTree::Impl(points_begin, points_end))
{
}

SearchContextKdTree::~SearchContextKdTree() 
{
}

int32_t SearchContextKdTree::search_impl(const Rect rect, const int32_t count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
