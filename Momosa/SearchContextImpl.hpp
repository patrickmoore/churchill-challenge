#pragma once

#include "point_search.h"
#include <vector>
#include <memory>


template<class T>
class SearchContextImpl
{
public:
    int32_t search(const Rect rect, const int32_t count, Point* out_points) 
    {
        return static_cast<T*>(this)->search_impl(rect, count, out_points);
    }
};

class SearchContextLinear : public SearchContextImpl<SearchContextLinear>
{
public:
    SearchContextLinear(const Point* points_begin, const Point* points_end);
    ~SearchContextLinear();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class SearchContextKdTree : public SearchContextImpl<SearchContextKdTree>
{
public:
    SearchContextKdTree(const Point* points_begin, const Point* points_end);
    ~SearchContextKdTree();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class SearchContextBoostGeometry : public SearchContextImpl<SearchContextBoostGeometry>
{
public:
    SearchContextBoostGeometry(const Point* points_begin, const Point* points_end);
    ~SearchContextBoostGeometry();
    int32_t search_impl(const Rect rect, const int32_t count, Point* out_points);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

