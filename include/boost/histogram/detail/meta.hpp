// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_META_HPP
#define BOOST_HISTOGRAM_DETAIL_META_HPP

#include <boost/config/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#if BOOST_WORKAROUND(BOOST_GCC, >= 60000)
#pragma GCC diagnostic pop
#endif
#include <boost/histogram/fwd.hpp>
#include <boost/mp11.hpp>
#include <functional>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
using unqual = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T, class U>
using convert_integer = mp11::mp_if<std::is_integral<unqual<T>>, U, T>;

template <class T>
using mp_size = mp11::mp_size<unqual<T>>;

template <typename T, unsigned N>
using mp_at_c = mp11::mp_at_c<unqual<T>, N>;

template <template <class> class F, typename T, typename E>
using mp_eval_or = mp11::mp_eval_if_c<!(mp11::mp_valid<F, T>::value), E, F, T>;

template <typename T1, typename T2>
using copy_qualifiers = mp11::mp_if<
    std::is_rvalue_reference<T1>, T2&&,
    mp11::mp_if<std::is_lvalue_reference<T1>,
                mp11::mp_if<std::is_const<typename std::remove_reference<T1>::type>,
                            const T2&, T2&>,
                mp11::mp_if<std::is_const<T1>, const T2, T2>>>;

template <typename S, typename L>
using mp_set_union = mp11::mp_apply_q<mp11::mp_bind_front<mp11::mp_set_push_back, S>, L>;

template <typename L>
using mp_last = mp11::mp_at_c<L, (mp_size<L>::value - 1)>;

template <typename T>
using args_type = mp11::mp_if<std::is_member_function_pointer<T>,
                              mp11::mp_pop_front<boost::callable_traits::args_t<T>>,
                              boost::callable_traits::args_t<T>>;

template <typename T, std::size_t N = 0>
using arg_type = typename mp11::mp_at_c<args_type<T>, N>;

template <typename T>
using return_type = typename boost::callable_traits::return_type<T>::type;

template <typename V>
struct variant_first_arg_qualified_impl {
  using UV = unqual<V>;
  using T0 = mp11::mp_first<UV>;
  using type = copy_qualifiers<V, unqual<T0>>;
};

template <typename F, typename V,
          typename T = typename variant_first_arg_qualified_impl<V>::type>
using visitor_return_type = decltype(std::declval<F>()(std::declval<T>()));

template <bool B, typename T, typename F, typename... Ts>
constexpr decltype(auto) static_if_c(T&& t, F&& f, Ts&&... ts) {
  return std::get<(B ? 0 : 1)>(std::forward_as_tuple(
      std::forward<T>(t), std::forward<F>(f)))(std::forward<Ts>(ts)...);
}

template <typename B, typename... Ts>
constexpr decltype(auto) static_if(Ts&&... ts) {
  return static_if_c<B::value>(std::forward<Ts>(ts)...);
}

template <typename T>
constexpr T lowest() {
  return std::numeric_limits<T>::lowest();
}

template <>
constexpr double lowest() {
  return -std::numeric_limits<double>::infinity();
}

template <>
constexpr float lowest() {
  return -std::numeric_limits<float>::infinity();
}

template <typename T>
constexpr T highest() {
  return std::numeric_limits<T>::max();
}

template <>
constexpr double highest() {
  return std::numeric_limits<double>::infinity();
}

template <>
constexpr float highest() {
  return std::numeric_limits<float>::infinity();
}

template <class... Ns>
struct sub_tuple_impl {
  template <typename T>
  static decltype(auto) apply(const T& t) {
    return std::forward_as_tuple(std::get<Ns::value>(t)...);
  }
};

template <std::size_t Offset, std::size_t N, typename T>
decltype(auto) sub_tuple(const T& t) {
  using LN = mp11::mp_iota_c<N>;
  using OffsetAdder = mp11::mp_bind_front<mp11::mp_plus, mp11::mp_size_t<Offset>>;
  using LN2 = mp11::mp_transform_q<OffsetAdder, LN>;
  return mp11::mp_rename<LN2, sub_tuple_impl>::apply(t);
}

template <typename T>
using get_storage_tag = typename T::storage_tag;

