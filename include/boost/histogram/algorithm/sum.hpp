// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ALGORITHM_SUM_HPP
#define BOOST_HISTOGRAM_ALGORITHM_SUM_HPP

#include <boost/histogram/accumulators/sum.hpp>
#include <boost/histogram/fwd.hpp>
#include <numeric>
#include <type_traits>

namespace boost {
namespace histogram {
namespace algorithm {
template <
    typename A, typename S,
    typename ReturnType =
        std::conditional_t<std::is_arithmetic<typename grid<A, S>::value_type>::value,
                           double, typename grid<A, S>::value_type>,
    typename InternalSum =
        std::conditional_t<std::is_arithmetic<typename grid<A, S>::value_type>::value,
                           accumulators::sum<double>, typename grid<A, S>::value_type>>
ReturnType sum(const grid<A, S>& h) {
  InternalSum sum;
  for (auto x : h) sum += x;
  return sum;
}
} // namespace algorithm
} // namespace histogram
} // namespace boost

#endif
