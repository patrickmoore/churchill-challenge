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
//#define PROFILE

#if defined(PROFILE)
#include <windows.h>
#include <iostream>
#endif

class SearchContext
{
public:
    SearchContext(const Point* points_begin, const Point* points_end) 
        : m_impl(points_begin, points_end)
#if defined(VALIDATE)
        , m_implValidate(points_begin, points_end)
#endif
#if defined(PROFILE)
        , m_implProf1(points_begin, points_end)
        , m_implProf2(points_begin, points_end)
#endif
    {
    }

    ~SearchContext() {}

    int32_t search(const Rect& rect, const int32_t count, Point* out_points)
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

#if defined(PROFILE)
        LARGE_INTEGER frequency;
        LARGE_INTEGER t1,t2;
        double prof1_elapsedTime;
        double prof2_elapsedTime;
        QueryPerformanceFrequency(&frequency);

        {
            QueryPerformanceCounter(&t1);
            auto resultsProf1 = m_implProf1.search(rect, count, out_points);
            QueryPerformanceCounter(&t2);
            prof1_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
        }

        {
            QueryPerformanceCounter(&t1);
            auto resultsProf2 = m_implProf2.search(rect, count, out_points);
            QueryPerformanceCounter(&t2);
            prof2_elapsedTime=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
        }

        std::cout << std::endl << "Prof1: " << prof1_elapsedTime << "    Prof2: " << prof2_elapsedTime << std::endl;
#endif

        return results;
    }

private:
    SearchContextRTree m_impl;
    //SearchContextKdTree m_impl;

#if defined(VALIDATE)
    SearchContextLinear m_implValidate;
#endif

#if defined(PROFILE)
    SearchContextRTree m_implProf1;
    SearchContextKdTree m_implProf2;
#endif
};