template <typename T>
using is_storage = mp11::mp_valid<get_storage_tag, T>;

#define BOOST_HISTOGRAM_MAKE_SFINAE(name, cond) \
  template <class TT>                           \
  struct name##_impl {                          \
    template <class T, class = decltype(cond)>  \
    static std::true_type Test(std::nullptr_t); \
    template <class T>                          \
    static std::false_type Test(...);           \
    using type = decltype(Test<TT>(nullptr));   \
  };                                            \
  template <class T>                            \
  using name = typename name##_impl<T>::type

#define BOOST_HISTOGRAM_MAKE_BINARY_SFINAE(name, cond)  \
  template <class TT, class UU>                         \
  struct name##_impl {                                  \
    template <class T, class U, class = decltype(cond)> \
    static std::true_type Test(std::nullptr_t);         \
    template <class T, class U>                         \
    static std::false_type Test(...);                   \
    using type = decltype(Test<TT, UU>(nullptr));       \
  };                                                    \
  template <class T, class U = T>                       \
  using name = typename name##_impl<T, U>::type

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_metadata, (std::declval<T&>().metadata()));

// resize has two overloads, trying to get pmf in this case always fails
BOOST_HISTOGRAM_MAKE_SFINAE(has_method_resize, (std::declval<T&>().resize(0)));

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_size, &T::size);

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_clear, &T::clear);

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_lower, &T::lower);

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_value, &T::value);

template <typename T>
using get_value_method_return_type_impl = decltype(std::declval<T&>().value(0));

template <typename T, typename R>
using has_method_value_with_convertible_return_type =
    typename std::is_convertible<mp_eval_or<get_value_method_return_type_impl, T, void>,
                                 R>::type;

BOOST_HISTOGRAM_MAKE_SFINAE(has_method_options, (std::declval<const T&>().options()));

BOOST_HISTOGRAM_MAKE_SFINAE(has_allocator, &T::get_allocator);

BOOST_HISTOGRAM_MAKE_SFINAE(is_indexable, (std::declval<T&>()[0]));

BOOST_HISTOGRAM_MAKE_SFINAE(is_transform, (&T::forward, &T::inverse));

BOOST_HISTOGRAM_MAKE_SFINAE(is_vector_like,
                            (std::declval<T&>()[0], &T::size,
                             std::declval<T&>().resize(0), &T::cbegin, &T::cend));

BOOST_HISTOGRAM_MAKE_SFINAE(is_array_like,
                            (std::declval<T&>()[0], &T::size, std::tuple_size<T>::value,
                             &T::cbegin, &T::cend));

BOOST_HISTOGRAM_MAKE_SFINAE(is_map_like,
                            (typename T::key_type(), typename T::mapped_type(),
                             std::declval<T&>().begin(), std::declval<T&>().end()));

BOOST_HISTOGRAM_MAKE_SFINAE(is_indexable_container, (std::declval<T&>()[0], &T::size,
                                                     std::begin(std::declval<T&>()),
                                                     std::end(std::declval<T&>())));

// is_axis is false for axis::variant, because operator() is templated
BOOST_HISTOGRAM_MAKE_SFINAE(is_axis, (&T::size, &T::operator()));

BOOST_HISTOGRAM_MAKE_SFINAE(is_iterable, (std::begin(std::declval<T&>()),
                                          std::end(std::declval<T&>())));

BOOST_HISTOGRAM_MAKE_SFINAE(is_streamable,
                            (std::declval<std::ostream&>() << std::declval<T&>()));

BOOST_HISTOGRAM_MAKE_SFINAE(is_incrementable, (++std::declval<T&>()));

BOOST_HISTOGRAM_MAKE_SFINAE(has_fixed_size, (std::tuple_size<T>::value));

BOOST_HISTOGRAM_MAKE_SFINAE(has_operator_rmul, (std::declval<T&>() *= 1.0));

BOOST_HISTOGRAM_MAKE_SFINAE(has_operator_preincrement, (++std::declval<T&>()));

BOOST_HISTOGRAM_MAKE_BINARY_SFINAE(has_operator_equal, (std::declval<const T&>() ==
                                                        std::declval<const U&>()));

