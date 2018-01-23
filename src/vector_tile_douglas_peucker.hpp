#pragma once

// Some concepts and code layout borrowed from boost geometry
// so boost license is included for this header.

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <mapbox/geometry/geometry.hpp>

// std
#include <vector>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace mapnik 
{

namespace vector_tile_impl
{

namespace detail
{

template<typename T>
struct douglas_peucker_point
{
    mapbox::geometry::point<T> const& p;
    bool included;

    inline douglas_peucker_point(mapbox::geometry::point<T> const& ap)
        : p(ap)
        , included(false)
    {}

    inline douglas_peucker_point<T> operator=(douglas_peucker_point<T> const& )
    {
        return douglas_peucker_point<T>(*this);
    }
};

template <typename value_type, typename calc_type, typename Range>
inline void consider(Range const& vec,
                     std::vector<bool> & included,
                     std::uint32_t begin_idx,
                     std::uint32_t last_idx,
                     calc_type const& max_dist)
{
    std::uint32_t size = last_idx - begin_idx + 1;

    // size must be at least 3
    // because we want to consider a candidate point in between
    if (size <= 2)
    {
        return;
    }
    
    // Find most far point, compare to the current segment
    calc_type md(-1.0); // any value < 0
    std::uint32_t candidate = begin_idx + 1;
    {
        /*
            Algorithm [p: (px,py), p1: (x1,y1), p2: (x2,y2)]
            VECTOR v(x2 - x1, y2 - y1)
            VECTOR w(px - x1, py - y1)
            c1 = w . v
            c2 = v . v
            b = c1 / c2
            RETURN POINT(x1 + b * vx, y1 + b * vy)
        */
        auto const& last = vec[last_idx];
        auto const& begin = vec[begin_idx];
        calc_type const v_x = last.x - begin.x;
        calc_type const v_y = last.y - begin.y;
        calc_type const c2 = v_x * v_x + v_y * v_y;
        std::uint32_t i = begin_idx + 1;

        if (size > 4)
        {
            __m128 begin_x_4 = _mm_set1_ps(begin.x);
            __m128 begin_y_4 = _mm_set1_ps(begin.y);
            __m128 last_x_4 = _mm_set1_ps(last.x);
            __m128 last_y_4 = _mm_set1_ps(last.y);
            __m128 v_x_4 =  _mm_set1_ps(v_x);
            __m128 v_y_4 =  _mm_set1_ps(v_y);
            __m128 c2_4 =  _mm_set1_ps(c2);
            __m128 zero_4 = _mm_set1_ps(0.0);
            __m128 md_4 = _mm_set1_ps(md);
            __m128 md_idx = _mm_castsi128_ps(_mm_set1_epi32(i));

            for (; i + 3 < last_idx; i += 4)
            {
                // Setup loop values
                __m128 it_x_4 = _mm_set_ps(vec[i].x, vec[i+1].x, vec[i+2].x, vec[i+3].x);
                __m128 it_y_4 = _mm_set_ps(vec[i].y, vec[i+1].y, vec[i+2].y, vec[i+3].y);
                __m128 it_idx = _mm_castsi128_ps(_mm_set_epi32(i, i+1, i+2, i+3));     
                
                // Calculate c1
                __m128 w_x_4 = _mm_sub_ps(it_x_4, begin_x_4);
                __m128 w_y_4 = _mm_sub_ps(it_y_4, begin_y_4);
                __m128 u_x_4 = _mm_sub_ps(it_x_4, last_x_4);
                __m128 u_y_4 = _mm_sub_ps(it_y_4, last_y_4);
                __m128 c1_4 = _mm_sub_ps(_mm_mul_ps(w_x_4, v_x_4), _mm_mul_ps(w_y_4, v_y_4));
                
                // Calculate two optional distance results
                __m128 dist_1 = _mm_add_ps(_mm_mul_ps(w_x_4, w_x_4), _mm_mul_ps(w_y_4, w_y_4));
                __m128 dist_2 = _mm_add_ps(_mm_mul_ps(u_x_4, u_x_4), _mm_mul_ps(u_y_4, u_y_4));
                __m128 bool_dist_1 = _mm_cmple_ps(c1_4, zero_4);
                __m128 bool_dist_2 = _mm_cmple_ps(c2_4, c1_4);

                // Calculate the default distance solution now
                __m128 b_4 = _mm_div_ps(c1_4, c2_4);
                __m128 dx_4 = _mm_sub_ps(w_x_4, _mm_mul_ps(b_4, v_x_4));
                __m128 dy_4 = _mm_sub_ps(w_y_4, _mm_mul_ps(b_4, v_y_4));
                __m128 dist_3 = _mm_add_ps(_mm_mul_ps(dx_4, dx_4), _mm_mul_ps(dy_4, dy_4));
                
                __m128 dist = _mm_and_ps(dist_1, bool_dist_1);
                dist = _mm_or_ps(dist, _mm_and_ps(_mm_andnot_ps(bool_dist_1, bool_dist_2), dist_2));
                dist = _mm_or_ps(dist, _mm_andnot_ps(_mm_and_ps(bool_dist_1, bool_dist_2), dist_3));
                __m128 bool_md = _mm_cmplt_ps(md_4, dist);
                md_4 = _mm_or_ps(_mm_andnot_ps(bool_md, md_4), _mm_and_ps(bool_md, dist));
                md_idx = _mm_or_ps(_mm_andnot_ps(bool_md, md_idx), _mm_and_ps(bool_md, it_idx));
            }

            float md_out [4];
            std::uint32_t md_idx_out [4];
            _mm_store_ps(md_out, md_4);
            _mm_store_ps(reinterpret_cast<float*>(md_idx_out), md_idx);
            
            md = md_out[0];
            candidate = md_idx_out[0];
            for (std::size_t j = 1; j < 4; ++j)
            {
                if (md < md_out[j])
                {
                    md = md_out[j];
                    candidate = md_idx_out[j];
                }
                else if (md == md_out[j] && md_idx_out[j] < candidate)
                {
                    candidate = md_idx_out[j];
                }
            }
        }

        for (; i < last_idx; ++i)
        {
            auto const& it = vec[i];
            calc_type const w_x = it.x - begin.x;
            calc_type const w_y = it.y - begin.y;
            calc_type const c1 = w_x * v_x + w_y * v_y;
            calc_type dist;
            if (c1 <= 0) // calc_type() should be 0 of the proper calc type format
            {
                dist = w_x * w_x + w_y * w_y;
            }
            else if (c2 <= c1)
            {
                calc_type const dx = it.x - last.x;
                calc_type const dy = it.y - last.y;
                dist = dx * dx + dy * dy;
            }
            else 
            {
                // See above, c1 > 0 AND c2 > c1 so: c2 != 0
                calc_type const b = c1 / c2;
                calc_type const dx = w_x - b * v_x;
                calc_type const dy = w_y - b * v_y;
                dist = dx * dx + dy * dy;
            }
            if (md < dist)
            {
                md = dist;
                candidate = i;
            }
        }
    }

    // If a point is found, set the include flag
    // and handle segments in between recursively
    if (max_dist < md)
    {
        included[candidate] = true;
        consider<value_type, calc_type, Range>(vec, included, begin_idx, candidate, max_dist);
        consider<value_type, calc_type, Range>(vec, included, candidate, last_idx, max_dist);
    }
}

template <typename value_type, typename calc_type>
inline void consider(typename std::vector<douglas_peucker_point<value_type> >::iterator begin,
                     typename std::vector<douglas_peucker_point<value_type> >::iterator end,
                     calc_type const& max_dist)
{
    typedef typename std::vector<douglas_peucker_point<value_type> >::iterator iterator_type;
    
    std::size_t size = end - begin;

    // size must be at least 3
    // because we want to consider a candidate point in between
    if (size <= 2)
    {
        return;
    }

    iterator_type last = end - 1;

    // Find most far point, compare to the current segment
    calc_type md(-1.0); // any value < 0
    iterator_type candidate;
    {
        /*
            Algorithm [p: (px,py), p1: (x1,y1), p2: (x2,y2)]
            VECTOR v(x2 - x1, y2 - y1)
            VECTOR w(px - x1, py - y1)
            c1 = w . v
            c2 = v . v
            b = c1 / c2
            RETURN POINT(x1 + b * vx, y1 + b * vy)
        */
        calc_type const v_x = last->p.x - begin->p.x;
        calc_type const v_y = last->p.y - begin->p.y;
        calc_type const c2 = v_x * v_x + v_y * v_y;

        for(iterator_type it = begin + 1; it != last; ++it)
        {
            calc_type const w_x = it->p.x - begin->p.x;
            calc_type const w_y = it->p.y - begin->p.y;
            calc_type const c1 = w_x * v_x + w_y * v_y;
            calc_type dist;
            if (c1 <= 0) // calc_type() should be 0 of the proper calc type format
            {
                calc_type const dx = it->p.x - begin->p.x;
                calc_type const dy = it->p.y - begin->p.y;
                dist = dx * dx + dy * dy;
            }
            else if (c2 <= c1)
            {
                calc_type const dx = it->p.x - last->p.x;
                calc_type const dy = it->p.y - last->p.y;
                dist = dx * dx + dy * dy;
            }
            else 
            {
                // See above, c1 > 0 AND c2 > c1 so: c2 != 0
                calc_type const b = c1 / c2;
                calc_type const dx = w_x - b * v_x;
                calc_type const dy = w_y - b * v_y;
                dist = dx * dx + dy * dy;
            }
            if (md < dist)
            {
                md = dist;
                candidate = it;
            }
        }
    }

    // If a point is found, set the include flag
    // and handle segments in between recursively
    if (max_dist < md)
    {
        candidate->included = true;
        consider<value_type>(begin, candidate + 1, max_dist);
        consider<value_type>(candidate, end, max_dist);
    }
}

} // end ns detail

template <typename value_type, typename calc_type, typename Range, typename OutputIterator>
inline void douglas_peucker(Range const& range,
                            OutputIterator out,
                            calc_type max_distance)
{
    if (range.size() < std::numeric_limits<std::uint32_t>::max())
    {
        std::vector<bool> included(range.size());
        
        // Include first and last point of line,
        // they are always part of the line
        included.front() = true;
        included.back() = true;
        
        // We will compare to squared of distance so we don't have to do a sqrt
        float const max_sqrd = max_distance * max_distance;
        
        detail::consider<value_type, float, Range>(range, included, 0, range.size() - 1, max_sqrd);
        
        // Copy included elements to the output
        auto data = std::begin(range);
        auto end_data = std::end(range);
        auto inc = included.begin();
        for (; data != end_data; ++data)
        {
            if (*inc)
            {
                // copy-coordinates does not work because OutputIterator
                // does not model Point (??)
                //geometry::convert(it->p, *out);
                *out++ = *data;
            }
            ++inc;
        }
    }
    else
    {
        // Copy coordinates, a vector of references to all points
        std::vector<detail::douglas_peucker_point<value_type> > ref_candidates(std::begin(range),
                        std::end(range));

        // Include first and last point of line,
        // they are always part of the line
        ref_candidates.front().included = true;
        ref_candidates.back().included = true;

        // We will compare to squared of distance so we don't have to do a sqrt
        calc_type const max_sqrd = max_distance * max_distance;
        
        // Get points, recursively, including them if they are further away
        // than the specified distance
        detail::consider<value_type, calc_type>(std::begin(ref_candidates), std::end(ref_candidates), max_sqrd);
        
        // Copy included elements to the output
        for(typename std::vector<detail::douglas_peucker_point<value_type> >::const_iterator it
                        = std::begin(ref_candidates);
            it != std::end(ref_candidates);
            ++it)
        {
            if (it->included)
            {
                // copy-coordinates does not work because OutputIterator
                // does not model Point (??)
                //geometry::convert(it->p, *out);
                *out++ = it->p;
            }
        }
    }

}

} // end ns vector_tile_impl

} // end ns mapnik
