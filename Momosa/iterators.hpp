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

#include <iterator>

 #pragma pack(push,_CRT_PACKING)
 #pragma warning(push,3)
 #pragma push_macro("new")
 #undef new

// A back inserter iterator that will only insert values to the capacity of container
template<class Container>
class min_constrained_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    typedef min_constrained_iterator<Container> Myt;
    typedef Container container_type;
    typedef typename Container::const_reference const_reference;
    typedef typename Container::value_type Val_t;

    explicit min_constrained_iterator(Container& cont_)
        : container(&cont_)
    {
    }

    Myt& operator=(const Val_t& val)
    {
        insert_impl(val);
        return (*this);
    }

    Myt& operator*()
    {	// pretend to return designated value
        return (*this);
    }

    Myt& operator++()
    {	// pretend to preincrement
        return (*this);
    }

    Myt operator++(int)
    {	// pretend to postincrement
        return (*this);
    }

protected:
    void move_max_to_back()
    {
        auto& c = *container;
        auto max_val_pos = std::max_element(c.begin(), c.end(), 
            [&](const Val_t& a, const Val_t& b) { return a < b; }); // TODO: make this a predicate

        std::iter_swap(max_val_pos, (c.end()-1));
    }

    void insert_impl(const Val_t& val)
    {
        auto& c = *container;
        if(c.size() < c.capacity())
        {
            c.push_back(val);
            move_max_to_back();
        }
        else if(val < c.back()) // TODO: make this a predicate
        {
            c.back() = val;
            move_max_to_back();
        }
    }

protected:
    Container *container;
};

template<class Container> inline
min_constrained_iterator<Container> min_constrained_inserter(Container& cont)
{
    return (min_constrained_iterator<Container>(cont));
}

 #pragma pop_macro("new")
 #pragma warning(pop)
 #pragma pack(pop)