BOOST_HISTOGRAM_MAKE_BINARY_SFINAE(has_operator_radd,
                                   (std::declval<T&>() += std::declval<U&>()));

template <typename T>
struct is_tuple_impl : std::false_type {};

template <typename... Ts>
struct is_tuple_impl<std::tuple<Ts...>> : std::true_type {};

template <typename T>
using is_tuple = typename is_tuple_impl<T>::type;

template <typename T>
struct is_axis_variant_impl : std::false_type {};

template <typename... Ts>
struct is_axis_variant_impl<axis::variant<Ts...>> : std::true_type {};

template <typename T>
using is_axis_variant = typename is_axis_variant_impl<T>::type;

template <typename T>
using is_any_axis = mp11::mp_or<is_axis<T>, is_axis_variant<T>>;

template <typename T>
using is_sequence_of_axis = mp11::mp_and<is_iterable<T>, is_axis<mp11::mp_first<T>>>;

template <typename T>
using is_sequence_of_axis_variant =
    mp11::mp_and<is_iterable<T>, is_axis_variant<mp11::mp_first<T>>>;

template <typename T>
using is_sequence_of_any_axis =
    mp11::mp_and<is_iterable<T>, is_any_axis<mp11::mp_first<T>>>;

template <typename T>
struct is_weight_impl : std::false_type {};

template <typename T>
struct is_weight_impl<weight_type<T>> : std::true_type {};

template <typename T>
using is_weight = is_weight_impl<unqual<T>>;

template <typename T>
struct is_sample_impl : std::false_type {};

template <typename T>
struct is_sample_impl<sample_type<T>> : std::true_type {};

template <typename T>
using is_sample = is_sample_impl<unqual<T>>;

// poor-mans concept checks
template <class B>
using requires = std::enable_if_t<B::value>;

template <class T, class = decltype(*std::declval<T&>(), ++std::declval<T&>())>
struct requires_iterator {};

template <class T, class = requires<is_iterable<unqual<T>>>>
struct requires_iterable {};

template <class T, class = requires<is_axis<unqual<T>>>>
struct requires_axis {};

template <class T, class = requires<is_any_axis<unqual<T>>>>
struct requires_any_axis {};

template <class T, class = requires<is_sequence_of_axis<unqual<T>>>>
struct requires_sequence_of_axis {};

template <class T, class = requires<is_sequence_of_axis_variant<unqual<T>>>>
struct requires_sequence_of_axis_variant {};

template <class T, class = requires<is_sequence_of_any_axis<unqual<T>>>>
struct requires_sequence_of_any_axis {};

template <class T, class = requires<is_any_axis<mp11::mp_first<unqual<T>>>>>
struct requires_axes {};

template <class T, class U, class = requires<std::is_convertible<T, U>>>
struct requires_convertible {};

template <typename T>
auto make_default(const T& t) {
  using U = unqual<T>;
  return static_if<has_allocator<U>>([](const auto& t) { return U(t.get_allocator()); },
                                     [](const auto&) { return U(); }, t);
}

template <typename T>
constexpr bool relaxed_equal(const T& a, const T& b) noexcept {
  return static_if<has_operator_equal<unqual<T>>>(
      [](const auto& a, const auto& b) { return a == b; },
      [](const auto&, const auto&) { return true; }, a, b);
}

template <typename T>
using get_scale_type_helper = typename T::value_type;

template <typename T>
using get_scale_type = detail::mp_eval_or<detail::get_scale_type_helper, T, T>;

struct one_unit {};

template <typename T>
T operator*(T&& t, const one_unit&) {
  return std::forward<T>(t);
}

template <typename T>
T operator/(T&& t, const one_unit&) {
  return std::forward<T>(t);
}

template <typename T>
using get_unit_type_helper = typename T::unit_type;

template <typename T>
using get_unit_type = detail::mp_eval_or<detail::get_unit_type_helper, T, one_unit>;

template <typename T, typename R = get_scale_type<T>>
R get_scale(const T& t) {
  return t / get_unit_type<T>();
}

struct product {
  auto operator()() { return 1.0; } // namespace detail

  template <class T, class... Ts>
  auto operator()(T t, Ts... ts) {
    return t * product()(ts...);
  }
}; // namespace histogram

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
