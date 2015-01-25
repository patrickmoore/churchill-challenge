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
    SearchContextRTree m_impl;
    //SearchContextKdTree m_impl;
    //SearchContextBoostGeometry m_impl;

#if defined(VALIDATE)
    SearchContextLinear m_implValidate;
#endif
};