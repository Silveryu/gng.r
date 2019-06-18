/* Copyright 2017 Andrey Goryachev

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include "detail/cosine.hpp"
#include "detail/dot_product.hpp"
#include "detail/l2_dist.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace hnsw {

struct l2_square_distance_ptr_t {

    size_t dim;

    template<class Vector>
    auto operator()(const Vector &one, const Vector &another) const {
        return detail::l2sqr_dist(one, another, this->dim);
    }

  public:
    l2_square_distance_ptr_t(size_t dim): dim(dim){};

    void set_dim(size_t d){
        this->dim = d;
    };

    size_t get_dim(){
        return this->dim;
    }



};

struct l2_square_distance_t {
    template<class Vector>
    auto operator()(const Vector &one, const Vector &another) const {
        if (one.size() != another.size()) {
            throw std::runtime_error("l2_square_distance_t: vectors sizes do not match");
        }

        return detail::l2sqr_dist(one.data(), another.data(), one.size());
    }
};


struct cosine_distance_t {
    template<class Vector>
    auto operator()(const Vector &one, const Vector &another) const {
        if (one.size() != another.size()) {
            throw std::runtime_error("cosine_distance_t: vectors sizes do not match");
        }

        using result_type = decltype(*one.data());

        result_type cosine = detail::cosine(one.data(), another.data(), one.size());

        return std::max(result_type(0), result_type(result_type(1.0) - cosine));
    }
};


// Normalize your vectors before putting them in the index or searching, when using this distance.
struct dot_product_distance_t {
    template<class Vector>
    auto operator()(const Vector &one, const Vector &another) const {
        if (one.size() != another.size()) {
            throw std::runtime_error("dot_product_distance_t: vectors sizes do not match");
        }

        using result_type = decltype(*one.data());

        result_type product = detail::dot_product(one.data(), another.data(), one.size());

        return std::max(result_type(0), result_type(result_type(1.0) - product));
    }
};


}
