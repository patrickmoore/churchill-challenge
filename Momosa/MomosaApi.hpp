#pragma once

#if defined(MOMONAV_DLL_EXPORTS)
#   define MOMONAV_DLL_API __declspec(dllexport)
#else
#   define MOMONAV_DLL_API __declspec(dllimport)
#endif

#include "point_search.h"

extern "C" 
{
    MOMONAV_DLL_API SearchContext* create(const Point* points_begin, const Point* points_end);
    MOMONAV_DLL_API int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
    MOMONAV_DLL_API SearchContext* destroy(SearchContext* sc);
}
