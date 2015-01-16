#pragma once

#include "SearchContextImpl.hpp"
#include <assert.h>

//#define VALIDATE

class SearchContext
{
public:
    SearchContext(const Point* points_begin, const Point* points_end) 
        : m_impl(points_begin, points_end)
#if defined(VALIDATE)
        , m_implValidate(points_begin, points_end)
#endif
    {
    }

    ~SearchContext() {}

    int32_t search(const Rect rect, const int32_t count, Point* out_points)
    {
#if defined(VALIDATE)
        Point out_valid1[20];
        memcpy(out_valid1, out_points, sizeof(Point)*20);

        auto valid_results = m_implValidate.search(rect, count, out_valid1);
#endif

        auto results = m_impl.search(rect, count, out_points);


#if defined(VALIDATE)
        Point out_valid2[20];
        memcpy(out_valid2, out_points, sizeof(Point)*20);
        if(valid_results > 0) 
        {
            if(results != valid_results || memcmp(out_valid1, out_valid2, sizeof(Point)*20) != 0)
            {
                assert(0);
            }
        }
#endif

        return results;
    }

private:
    SearchContextKdTree m_impl;
    //SearchContextLinear m_impl;
    //SearchContextBoostGeometry m_impl;

#if defined(VALIDATE)
    SearchContextLinear m_implValidate;
#endif
};