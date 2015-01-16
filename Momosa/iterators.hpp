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

// A back inserter iterator that will only insert values to the capacity of container
template<class Container>
class min_constrained_iterator : 
    public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    typedef min_constrained_iterator<Container> min_con_iterator;
    typedef Container container_type;
    typedef typename Container::const_reference const_reference;
    typedef typename Container::value_type value_type;

    explicit min_constrained_iterator(Container& c)
        : container(&c)
    {
    }

    min_con_iterator& operator=(value_type const& value)
    {
        insert_impl(value);
        return (*this);
    }

    min_con_iterator& operator*()
    {
        return (*this);
    }

    min_con_iterator& operator++()
    {
        return (*this);
    }

    min_con_iterator operator++(int)
    {
        return (*this);
    }

protected:
    void move_max_to_back()
    {
        auto& c = *container;
        auto max_val_pos = std::max_element(c.begin(), c.end(), 
            [&](const value_type& a, const value_type& b) { return a < b; }); // TODO: make this a predicate

        std::iter_swap(max_val_pos, (c.end()-1));
    }

    void insert_impl(value_type const& value)
    {
        auto& c = *container;
        if(c.size() < c.capacity())
        {
            c.push_back(value);
            move_max_to_back();
        }
        else if(value < c.back()) // TODO: make this a predicate
        {
            c.back() = value;
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
