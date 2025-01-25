// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _MSFT_PROXY_
#define _MSFT_PROXY_

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <bit>
//#include <concepts>
#include <exception>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <vector>
/*#if __STDC_HOSTED__
#include <format>
#endif  // __STDC_HOSTED__*/
#if __cplusplus < 202002L
namespace std{
  template<typename T>
  struct type_identity{
    typedef T type;
  };
  template<typename _Tp, typename... _Args>
    constexpr auto
    construct_at(_Tp* __location, _Args&&... __args)
    noexcept(noexcept(::new((void*)0) _Tp(std::declval<_Args>()...)))
    -> decltype(::new((void*)0) _Tp(std::declval<_Args>()...))
    { return ::new((void*)__location) _Tp(std::forward<_Args>(__args)...); }
  template<typename _Tp>
    constexpr bool
    has_single_bit(_Tp __x) noexcept
    { return std::__popcount(__x) == 1; }

  template<size_t _Align, class _Tp>
    [[nodiscard,__gnu__::__always_inline__]]
    constexpr _Tp*
    assume_aligned(_Tp* __ptr) noexcept
    {
      static_assert(std::has_single_bit(_Align));
      if (__builtin_constant_p (__ptr))
	return __ptr;
      else
	{
	  // This function is expected to be used in hot code, where
	  // __glibcxx_assert would add unwanted overhead.
	  _GLIBCXX_DEBUG_ASSERT((uintptr_t)__ptr % _Align == 0);
	  return static_cast<_Tp*>(__builtin_assume_aligned(__ptr, _Align));
	}
    }
};
#endif

#if __has_cpp_attribute(msvc::no_unique_address)
#define ___PRO_NO_UNIQUE_ADDRESS_ATTRIBUTE msvc::no_unique_address
#elif __has_cpp_attribute(no_unique_address)
#define ___PRO_NO_UNIQUE_ADDRESS_ATTRIBUTE no_unique_address
#else
#error "Proxy requires C++20 attribute no_unique_address"
#endif

#ifdef __cpp_exceptions
#define ___PRO_THROW(...) throw __VA_ARGS__
#else
#define ___PRO_THROW(...) std::abort()
#endif  // __cpp_exceptions

#ifdef _MSC_VER
#define ___PRO_ENFORCE_EBO __declspec(empty_bases)
#else
#define ___PRO_ENFORCE_EBO
#endif  // _MSC_VER

#ifdef NDEBUG
#define ___PRO_DEBUG(...)
#else
#define ___PRO_DEBUG(...) __VA_ARGS__
#endif  // NDEBUG

#define __msft_lib_proxy 202410L

namespace pro {

enum class constraint_level { none, nontrivial, nothrow, trivial };

template<std::size_t Tmax_size, std::size_t Tmax_align, constraint_level Tcopyability, constraint_level Trelocatability, constraint_level Tdestructibility>
struct proxiable_ptr_constraints_t{
  template<std::size_t new_max_size>
  using with_max_size = proxiable_ptr_constraints_t<new_max_size, Tmax_align, Tcopyability, Trelocatability, Tdestructibility>;

  template<std::size_t new_max_align>
  using with_max_align = proxiable_ptr_constraints_t<Tmax_size, new_max_align, Tcopyability, Trelocatability, Tdestructibility>;

  template<constraint_level new_copyability>
  using with_copyability = proxiable_ptr_constraints_t<Tmax_size, Tmax_align, new_copyability, Trelocatability, Tdestructibility>;

  template<constraint_level new_relocatability>
  using with_relocatability = proxiable_ptr_constraints_t<Tmax_size, Tmax_align, Tcopyability, new_relocatability, Tdestructibility>;

  template<constraint_level new_destructibility>
  using with_destructibility = proxiable_ptr_constraints_t<Tmax_size, Tmax_align, Tcopyability, Trelocatability, new_destructibility>;

  static constexpr auto max_size = Tmax_size;
  static constexpr auto max_align = Tmax_align;
  static constexpr auto copyability= Tcopyability;
  static constexpr auto relocatability= Trelocatability;
  static constexpr auto destructibility= Tdestructibility;
};

struct proxiable_ptr_constraints {
  std::size_t max_size;
  std::size_t max_align;
  constraint_level copyability;
  constraint_level relocatability;
  constraint_level destructibility;
};

template <class F> struct proxy_indirect_accessor;
template <class F> class proxy;


namespace details {

struct applicable_traits { static constexpr bool applicable = true; };
struct inapplicable_traits { static constexpr bool applicable = false; };

enum class qualifier_type { lv, const_lv, rv, const_rv };
template <class T, qualifier_type Q> struct add_qualifier_traits;
template <class T>
struct add_qualifier_traits<T, qualifier_type::lv> : std::type_identity<T&> {};
template <class T>
struct add_qualifier_traits<T, qualifier_type::const_lv>
    : std::type_identity<const T&> {};
template <class T>
struct add_qualifier_traits<T, qualifier_type::rv> : std::type_identity<T&&> {};
template <class T>
struct add_qualifier_traits<T, qualifier_type::const_rv>
    : std::type_identity<const T&&> {};
template <class T, qualifier_type Q>
using add_qualifier_t = typename add_qualifier_traits<T, Q>::type;
template <class T, qualifier_type Q>
using add_qualifier_ptr_t = std::remove_reference_t<add_qualifier_t<T, Q>>*;

template <template <class, class> class R, class O, class... Is>
struct recursive_reduction : std::type_identity<O> {};
template <template <class, class> class R, class O, class I, class... Is>
struct recursive_reduction<R, O, I, Is...>
    : recursive_reduction<R, R<O, I>, Is...> {};
template <template <class, class> class R, class O, class... Is>
using recursive_reduction_t = typename recursive_reduction<R, O, Is...>::type;

template<typename... Args>
struct template_args_count;
template<>
struct template_args_count<>{
    static constexpr int value = 0;
};
template<typename Head, typename... Args>
struct template_args_count<Head, Args...>{
    static constexpr int value = template_args_count<Args...>::value + 1;
};

template <class T, std::size_t I>
struct has_tuple_element;
template <typename... Args, std::size_t I>
struct has_tuple_element<std::tuple<Args...>, I>{
    static constexpr bool value = template_args_count<Args...>::value > I;
};

template <template <class...> class T, class TL, class Is, class... Args>
struct instantiated_traits;
template <template <class...> class T, class TL, std::size_t... Is,
    class... Args>
struct instantiated_traits<T, TL, std::index_sequence<Is...>, Args...>
    : std::type_identity<T<Args..., std::tuple_element_t<Is, TL>...>> {};
template <template <class...> class T, class TL, class... Args>
using instantiated_t = typename instantiated_traits<
    T, TL, std::make_index_sequence<std::tuple_size_v<TL>>, Args...>::type;

template <class T>
constexpr bool has_copyability(constraint_level level) {
  switch (level) {
    case constraint_level::none: return true;
    case constraint_level::nontrivial: return std::is_copy_constructible_v<T>;
    case constraint_level::nothrow:
      return std::is_nothrow_copy_constructible_v<T>;
    case constraint_level::trivial:
      return std::is_trivially_copy_constructible_v<T> &&
          std::is_trivially_destructible_v<T>;
    default: return false;
  }
}
template <class T>
constexpr bool has_relocatability(constraint_level level) {
  switch (level) {
    case constraint_level::none: return true;
    case constraint_level::nontrivial:
      return std::is_move_constructible_v<T> && std::is_destructible_v<T>;
    case constraint_level::nothrow:
      return std::is_nothrow_move_constructible_v<T> &&
          std::is_nothrow_destructible_v<T>;
    case constraint_level::trivial:
      return std::is_trivially_move_constructible_v<T> &&
          std::is_trivially_destructible_v<T>;
    default: return false;
  }
}
template <class T>
constexpr bool has_destructibility(constraint_level level) {
  switch (level) {
    case constraint_level::none: return true;
    case constraint_level::nontrivial: return std::is_destructible_v<T>;
    case constraint_level::nothrow: return std::is_nothrow_destructible_v<T>;
    case constraint_level::trivial: return std::is_trivially_destructible_v<T>;
    default: return false;
  }
}

template <class T>
class destruction_guard {
 public:
  explicit destruction_guard(T* p) noexcept : p_(p) {}
  destruction_guard(const destruction_guard&) = delete;
  ~destruction_guard() noexcept(std::is_nothrow_destructible_v<T>)
      { std::destroy_at(p_); }

 private:
  T* p_;
};

template <class P, qualifier_type Q, bool NE>
struct ptr_traits_applic: applicable_traits{
  using target_type = decltype(*std::declval<add_qualifier_t<P, Q>>());
};

template <class P, qualifier_type Q, bool NE>
struct ptr_traits_helpers{
  template<class PP>
  static auto test(
    std::type_identity<PP> *, 
    std::in_place_type_t<decltype(*std::declval<add_qualifier_t<PP, Q>>())>* = nullptr
    ) -> ptr_traits_applic<PP, Q, NE>;

  static auto test(...) -> inapplicable_traits;

  using type = decltype(test((std::type_identity<P> *)nullptr));
};



template <class P, qualifier_type Q, bool NE>
struct ptr_traits : ptr_traits_helpers<P, Q, NE>::type {};

template <class D, bool NE, class R, class... Args>
inline constexpr bool invocable_dispatch = (NE && std::is_nothrow_invocable_r_v<
    R, D, Args...>) || (!NE && std::is_invocable_r_v<R, D, Args...>);

template <class D, class P, qualifier_type Q, bool NE, class R, class... Args>
inline constexpr bool invocable_dispatch_ptr_indirect = ptr_traits<P, Q, NE>::applicable &&
    invocable_dispatch<
        D, NE, R, typename ptr_traits<P, Q, NE>::target_type, Args...>;
template <class D, class P, qualifier_type Q, bool NE, class R, class... Args>
inline constexpr bool invocable_dispatch_ptr_direct = invocable_dispatch<
    D, NE, R, add_qualifier_t<P, Q>, Args...> && (Q != qualifier_type::rv ||
        (NE && std::is_nothrow_destructible_v<P>) ||
        (!NE && std::is_destructible_v<P>));

template <bool NE, class R, class... Args>
using func_ptr_t = std::conditional_t<
    NE, R (*)(Args...) noexcept, R (*)(Args...)>;

template <class D, class R, class... Args>
R invoke_dispatch(Args&&... args) {
  if constexpr (std::is_void_v<R>) {
    D{}(std::forward<Args>(args)...);
  } else {
    return D{}(std::forward<Args>(args)...);
  }
}
template <class D, class P, qualifier_type Q, class R, class... Args>
R indirect_conv_dispatcher(add_qualifier_t<std::byte, Q> self, Args... args)
    noexcept(invocable_dispatch_ptr_indirect<D, P, Q, true, R, Args...>) {
  return invoke_dispatch<D, R>(*std::forward<add_qualifier_t<P, Q>>(
      *std::launder(reinterpret_cast<add_qualifier_ptr_t<P, Q>>(&self))),
      std::forward<Args>(args)...);
}
template <class D, class P, qualifier_type Q, class R, class... Args>
R direct_conv_dispatcher(add_qualifier_t<std::byte, Q> self, Args... args)
    noexcept(invocable_dispatch_ptr_direct<D, P, Q, true, R, Args...>) {
  auto& qp = *std::launder(
      reinterpret_cast<add_qualifier_ptr_t<P, Q>>(&self));
  if constexpr (Q == qualifier_type::rv) {
    destruction_guard guard{&qp};
    return invoke_dispatch<D, R>(
        std::forward<add_qualifier_t<P, Q>>(qp), std::forward<Args>(args)...);
  } else {
    return invoke_dispatch<D, R>(
        std::forward<add_qualifier_t<P, Q>>(qp), std::forward<Args>(args)...);
  }
}
template <class D, qualifier_type Q, class R, class... Args>
R default_conv_dispatcher(add_qualifier_t<std::byte, Q>, Args... args)
    noexcept(invocable_dispatch<D, true, R, std::nullptr_t, Args...>)
    { return invoke_dispatch<D, R>(nullptr, std::forward<Args>(args)...); }
template <class P>
void copying_dispatcher(std::byte& self, const std::byte& rhs)
    noexcept(has_copyability<P>(constraint_level::nothrow)) {
  std::construct_at(reinterpret_cast<P*>(&self),
      *std::launder(reinterpret_cast<const P*>(&rhs)));
}
template <std::size_t Len, std::size_t Align>
void copying_default_dispatcher(std::byte& self, const std::byte& rhs)
    noexcept {
  std::uninitialized_copy_n(
      std::assume_aligned<Align>(&rhs), Len, std::assume_aligned<Align>(&self));
}
template <class P>
void relocation_dispatcher(std::byte& self, const std::byte& rhs)
    noexcept(has_relocatability<P>(constraint_level::nothrow)) {
  P* other = std::launder(reinterpret_cast<P*>(const_cast<std::byte*>(&rhs)));
  destruction_guard guard{other};
  std::construct_at(reinterpret_cast<P*>(&self), std::move(*other));
}
template <class P>
void destruction_dispatcher(std::byte& self)
    noexcept(has_destructibility<P>(constraint_level::nothrow))
    { std::destroy_at(std::launder(reinterpret_cast<P*>(&self))); }
inline void destruction_default_dispatcher(std::byte&) noexcept {}

template <class O> struct overload_traits : inapplicable_traits {};
template <qualifier_type Q, bool NE, class R, class... Args>
struct overload_traits_impl : applicable_traits {
  template <bool IsDirect, class D>
  struct meta_provider {
    template <class P>
    static constexpr auto get()
        -> func_ptr_t<NE, R, add_qualifier_t<std::byte, Q>, Args...> {
      if constexpr (!IsDirect &&
          invocable_dispatch_ptr_indirect<D, P, Q, NE, R, Args...>) {
        return &indirect_conv_dispatcher<D, P, Q, R, Args...>;
      } else if constexpr (IsDirect &&
          invocable_dispatch_ptr_direct<D, P, Q, NE, R, Args...>) {
        return &direct_conv_dispatcher<D, P, Q, R, Args...>;
      } else if constexpr (invocable_dispatch<
          D, NE, R, std::nullptr_t, Args...>) {
        return &default_conv_dispatcher<D, Q, R, Args...>;
      } else {
        return nullptr;
      }
    }
  };
  using return_type = R;
  using view_type = R(Args...) const noexcept(NE);

  template <bool IsDirect, class D, class P>
  static constexpr bool applicable_ptr =
      meta_provider<IsDirect, D>::template get<P>() != nullptr;
  static constexpr qualifier_type qualifier = Q;
};
template <class R, class... Args>
struct overload_traits<R(Args...)>
    : overload_traits_impl<qualifier_type::lv, false, R, Args...> {
      static constexpr bool qt = true;
    };
template <class R, class... Args>
struct overload_traits<R(Args...) noexcept>
    : overload_traits_impl<qualifier_type::lv, true, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) &>
    : overload_traits_impl<qualifier_type::lv, false, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) & noexcept>
    : overload_traits_impl<qualifier_type::lv, true, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) &&>
    : overload_traits_impl<qualifier_type::rv, false, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) && noexcept>
    : overload_traits_impl<qualifier_type::rv, true, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const>
    : overload_traits_impl<qualifier_type::const_lv, false, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const noexcept>
    : overload_traits_impl<qualifier_type::const_lv, true, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const&>
    : overload_traits_impl<qualifier_type::const_lv, false, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const& noexcept>
    : overload_traits_impl<qualifier_type::const_lv, true, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const&&>
    : overload_traits_impl<qualifier_type::const_rv, false, R, Args...> {};
template <class R, class... Args>
struct overload_traits<R(Args...) const&& noexcept>
    : overload_traits_impl<qualifier_type::const_rv, true, R, Args...> {};

template <class MP>
struct dispatcher_meta {
  constexpr dispatcher_meta() noexcept : dispatcher(nullptr) {}
  template <class P>
  constexpr explicit dispatcher_meta(std::in_place_type_t<P>) noexcept
      : dispatcher(MP::template get<P>()) {}

  decltype(MP::template get<void>()) dispatcher;
};

template <class... Ms>
struct composite_meta_impl : Ms... {
  constexpr composite_meta_impl() noexcept = default;
  template <class P>
  constexpr explicit composite_meta_impl(std::in_place_type_t<P>) noexcept
      : Ms(std::in_place_type<P>)... {}
};

// Remove I if I is void
template<typename I, typename... Ms>
struct meta_reduction_helper{
  template<typename II>
  static auto test(std::type_identity<II>*, typename std::enable_if<!std::is_void_v<II>, int>::type) -> composite_meta_impl<Ms..., I>;
  static auto test(...) -> composite_meta_impl<Ms...>;

  using type = decltype(test((std::type_identity<I> *)nullptr, 0));
};

template <class O, class I> struct meta_reduction : std::type_identity<O> {};
template <class... Ms, class I>
struct meta_reduction<composite_meta_impl<Ms...>, I>
    : std::type_identity<typename meta_reduction_helper<I, Ms...>::type> {};
template <class... Ms1, class... Ms2>
struct meta_reduction<composite_meta_impl<Ms1...>, composite_meta_impl<Ms2...>>
    : std::type_identity<composite_meta_impl<Ms1..., Ms2...>> {};


template <class O, class I>
using meta_reduction_t = typename meta_reduction<O, I>::type;
template <class... Ms>
using composite_meta =
    recursive_reduction_t<meta_reduction_t, composite_meta_impl<>, Ms...>;


template <class C, class... Os>
struct conv_traits_impl_applic : applicable_traits {
  using meta = composite_meta_impl<dispatcher_meta<typename overload_traits<Os>
      ::template meta_provider<C::is_direct, typename C::dispatch_type>>...>;

  template <class P>
  static constexpr bool applicable_ptr =
      (overload_traits<Os>::template applicable_ptr<
          C::is_direct, typename C::dispatch_type, P> && ...);
};

template<class C, class... Os>
struct conv_traits_impl_helpers{
  template<class... O>
  static auto test(typename std::enable_if<(sizeof...(O) > 0u && (overload_traits<O>::applicable && ...)), int>::type, std::type_identity<O> *...) -> conv_traits_impl_applic<C, O...>;
  static auto test(...) -> inapplicable_traits;

  using type = decltype(conv_traits_impl_helpers::test(0, ((std::type_identity<Os> *)nullptr)...));
};

template <class C, class... Os>
struct conv_traits_impl : conv_traits_impl_helpers<C, Os...>::type {};

template<typename C>
struct conv_traits_helpers{
  template<typename CX>
  static auto test(
    std::in_place_type_t<CX>,
    std::in_place_type_t<typename CX::dispatch_type>* = nullptr,
    std::in_place_type_t<typename CX::overload_types>* = nullptr , 
    typename std::enable_if<std::is_trivial_v<typename C::dispatch_type>, int>::type = 0) 
    -> instantiated_t<conv_traits_impl, typename C::overload_types, C>;

  static auto test(...) -> inapplicable_traits;

  using type = decltype(test(std::in_place_type<C>));
};


template <class C> struct conv_traits : conv_traits_helpers<C>::type {};



template <class P>
using ptr_element_t = typename std::pointer_traits<P>::element_type;

template<class P, bool is_direct>
struct refl_meta_ctor_helpers;

template<class P>
struct refl_meta_ctor_helpers<P, true>{
  using type = std::in_place_type_t<P>;
};

template<class P>
struct refl_meta_ctor_helpers<P, false>{
  using type = std::in_place_type_t<ptr_element_t<P>>;
};

template <bool IsDirect, class R>
struct refl_meta {
  template <class P>
  constexpr explicit refl_meta(std::in_place_type_t<P>)
      : reflector(typename refl_meta_ctor_helpers<P, IsDirect>::type()) {}

  R reflector;
};

template<typename T>
struct has_reflector_type_helper{
  template<typename R>
  struct local_true_type{ char data[2];};
  template<typename C>
  static auto test(C *) -> local_true_type<decltype(C::reflector_type)>;
  static auto test(...) -> char;
  static constexpr bool value = sizeof(test((T *)nullptr)) == sizeof(char);
};

template <class R, bool is_ref_type = has_reflector_type_helper<R>::value > struct refl_traits;
template <class R> struct refl_traits<R, false> : inapplicable_traits {};
template <class R>
struct refl_traits<R, true> : applicable_traits {
  using meta = refl_meta<R::is_direct, typename R::reflector_type>;

  template <class P>
  static constexpr bool applicable_ptr = true;
};

template <bool NE>
struct copyability_meta_provider {
  template <class P>
  static constexpr func_ptr_t<NE, void, std::byte&, const std::byte&> get() {
    if constexpr (has_copyability<P>(constraint_level::trivial)) {
      return &copying_default_dispatcher<sizeof(P), alignof(P)>;
    } else {
      return &copying_dispatcher<P>;
    }
  }
};
template <bool NE>
struct relocatability_meta_provider {
  template <class P>
  static constexpr func_ptr_t<NE, void, std::byte&, const std::byte&> get() {
    if constexpr (has_relocatability<P>(constraint_level::trivial)) {
      return &copying_default_dispatcher<sizeof(P), alignof(P)>;
    } else {
      return &relocation_dispatcher<P>;
    }
  }
};
template <bool NE>
struct destructibility_meta_provider {
  template <class P>
  static constexpr func_ptr_t<NE, void, std::byte&> get() {
    if constexpr (has_destructibility<P>(constraint_level::trivial)) {
      return &destruction_default_dispatcher;
    } else {
      return &destruction_dispatcher<P>;
    }
  }
};
template <template <bool> class MP, constraint_level C>
struct lifetime_meta_traits : std::type_identity<void> {};
template <template <bool> class MP>
struct lifetime_meta_traits<MP, constraint_level::nothrow>
    : std::type_identity<dispatcher_meta<MP<true>>> {};
template <template <bool> class MP>
struct lifetime_meta_traits<MP, constraint_level::nontrivial>
    : std::type_identity<dispatcher_meta<MP<false>>> {};
template <template <bool> class MP, constraint_level C>
using lifetime_meta_t = typename lifetime_meta_traits<MP, C>::type;

template <class... As>
class ___PRO_ENFORCE_EBO composite_accessor_impl : public As... {
  template <class> friend class pro::proxy;
  template <class F> friend struct pro::proxy_indirect_accessor;

  composite_accessor_impl() noexcept = default;
  composite_accessor_impl(const composite_accessor_impl&) noexcept = default;
  composite_accessor_impl& operator=(const composite_accessor_impl&) noexcept
      = default;
};


template <class T>
struct accessor_traits_impl : std::type_identity<typename std::conditional<(std::is_nothrow_default_constructible_v<T> &&
        std::is_trivially_copyable_v<T> && !std::is_final_v<T>), T, void>::type> {};

template <class SFINAE, class T, class F>
struct accessor_traits : std::type_identity<void> {};
template <class T, class F>
struct accessor_traits<std::void_t<typename T::template accessor<F>>, T, F>
    : accessor_traits_impl<typename T::template accessor<F>> {};
template <class T, class F>
using accessor_t = typename accessor_traits<void, T, F>::type;

template<bool IsDirect, class F, class O, class I, bool is_direct = (IsDirect == I::is_direct && !std::is_void_v<accessor_t<I, F>>)>
struct composite_accessor_reduction_helpers;

template <bool IsDirect, class F, class O, class I>
struct composite_accessor_reduction_helpers<IsDirect, F, O, I, false>{
  using type = std::type_identity<O>;
};

template <bool IsDirect, class F, class... As, class I>
struct composite_accessor_reduction_helpers<IsDirect, F, composite_accessor_impl<As...>, I, true>{
  using type = std::type_identity<composite_accessor_impl<As..., accessor_t<I, F>>>;
};


template <bool IsDirect, class F, class O, class I>
struct composite_accessor_reduction : composite_accessor_reduction_helpers<IsDirect, F, O, I>::type {};


template <bool IsDirect, class F>
struct composite_accessor_helper {
  template <class O, class I>
  using reduction_t =
      typename composite_accessor_reduction<IsDirect, F, O, I>::type;
};
template <bool IsDirect, class F, class... Ts>
using composite_accessor = recursive_reduction_t<
      composite_accessor_helper<IsDirect, F>::template reduction_t,
      composite_accessor_impl<>, Ts...>;

template <class A1, class A2> struct composite_accessor_merge_traits;
template <class... A1, class... A2>
struct composite_accessor_merge_traits<
    composite_accessor_impl<A1...>, composite_accessor_impl<A2...>>
    : std::type_identity<composite_accessor_impl<A1..., A2...>> {};
template <class A1, class A2>
using merged_composite_accessor =
    typename composite_accessor_merge_traits<A1, A2>::type;

template <class T> struct in_place_type_traits : inapplicable_traits {};
template <class T>
struct in_place_type_traits<std::in_place_type_t<T>> : applicable_traits {};
template <class T>
constexpr bool is_in_place_type = in_place_type_traits<T>::applicable;

template <class F>
constexpr bool is_facade_constraints_well_formed() {
  if constexpr (std::is_same_v<decltype(F::constraints), const proxiable_ptr_constraints>) {

      return std::has_single_bit(F::constraints.max_align) &&
          F::constraints.max_size % F::constraints.max_align == 0u;
    
  }
  return false;
}

template <class F, class... Cs>
struct facade_conv_traits_impl_appli : applicable_traits {
  using conv_meta = composite_meta<typename conv_traits<Cs>::meta...>;
  using conv_indirect_accessor = composite_accessor<false, F, Cs...>;
  using conv_direct_accessor = composite_accessor<true, F, Cs...>;

  template <class P>
  static constexpr bool conv_applicable_ptr =
      (conv_traits<Cs>::template applicable_ptr<P> && ...);
  template <bool IsDirect, class D, class O>
  static constexpr bool is_invocable = std::is_base_of_v<dispatcher_meta<
      typename overload_traits<O>::template meta_provider<IsDirect, D>>,
      conv_meta>;
};

template <class F, class... Cs>
struct facade_conv_traits_impl_helpers{
  template<class... C>
  static auto test(typename std::enable_if<(conv_traits<C>::applicable && ...), int>::type, std::type_identity<C>* ...) -> facade_conv_traits_impl_appli<F, C...>;
  static auto test(...) -> inapplicable_traits;

  using type = decltype(test(0, ((std::type_identity<Cs> *)nullptr)...));
};
template <class F, class... Cs>
struct facade_conv_traits_impl : facade_conv_traits_impl_helpers<F, Cs...>::type {};


template <class F, class... Rs> 
struct facade_refl_traits_impl_applic : applicable_traits {
  using refl_meta = composite_meta<typename refl_traits<Rs>::meta...>;
  using refl_indirect_accessor = composite_accessor<false, F, Rs...>;
  using refl_direct_accessor = composite_accessor<true, F, Rs...>;

  template <class P>
  static constexpr bool refl_applicable_ptr =
      (refl_traits<Rs>::template applicable_ptr<P> && ...);
};

template <class F, class... Rs>
struct facade_refl_traits_impl_helpers{
  template<class... R>
  static auto test(typename std::enable_if<(refl_traits<R>::applicable && ...), int>::type, std::type_identity<R>* ...) -> facade_refl_traits_impl_applic<F, R...>;
  static auto test(...) -> inapplicable_traits;

  using type = decltype(test(0, ((std::type_identity<Rs> *)nullptr)...));
};
template <class F, class... Rs>
struct facade_refl_traits_impl : facade_refl_traits_impl_helpers<F, Rs...>::type {};

struct poly_cast_meta;

template <class F>
struct facade_traits_applic
    : instantiated_t<facade_conv_traits_impl, typename F::convention_types, F>,
      instantiated_t<facade_refl_traits_impl, typename F::reflection_types, F> {
  using copyability_meta = lifetime_meta_t<
      copyability_meta_provider, F::constraints::copyability>;
  using relocatability_meta = lifetime_meta_t<
      relocatability_meta_provider,
      F::constraints::copyability == constraint_level::trivial ?
          constraint_level::trivial : F::constraints::relocatability>;
  using destructibility_meta = lifetime_meta_t<
      destructibility_meta_provider, F::constraints::destructibility>;
  using meta = composite_meta<poly_cast_meta, copyability_meta, relocatability_meta,
      destructibility_meta, typename facade_traits_applic::conv_meta,
      typename facade_traits_applic::refl_meta>;
  using indirect_accessor = merged_composite_accessor<
      typename facade_traits_applic::conv_indirect_accessor,
      typename facade_traits_applic::refl_indirect_accessor>;
  using direct_accessor = merged_composite_accessor<
      typename facade_traits_applic::conv_direct_accessor,
      typename facade_traits_applic::refl_direct_accessor>;
};

template <class F>
struct facade_traits_helpers{
  template<class FF>
  static auto test(
    std::type_identity<FF> *, 
    typename std::enable_if<instantiated_t<facade_conv_traits_impl, typename FF::convention_types, FF>
            ::applicable, int>::type = 0,
    typename std::enable_if<instantiated_t<facade_refl_traits_impl, typename FF::reflection_types, FF>
            ::applicable, int>::type = 0,
    typename std::type_identity<typename FF::convention_types>* = nullptr,
    typename std::type_identity<typename FF::reflection_types>* = nullptr
  ) -> facade_traits_applic<FF>;
  static auto test(...) -> inapplicable_traits;

  using type = decltype(test((std::type_identity<F> *)nullptr));

};

template <class F> struct facade_traits : facade_traits_helpers<F>::type {};


using ptr_prototype = void*[2];

template <class M>
struct meta_ptr_indirect_impl {
  constexpr meta_ptr_indirect_impl() noexcept : ptr_(nullptr) {};
  constexpr meta_ptr_indirect_impl(const std::byte* ptr) noexcept : ptr_((M *)ptr) {};
  template <class P>
  constexpr explicit meta_ptr_indirect_impl(std::in_place_type_t<P>) noexcept
      : ptr_(&storage<P>) {}
  bool has_value() const noexcept { return ptr_ != nullptr; }
  void reset() noexcept { ptr_ = nullptr; }
  const M* operator->() const noexcept { return ptr_; }
  const M* get_ptr() const noexcept {return ptr_; }

 private:
  const M* ptr_;
  template <class P> static constexpr M storage{std::in_place_type<P>};
};
template <class M, class DM>
struct meta_ptr_direct_impl : private M {
  using M::M;
  bool has_value() const noexcept { return this->DM::dispatcher != nullptr; }
  void reset() noexcept { this->DM::dispatcher = nullptr; }
  const M* operator->() const noexcept { return this; }
};


template <class M>
struct meta_ptr_traits_impl : std::type_identity<meta_ptr_indirect_impl<M>> {};
template <class MP, class... Ms>
struct meta_ptr_traits_impl<composite_meta_impl<dispatcher_meta<MP>, Ms...>>
    : std::type_identity<meta_ptr_direct_impl<composite_meta_impl<
          dispatcher_meta<MP>, Ms...>, dispatcher_meta<MP>>> {};


template <class M>
struct meta_ptr_traits_helpers{
  template<class MM>
  static auto test(std::type_identity<MM> *, typename std::enable_if<(sizeof(MM) <= sizeof(ptr_prototype) &&
        alignof(MM) <= alignof(ptr_prototype) &&
        std::is_nothrow_default_constructible_v<MM> &&
        std::is_trivially_copyable_v<MM>), int>::type = 0) -> typename meta_ptr_traits_impl<MM>::type;

  static auto test(...) -> meta_ptr_indirect_impl<M>;

  using type = decltype(test((std::type_identity<M> *)nullptr));
};

template <class M>
using meta_ptr = typename meta_ptr_traits_helpers<M>::type;


struct static_type_token_impl{
    template<typename T>
    using remove_qualifiers = typename std::remove_const<typename std::remove_reference<T>::type>::type;

    std::string_view type_name;

    bool operator ==(const static_type_token_impl& rhs) const noexcept{
        return type_name == rhs.type_name;
    }
    explicit operator size_t() const noexcept{
        return std::hash<std::string_view>{}(type_name);
    }

    constexpr static_type_token_impl() noexcept: type_name("<none>")  {
    }
    template <class P>
    explicit constexpr static_type_token_impl(std::in_place_type_t<P>) noexcept : type_name(make_buffer<remove_qualifiers<P>>())  {
    }
    template<typename T>
    static constexpr std::string_view make_buffer(){
        constexpr const char *name_buffer = __PRETTY_FUNCTION__;
        constexpr const std::int32_t N = sizeof(__PRETTY_FUNCTION__);

        for (std::int32_t i = N - 2; i >= 0; i--) {
            if (name_buffer[i] == '[') {
                auto start = name_buffer + (i + 5);
                auto len = N - (i + 7);

                return std::string_view(start, len);
            }
        }
        return std::string_view(name_buffer);
    }

    
};

struct static_type_token{
  constexpr static_type_token() noexcept : token_ptr(&null_type){
  }
  template<typename T>
  constexpr static_type_token(std::in_place_type_t<T>) noexcept : token_ptr(&token<T>){
  }

  const static_type_token_impl* token_ptr;

  static constexpr static_type_token_impl null_type{};
  template<typename T> static constexpr static_type_token_impl token{std::in_place_type<T>};

  const static_type_token_impl *operator->() const noexcept{
    return token_ptr;
  }

  bool operator ==(const static_type_token& rhs) const noexcept{
    return token_ptr == rhs.token_ptr;
  }
  operator std::size_t() const noexcept{
    return reinterpret_cast<std::size_t>(token_ptr);
  }
};



template <class MP>
struct meta_ptr_reset_guard {
 public:
  explicit meta_ptr_reset_guard(MP& meta) noexcept : meta_(meta) {}
  meta_ptr_reset_guard(const meta_ptr_reset_guard&) = delete;
  ~meta_ptr_reset_guard() { meta_.reset(); }

 private:
  MP& meta_;
};

template <class F>
struct proxy_helper {
  static inline const auto& get_meta(const proxy<F>& p) noexcept {
    assert((std::ignore = "proxy probably have been dumped" , p.has_value()));
    return *p.meta_.operator->();
  }
  template <bool IsDirect, class D, class O, qualifier_type Q, class... Args>
  static decltype(auto) invoke(add_qualifier_t<proxy<F>, Q> p, Args&&... args) {
    auto dispatcher = get_meta(p)
        .template dispatcher_meta<typename overload_traits<O>
        ::template meta_provider<IsDirect, D>>::dispatcher;
    if constexpr (
        IsDirect && overload_traits<O>::qualifier == qualifier_type::rv) {
      meta_ptr_reset_guard guard{p.meta_};
      return dispatcher(std::forward<add_qualifier_t<std::byte, Q>>(*p.ptr_),
          std::forward<Args>(args)...);
    } else {
      return dispatcher(std::forward<add_qualifier_t<std::byte, Q>>(*p.ptr_),
          std::forward<Args>(args)...);
    }
  }
  template <class A, qualifier_type Q>
  static add_qualifier_t<proxy<F>, Q> access(add_qualifier_t<A, Q> a) {
    if constexpr (std::is_base_of_v<A, proxy<F>>) {
      return static_cast<add_qualifier_t<proxy<F>, Q>>(
          std::forward<add_qualifier_t<A, Q>>(a));
    } else {
      // Note: The use of offsetof below is technically undefined until C++20
      // because proxy may not be a standard layout type. However, all compilers
      // currently provide well-defined behavior as an extension (which is
      // demonstrated since constexpr evaluation must diagnose all undefined
      // behavior). However, various compilers also warn about this use of
      // offsetof, which must be suppressed.
#if defined(__INTEL_COMPILER)
#pragma warning push
#pragma warning(disable : 1875)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif  // defined(__INTEL_COMPILER)
#if defined(__NVCC__)
#pragma nv_diagnostic push
#pragma nv_diag_suppress 1427
#endif  // defined(__NVCC__)
#if defined(__NVCOMPILER)
#pragma diagnostic push
#pragma diag_suppress offset_in_non_POD_nonstandard
#endif  // defined(__NVCOMPILER)
      constexpr std::size_t offset = offsetof(proxy<F>, ia_);
#if defined(__INTEL_COMPILER)
#pragma warning pop
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif  // defined(__INTEL_COMPILER)
#if defined(__NVCC__)
#pragma nv_diagnostic pop
#endif  // defined(__NVCC__)
#if defined(__NVCOMPILER)
#pragma diagnostic pop
#endif  // defined(__NVCOMPILER)

      return reinterpret_cast<add_qualifier_t<proxy<F>, Q>>(
          *(reinterpret_cast<add_qualifier_ptr_t<std::byte, Q>>(
              static_cast<add_qualifier_ptr_t<proxy_indirect_accessor<F>, Q>>(
                  std::addressof(a))) - offset));
    }
  }
};
}  // namespace details

template <class F>
static inline constexpr bool facade = details::facade_traits<F>::applicable;

template <class P, class F>
static inline constexpr bool proxiable = facade<F> && sizeof(P) <= F::constraints::max_size &&
    alignof(P) <= F::constraints::max_align &&
    details::has_copyability<P>(F::constraints::copyability) &&
    details::has_relocatability<P>(F::constraints::relocatability) &&
    details::has_destructibility<P>(F::constraints::destructibility) &&
    details::facade_traits<F>::template conv_applicable_ptr<P> &&
    details::facade_traits<F>::template refl_applicable_ptr<P>;


namespace details{

  template <class T>
  class inplace_ptr;

  template <class T, class Alloc>
  class allocated_ptr;

  template <class T, class Alloc>
  class compact_ptr;


  template<class P>
  struct get_object_fn_collections;

  struct static_meta_manager{
    enum ptr_type{
      inplace, allocated, compact, raw
    };

    struct meta_info{
      std::byte *meta_ptr;
      std::size_t size;
      std::size_t align;
      static_type_token allocator;
      ptr_type type;

      void (*create_ptr_copy)(std::byte *dst, std::byte* obj, const std::byte *alloc);
      void (*create_ptr_move)(std::byte *dst, std::byte* obj, const std::byte *alloc);
      
      meta_info(): meta_ptr(nullptr), size(0), align(0), allocator(), type(inplace), create_ptr_copy(nullptr), create_ptr_move(nullptr){}
      template<class P>
      meta_info(std::byte *meta_ptr_, std::in_place_type_t<P>, const static_type_token& allocator_)
        : meta_ptr(meta_ptr_), 
        size(sizeof(typename get_object_fn_collections<P>::value_type)),
        align(alignof(typename get_object_fn_collections<P>::value_type)), 
        allocator(allocator_), 
        type(get_object_fn_collections<P>::type),
        create_ptr_copy(get_object_fn_collections<P>::get_copy_fn()),
        create_ptr_move(get_object_fn_collections<P>::get_move_fn()) {}
    };
    struct meta_key{
      static_type_token facade_type;
      static_type_token proxiable_type;

      template<class F, class P>
      meta_key(std::in_place_type_t<F>, std::in_place_type_t<P>): facade_type(static_type_token{std::in_place_type<F>}), proxiable_type(static_type_token{std::in_place_type<P>}){}
      meta_key(const static_type_token& facade, const static_type_token& prox):facade_type(facade), proxiable_type(prox){}

      bool operator ==(const meta_key& rhs) const noexcept{
        return facade_type == rhs.facade_type && proxiable_type == rhs.proxiable_type;
      }
    };
    struct type_token_hasher
    {
      std::size_t operator()(const meta_key& k) const
      {
        return static_cast<std::size_t>(k.facade_type) ^ static_cast<std::size_t>(k.proxiable_type);
      }
    };

    inline static std::unordered_map<meta_key, std::vector<meta_info>, type_token_hasher> meta_map{};

    template<class P, class F> inline static bool registered = false;
    template<class P, class F>
    static void register_facade_meta(){
      if(registered<P, F>) return;
      registered<P, F> = true;

      auto meta_ = meta_ptr<typename facade_traits<F>::meta>{std::in_place_type<P>};
      auto key = meta_key{std::in_place_type<F>, std::in_place_type<typename get_object_fn_collections<P>::value_type>};
      auto value = meta_info((std::byte*)meta_.get_ptr(), std::in_place_type<P>, static_type_token{std::in_place_type<typename get_object_fn_collections<P>::allocator>});

      decltype(meta_map)::iterator iter;

      if((iter = meta_map.find(key)) == meta_map.end()){
        meta_map[key] = std::vector<meta_info>{value};
      }else{
        (*iter).second.push_back(value);
      }

    }
    template<class T, class F>
    static void register_facade_inplace(){
      constexpr bool inplace_avail = pro::proxiable<pro::details::inplace_ptr<T>, F>;

      static_assert(inplace_avail, "Cannot find compatible inplace storage type for type T");

      register_facade_meta<pro::details::inplace_ptr<T>, F>();
    }
    template<class T, class F, class Alloc>
    static void register_facade_allocated(const Alloc&){
      constexpr bool allocated_avail = (pro::proxiable<pro::details::allocated_ptr<T, Alloc>, F>);
      constexpr bool compact_avail = (pro::proxiable<pro::details::compact_ptr<T, Alloc>, F>);

      static_assert(allocated_avail || compact_avail, "Cannot find compatible storage type for type T");

      if constexpr(allocated_avail){
        register_facade_meta<pro::details::allocated_ptr<T, Alloc>, F>();
      }else if constexpr(compact_avail){
        register_facade_meta<pro::details::compact_ptr<T, Alloc>, F>();
      } 
    }
  };


}  // namespace details


template<class F>
struct proxy_indirect_accessor_helpers{
  struct dummy_type{};
  template<class FF>
  static auto test(
    std::type_identity<FF> *,
    typename std::enable_if<!std::is_same_v<typename details::facade_traits<FF>
    ::indirect_accessor, details::composite_accessor_impl<>>, int>::type = 0) -> typename details::facade_traits<FF>::indirect_accessor;
  static auto test(...) -> dummy_type;

  using type = decltype(test((std::type_identity<F> *)nullptr));
};

template <class F> struct proxy_indirect_accessor: proxy_indirect_accessor_helpers<F>::type {};


template <class F>
class proxy : public details::facade_traits<F>::direct_accessor {
  static_assert(facade<F>);
  friend struct details::proxy_helper<F>;
  using _Traits = details::facade_traits<F>;

 public:
  proxy() noexcept {
    ___PRO_DEBUG(
      std::ignore = static_cast<proxy_indirect_accessor<F>*
          (proxy::*)() noexcept>(&proxy::operator->);
      std::ignore = static_cast<const proxy_indirect_accessor<F>*
          (proxy::*)() const noexcept>(&proxy::operator->);
      std::ignore = static_cast<proxy_indirect_accessor<F>&
          (proxy::*)() & noexcept>(&proxy::operator*);
      std::ignore = static_cast<const proxy_indirect_accessor<F>&
          (proxy::*)() const& noexcept>(&proxy::operator*);
      std::ignore = static_cast<proxy_indirect_accessor<F>&&
          (proxy::*)() && noexcept>(&proxy::operator*);
      std::ignore = static_cast<const proxy_indirect_accessor<F>&&
          (proxy::*)() const&& noexcept>(&proxy::operator*);
    )
  }
  proxy(std::nullptr_t) noexcept : proxy() {}
  proxy(const proxy& rhs)
      noexcept(F::constraints::copyability == constraint_level::nothrow) {
    static_assert(F::constraints::copyability > constraint_level::none, "Copy not supported for facade F");

    if constexpr(F::constraints::copyability == constraint_level::trivial){
      ia_ = rhs.ia_;
      meta_ = rhs.meta_;
      std::copy(rhs.ptr_, rhs.ptr_ + sizeof(rhs.ptr_), ptr_);
    }else{
      if (rhs.meta_.has_value()) {
        rhs.meta_->_Traits::copyability_meta::dispatcher(*ptr_, *rhs.ptr_);
        meta_ = rhs.meta_;
      }
    }
  }
  template<class SelfFacade>
  proxy(proxy<SelfFacade>&& rhs, 
    typename std::enable_if<std::is_same_v<SelfFacade, F>, int>::type = 0,
    typename std::enable_if<(SelfFacade::constraints::relocatability >= constraint_level::nontrivial &&
          SelfFacade::constraints::copyability != constraint_level::trivial), int>::type = 0)
      noexcept(F::constraints::relocatability == constraint_level::nothrow) {

    if (rhs.meta_.has_value()) {
      details::meta_ptr_reset_guard guard{rhs.meta_};
      if constexpr (F::constraints::relocatability ==
          constraint_level::trivial) {
        std::copy(rhs.ptr_, rhs.ptr_ + sizeof(rhs.ptr_), ptr_);
      } else {
        rhs.meta_->_Traits::relocatability_meta::dispatcher(*ptr_, *rhs.ptr_);
      }
      meta_ = rhs.meta_;
    }
  }

  template <class P>
  proxy(P&& ptr, 
    typename std::enable_if<!std::is_same_v<P, std::nullptr_t>, int>::type = 0, 
    typename std::enable_if<!details::is_in_place_type<std::decay_t<P>>, int>::type = 0,
    typename std::enable_if<proxiable<std::decay_t<P>, F>, int>::type = 0,
    typename std::enable_if<std::is_constructible_v<std::decay_t<P>, P>, int>::type = 0) 
    noexcept(std::is_nothrow_constructible_v<std::decay_t<P>, P>)
      : proxy() { 
        initialize<std::decay_t<P>>(std::forward<P>(ptr)); }
        
  template <typename P, class... Args>
  explicit proxy(std::in_place_type_t<P>, Args&&... args)
      noexcept(std::is_nothrow_constructible_v<P, Args...>)
      : proxy() { 
        static_assert(std::is_constructible_v<P, Args...>, "P should be able to construct with given arguments");
        static_assert(proxiable<P, F>, "P should proxiable as type F");
        initialize<P>(std::forward<Args>(args)...); 
      }
  template <typename P, class U, class... Args>
  explicit proxy(std::in_place_type_t<P>, std::initializer_list<U> il,
          Args&&... args)
      noexcept(std::is_nothrow_constructible_v<
          P, std::initializer_list<U>&, Args...>)
      : proxy() { 
        static_assert(std::is_constructible_v<P, std::initializer_list<U>&, Args...>, "P should be able to construct with given arguments");
        static_assert(proxiable<P, F>, "P should proxiable as type F");
        initialize<P>(il, std::forward<Args>(args)...); 
      }
  proxy& operator=(typename std::enable_if<F::constraints::destructibility >= constraint_level::nontrivial, std::nullptr_t>::type)
      noexcept(F::constraints::destructibility >= constraint_level::nothrow)
      { 
        reset(); return *this; 
  }
  proxy& operator=(const proxy& rhs)
      noexcept(F::constraints::copyability >= constraint_level::nothrow &&
          F::constraints::destructibility >= constraint_level::nothrow) {

    constexpr auto is_copy_trivial = F::constraints::copyability == constraint_level::trivial;
    constexpr auto is_copy_nontrivial = (F::constraints::copyability == constraint_level::nontrivial ||
      F::constraints::copyability == constraint_level::nothrow) &&
      F::constraints::destructibility >= constraint_level::nontrivial;

    static_assert(is_copy_trivial || is_copy_nontrivial, "Copy ability of F doesn't support copy");
    if constexpr(is_copy_trivial){
      ia_ = rhs.ia_;
      meta_ = rhs.meta_;
      std::copy(rhs.ptr_, rhs.ptr_ + sizeof(rhs.ptr_), ptr_);
    }else if constexpr(is_copy_nontrivial){
      if (this != &rhs) {
          if constexpr (F::constraints::copyability == constraint_level::nothrow) {
            std::destroy_at(this);
            std::construct_at(this, rhs);
          } else {
            *this = proxy{rhs};
          }
        }
      }
    return *this;
  }
  proxy& operator=(proxy&& rhs)
      noexcept(F::constraints::relocatability >= constraint_level::nothrow &&
          F::constraints::destructibility >= constraint_level::nothrow) {
    constexpr auto is_move_supported = F::constraints::relocatability >= constraint_level::nontrivial &&
          F::constraints::destructibility >= constraint_level::nontrivial &&
          F::constraints::copyability != constraint_level::trivial;
    static_assert(is_move_supported, "Move ability of F doesn't support move");

    if (this != &rhs) {
      reset();
      std::construct_at(this, std::move(rhs));
    }
    return *this;
  }
  template <class P>
  proxy& operator=(std::enable_if<proxiable<std::decay_t<P>, F> &&
          std::is_constructible_v<std::decay_t<P>, P> &&
          F::constraints::destructibility >= constraint_level::nontrivial,P&&> ptr)
      noexcept(std::is_nothrow_constructible_v<std::decay_t<P>, P> &&
          F::constraints::destructibility >= constraint_level::nothrow)
 {
    if constexpr (std::is_nothrow_constructible_v<std::decay_t<P>, P>) {
      std::destroy_at(this);
      initialize<std::decay_t<P>>(std::forward<P>(ptr));
    } else {
      *this = proxy{std::forward<P>(ptr)};
    }
    return *this;
  }
  ~proxy() noexcept(F::constraints::destructibility == constraint_level::nothrow)
          {
      if constexpr(F::constraints::destructibility == constraint_level::nontrivial ||
          F::constraints::destructibility == constraint_level::nothrow){
            if (meta_.has_value()) {
                  meta_->_Traits::destructibility_meta::dispatcher(*ptr_);
                }
          }
    
  }

  bool has_value() const noexcept { return meta_.has_value(); }
  explicit operator bool() const noexcept { return meta_.has_value(); }
  void reset()
      noexcept(F::constraints::destructibility >= constraint_level::nothrow)
      { 
        static_assert(F::constraints::destructibility >= constraint_level::nontrivial, "F doesn't support destruction");
        std::destroy_at(this); meta_.reset(); }
  void swap(proxy& rhs)
      noexcept(F::constraints::relocatability >= constraint_level::nothrow ||
          F::constraints::copyability == constraint_level::trivial) {
    static_assert(F::constraints::relocatability >= constraint_level::nontrivial ||
          F::constraints::copyability == constraint_level::trivial, "F doesn't support swap");
    if constexpr (F::constraints::relocatability == constraint_level::trivial ||
        F::constraints::copyability == constraint_level::trivial) {
      std::swap(meta_, rhs.meta_);
      std::swap(ptr_, rhs.ptr);
    } else {
      if (meta_.has_value()) {
        if (rhs.meta_.has_value()) {
          proxy temp = std::move(*this);
          std::construct_at(this, std::move(rhs));
          std::construct_at(&rhs, std::move(temp));
        } else {
          std::construct_at(&rhs, std::move(*this));
        }
      } else if (rhs.meta_.has_value()) {
        std::construct_at(this, std::move(rhs));
      }
    }
  }
  template <typename P, class... Args>
  P& emplace(Args&&... args)
      noexcept(std::is_nothrow_constructible_v<P, Args...> &&
          F::constraints::destructibility >= constraint_level::nothrow)
      { 
        static_assert(proxiable<P, F>, "P should be proxiable as type F");
        static_assert(std::is_constructible_v<P, Args...>, "P should be able to construct with given arguments");
        static_assert(F::constraints::destructibility >= constraint_level::nontrivial, "Desctuctibility of P should be at least non-trivial");

        reset(); return initialize<P>(std::forward<Args>(args)...); }
  template <typename P, class U, class... Args>
  P& emplace(std::initializer_list<U> il, Args&&... args)
      noexcept(std::is_nothrow_constructible_v<
          P, std::initializer_list<U>&, Args...> &&
          F::constraints::destructibility >= constraint_level::nothrow)
      { 
        static_assert(proxiable<P, F>, "P should be proxiable as type F");
        static_assert(std::is_constructible_v<P, std::initializer_list<U>&, Args...>, "P should be able to construct with given arguments");
        static_assert(F::constraints::destructibility >= constraint_level::nontrivial, "Desctuctibility of P should be at least non-trivial");

        reset(); return initialize<P>(il, std::forward<Args>(args)...); }
  proxy_indirect_accessor<F>* operator->() noexcept
      { return std::addressof(ia_); }
  const proxy_indirect_accessor<F>* operator->() const noexcept
      { return std::addressof(ia_); }
  proxy_indirect_accessor<F>& operator*() & noexcept { return ia_; }
  const proxy_indirect_accessor<F>& operator*() const& noexcept { return ia_; }
  proxy_indirect_accessor<F>&& operator*() && noexcept
      { return std::move(ia_); }
  const proxy_indirect_accessor<F>&& operator*() const&& noexcept
      { return std::forward<const proxy_indirect_accessor<F>>(ia_); }

  friend void swap(proxy& lhs, proxy& rhs) noexcept(noexcept(lhs.swap(rhs)))
      { lhs.swap(rhs); }
  friend bool operator==(const proxy& lhs, std::nullptr_t) noexcept
      { return !lhs.has_value(); }
  friend bool operator!=(const proxy& lhs, std::nullptr_t) noexcept
      { return lhs.has_value(); }
  friend bool operator==(std::nullptr_t, const proxy& lhs) noexcept
      { return !lhs.has_value(); }
  friend bool operator!=(std::nullptr_t, const proxy& lhs) noexcept
      { return lhs.has_value(); }

 public:
  template <class P, class... Args>
  P& initialize(Args&&... args) {
    P& result = *std::construct_at(
        reinterpret_cast<P*>(ptr_), std::forward<Args>(args)...);
    if constexpr (std::is_convertible_v<P, bool>)
        { assert((bool)result); }
    meta_ = details::meta_ptr<typename _Traits::meta>{std::in_place_type<P>};
    details::static_meta_manager::register_facade_meta<P, F>();
    return result;
  }

  [[___PRO_NO_UNIQUE_ADDRESS_ATTRIBUTE]]
  proxy_indirect_accessor<F> ia_;
  details::meta_ptr<typename _Traits::meta> meta_;
  alignas(F::constraints::max_align) std::byte ptr_[F::constraints::max_size];
};

template <bool IsDirect, class D, class O, class F, class... Args>
auto proxy_invoke(proxy<F>& p, Args&&... args)
    -> typename details::overload_traits<O>::return_type {
  return details::proxy_helper<F>::template invoke<IsDirect, D, O,
      details::qualifier_type::lv>(p, std::forward<Args>(args)...);
}
template <bool IsDirect, class D, class O, class F, class... Args>
auto proxy_invoke(const proxy<F>& p, Args&&... args)
    -> typename details::overload_traits<O>::return_type {
  return details::proxy_helper<F>::template invoke<IsDirect, D, O,
      details::qualifier_type::const_lv>(p, std::forward<Args>(args)...);
}
template <bool IsDirect, class D, class O, class F, class... Args>
auto proxy_invoke(proxy<F>&& p, Args&&... args)
    -> typename details::overload_traits<O>::return_type {
  return details::proxy_helper<F>::template invoke<
      IsDirect, D, O, details::qualifier_type::rv>(
      std::move(p), std::forward<Args>(args)...);
}
template <bool IsDirect, class D, class O, class F, class... Args>
auto proxy_invoke(const proxy<F>&& p, Args&&... args)
    -> typename details::overload_traits<O>::return_type {
  return details::proxy_helper<F>::template invoke<
      IsDirect, D, O, details::qualifier_type::const_rv>(
      std::move(p), std::forward<Args>(args)...);
}

template <bool IsDirect, class R, class F>
const R& proxy_reflect(const proxy<F>& p) noexcept {
  return static_cast<const details::refl_meta<IsDirect, R>&>(
      details::proxy_helper<F>::get_meta(p)).reflector;
}

template <class F, class A>
proxy<F>& access_proxy(A& a) noexcept {
  return details::proxy_helper<F>::template access<
      A, details::qualifier_type::lv>(a);
}
template <class F, class A>
const proxy<F>& access_proxy(const A& a) noexcept {
  return details::proxy_helper<F>::template access<
      A, details::qualifier_type::const_lv>(a);
}
template <class F, class A>
proxy<F>&& access_proxy(A&& a) noexcept {
  return details::proxy_helper<F>::template access<
      A, details::qualifier_type::rv>(std::forward<A>(a));
}
template <class F, class A>
const proxy<F>&& access_proxy(const A&& a) noexcept {
  return details::proxy_helper<F>::template access<
      A, details::qualifier_type::const_rv>(std::forward<const A>(a));
}

namespace details {

template <class T>
class inplace_ptr {
 public:
  using type = T;
  using allocator = void;
  template <class... Args>
  inplace_ptr(Args&&... args)
      noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : value_(std::forward<Args>(args)...) {
        static_assert(std::is_constructible_v<T, Args...>, "Unable to construct T with given arguments");
      }
  inplace_ptr(const inplace_ptr&)
      noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
  inplace_ptr(inplace_ptr&&)
      noexcept(std::is_nothrow_move_constructible_v<T>) = default;

  T* operator->() noexcept { return &value_; }
  const T* operator->() const noexcept { return &value_; }
  T& operator*() & noexcept { return value_; }
  const T& operator*() const& noexcept { return value_; }
  T&& operator*() && noexcept { return std::forward<T>(value_); }
  const T&& operator*() const&& noexcept
      { return std::forward<const T>(value_); }

 private:
  T value_;
};

#if __STDC_HOSTED__
template <class T, class Alloc>
static auto rebind_allocator(const Alloc& alloc) {
  return typename std::allocator_traits<Alloc>::template rebind_alloc<T>(alloc);
}
template <class T, class Alloc, class... Args>
static T* allocate(const Alloc& alloc, Args&&... args) {
  auto al = rebind_allocator<T>(alloc);
  auto deleter = [&](T* ptr) { al.deallocate(ptr, 1); };
  std::unique_ptr<T, decltype(deleter)> result{al.allocate(1), deleter};
  std::construct_at(result.get(), std::forward<Args>(args)...);
  return result.release();
}
template <class Alloc, class T>
static void deallocate(const Alloc& alloc, T* ptr) {
  auto al = rebind_allocator<T>(alloc);
  std::destroy_at(ptr);
  al.deallocate(ptr, 1);
}

template <class T, class Alloc>
class allocated_ptr {
 public:
  using type = T;
  using allocator = Alloc;
  template <class... Args>
  allocated_ptr(const Alloc& alloc, Args&&... args)
      : alloc_(alloc), ptr_(allocate<T>(alloc, std::forward<Args>(args)...)) {
        static_assert(std::is_constructible_v<T, Args...>, "Unable to construct T with given arguments");
      }
  allocated_ptr(const allocated_ptr& rhs)
      : alloc_(rhs.alloc_), ptr_(rhs.ptr_ == nullptr ? nullptr :
            allocate<T>(alloc_, std::as_const(*rhs.ptr_))) {

              static_assert(std::is_copy_constructible_v<T>, "T doesn't support copy");
            }
  allocated_ptr(allocated_ptr&& rhs)
      noexcept(std::is_nothrow_move_constructible_v<Alloc>)
      : alloc_(std::move(rhs.alloc_)), ptr_(std::exchange(rhs.ptr_, nullptr)) {}
  ~allocated_ptr() { if (ptr_ != nullptr) { deallocate(alloc_, ptr_); } }

  T* operator->() noexcept { return ptr_; }
  const T* operator->() const noexcept { return ptr_; }
  T& operator*() & noexcept { return *ptr_; }
  const T& operator*() const& noexcept { return *ptr_; }
  T&& operator*() && noexcept { return std::forward<T>(*ptr_); }
  const T&& operator*() const&& noexcept
      { return std::forward<const T>(*ptr_); }

 private:
  [[___PRO_NO_UNIQUE_ADDRESS_ATTRIBUTE]]
  Alloc alloc_;
  T* ptr_;
};
template <class T, class Alloc>
class compact_ptr {
 public:
  using type = T;
  using allocator = Alloc;
  template <class... Args>
  compact_ptr(const Alloc& alloc, Args&&... args)
      : ptr_(allocate<storage>(alloc, alloc, std::forward<Args>(args)...)) {
        static_assert(std::is_constructible_v<T, Args...>, "Unable to construct T with given args");
      }
  compact_ptr(const compact_ptr& rhs)
      : ptr_(rhs.ptr_ == nullptr ? nullptr : allocate<storage>(rhs.ptr_->alloc,
            rhs.ptr_->alloc, std::as_const(rhs.ptr_->value))) {

            static_assert(std::is_copy_constructible_v<T>, "T doesn't support copy");
            }
  compact_ptr(compact_ptr&& rhs) noexcept
      : ptr_(std::exchange(rhs.ptr_, nullptr)) {}
  ~compact_ptr() { if (ptr_ != nullptr) { deallocate(ptr_->alloc, ptr_); } }

  T* operator->() noexcept { return &ptr_->value; }
  const T* operator->() const noexcept { return &ptr_->value; }
  T& operator*() & noexcept { return ptr_->value; }
  const T& operator*() const& noexcept { return ptr_->value; }
  T&& operator*() && noexcept { return std::forward<T>(ptr_->value); }
  const T&& operator*() const&& noexcept
      { return std::forward<const T>(ptr_->value); }

 private:
  struct storage {
    template <class... Args>
    explicit storage(const Alloc& alloc, Args&&... args)
        : value(std::forward<Args>(args)...), alloc(alloc) {}

    T value;
    Alloc alloc;
  };

  storage* ptr_;
};
template <class F, class T, class Alloc, class... Args>
proxy<F> allocate_proxy_impl(const Alloc& alloc, Args&&... args) {
  if constexpr (proxiable<allocated_ptr<T, Alloc>, F>) {
    return proxy<F>{std::in_place_type<allocated_ptr<T, Alloc>>,
        alloc, std::forward<Args>(args)...};
  } else {
    return proxy<F>{std::in_place_type<compact_ptr<T, Alloc>>,
        alloc, std::forward<Args>(args)...};
  }
}
template <class F, class T, class... Args>
proxy<F> make_proxy_impl(Args&&... args) {
  if constexpr (proxiable<inplace_ptr<T>, F>) {
    return proxy<F>{std::in_place_type<inplace_ptr<T>>,
        std::forward<Args>(args)...};
  } else {
    return allocate_proxy_impl<F, T>(
        std::allocator<T>{}, std::forward<Args>(args)...);
  }
}
#endif  // __STDC_HOSTED__

template<class P>
struct get_object_fn_collections<pro::details::inplace_ptr<P>>{
  using value_type = P;
  using allocator = void;
  static std::byte* get_ptr(std::byte* ptrs){
    return (std::byte*)(&**(pro::details::inplace_ptr<P> *)ptrs);
  }
  static constexpr void (*get_copy_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_copy_constructible_v<P>){
      return &create_ptr_copy;
    }
    return nullptr;
  }
  static constexpr void (*get_move_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_move_constructible_v<P>){
      return &create_ptr_move;
    }
    return nullptr;
  }
  static void create_ptr_copy(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::inplace_ptr<P>((const P&)*(const P*)obj);
  }
  static void create_ptr_move(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::inplace_ptr<P>((P&&)*(P*)obj);
  }
  static constexpr static_meta_manager::ptr_type type = static_meta_manager::ptr_type::inplace;
};
template<class P, class Alloc>
struct get_object_fn_collections<pro::details::allocated_ptr<P, Alloc>>{
  using value_type = P;
  using allocator = Alloc;
  static std::byte* get_ptr(std::byte* ptrs){
    return (std::byte*)(&**(pro::details::allocated_ptr<P, Alloc> *)ptrs);
  }
  static constexpr void (*get_copy_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_copy_constructible_v<P>){
      return &create_ptr_copy;
    }
    return nullptr;
  }
  static constexpr void (*get_move_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_move_constructible_v<P>){
      return &create_ptr_move;
    }
    return nullptr;
  }
  static void create_ptr_copy(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::allocated_ptr<P, Alloc>((const Alloc&)*(const Alloc*)alloc,(const P&)*(P*)obj);
  }
  static void create_ptr_move(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::allocated_ptr<P, Alloc>((const Alloc&)*(const Alloc*)alloc,(P&&)*(P*)obj);
  }
  static constexpr static_meta_manager::ptr_type type = static_meta_manager::ptr_type::allocated;
};
template<class P, class Alloc>
struct get_object_fn_collections<pro::details::compact_ptr<P, Alloc>>{
  using value_type = P;
  using allocator = Alloc;
  static std::byte* get_ptr(std::byte* ptrs){
    return (std::byte*)(&**(pro::details::compact_ptr<P, Alloc> *)ptrs);
  }
  static constexpr void (*get_copy_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_copy_constructible_v<P>){
      return &create_ptr_copy;
    }
    return nullptr;
  }
  static constexpr void (*get_move_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_move_constructible_v<P>){
      return &create_ptr_move;
    }
    return nullptr;
  }
  static void create_ptr_copy(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::compact_ptr<P, Alloc>((const Alloc&)*(const Alloc*)alloc,(const P&)*(P*)obj);
  }
  static void create_ptr_move(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) pro::details::compact_ptr<P, Alloc>((const Alloc&)*(const Alloc*)alloc,(P&&)*(P*)obj);
  }
  static constexpr static_meta_manager::ptr_type type = static_meta_manager::ptr_type::compact;
};

template<class P>
struct get_object_fn_collections<P *>{
  using value_type = P *;
  using allocator = void;
  static std::byte* get_ptr(std::byte* ptrs){
    return (std::byte*)(*(P **)ptrs);
  }
  static constexpr void (*get_copy_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    return &create_ptr_copy;
  }
  static constexpr void (*get_move_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    return nullptr;
  }
  static void create_ptr_copy(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    *(P**)dst = *(P**)obj;
  }
  static auto create_ptr_move([[maybe_unused]]std::byte *dst, [[maybe_unused]]std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    
  }
  static constexpr static_meta_manager::ptr_type type = static_meta_manager::ptr_type::raw;
};

template<typename P>
struct get_object_fn_collections{
  using value_type = P;
  using allocator = void;
  static std::byte* get_ptr(std::byte* ptrs){
    return (std::byte*)((P *)ptrs);
  }
  static constexpr void (*get_copy_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_copy_constructible_v<P>){
      return &create_ptr_copy;
    }
    return nullptr;
  }
  static constexpr void (*get_move_fn())(std::byte *dst, std::byte* obj, const std::byte *alloc){
    if constexpr(std::is_move_constructible_v<P>){
      return &create_ptr_move;
    }
    return nullptr;
  }
  static void create_ptr_copy(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) P((const P&)*(const P*)obj);
  }
  static void create_ptr_move(std::byte *dst, std::byte* obj, [[maybe_unused]] const std::byte *alloc){
    new(dst) P((P&&)*(P*)obj);
  }
  static constexpr static_meta_manager::ptr_type type = static_meta_manager::ptr_type::raw;
};

struct poly_cast_meta{
  constexpr poly_cast_meta() noexcept :proxiable_type(), addr_fn(nullptr) {}
  using get_object_addr_fn = std::byte *(std::byte *);

  template<class P>
  constexpr static get_object_addr_fn* get_object_fn(){
    return &get_object_fn_collections<P>::get_ptr;
  }

  template <class P>
  constexpr explicit poly_cast_meta(std::in_place_type_t<P>) noexcept :proxiable_type(std::in_place_type<typename get_object_fn_collections<P>::value_type>), addr_fn(get_object_fn<P>()) {
  }

  template<class T, class F>
  T remove_proxy(pro::proxy<F>&& proxy) const noexcept{
    auto obj_addr = (T *)addr_fn(proxy.ptr_);

    T value {std::move(*obj_addr)};
    proxy.reset();

    return value;
  }

  template<class NF, class F, class Alloc = std::nullptr_t>
  std::optional<pro::proxy<NF>> cast_copy([[maybe_unused]]pro::proxy<F>& proxy, [[maybe_unused]]std::optional<Alloc> allocator = std::optional<Alloc>()) const noexcept{
    pro::proxy<NF> new_proxy{};

    auto key = static_meta_manager::meta_key(static_type_token{std::in_place_type<NF>}, proxiable_type);

    auto& meta_table = static_meta_manager::meta_map;
    decltype(static_meta_manager::meta_map)::iterator iter;
    if((iter = meta_table.find(key)) == meta_table.end()){
      return std::optional<pro::proxy<NF>>();
    }
  
    auto& info = (*iter).second;

    auto obj_addr = addr_fn(proxy.ptr_);

    static_type_token allocator_token{std::in_place_type<Alloc>};

    for(auto& i:info){
      if(i.type == static_meta_manager::ptr_type::inplace || i.allocator == allocator_token){
        if(i.create_ptr_copy != nullptr){
          i.create_ptr_copy(new_proxy.ptr_, obj_addr, (const std::byte*)&allocator.value());

          new_proxy.meta_ = pro::details::meta_ptr<typename facade_traits<NF>::meta>(i.meta_ptr);
          return std::optional<pro::proxy<NF>>(std::move(new_proxy));
        }
      }
    }
    return std::optional<pro::proxy<NF>>();
  }

  template<class NF, class F, class Alloc = std::nullptr_t>
  std::optional<pro::proxy<NF>> cast_move([[maybe_unused]]pro::proxy<F>& proxy, [[maybe_unused]] const std::optional<Alloc>& allocator = std::optional<Alloc>()) const noexcept{
    pro::proxy<NF> new_proxy{};

    auto key = static_meta_manager::meta_key(static_type_token{std::in_place_type<NF>}, proxiable_type);

    auto& meta_table = static_meta_manager::meta_map;
    decltype(static_meta_manager::meta_map)::iterator iter;
    if((iter = meta_table.find(key)) == meta_table.end()){
      return std::optional<pro::proxy<NF>>();
    }
  
    auto& info = (*iter).second;

    auto obj_addr = addr_fn(proxy.ptr_);

    static_type_token allocator_token{std::in_place_type<Alloc>};

    for(auto& i:info){
      if(i.type == static_meta_manager::ptr_type::inplace || i.allocator == allocator_token){
        if(i.create_ptr_move == nullptr){
          i.create_ptr_move(new_proxy.ptr_, obj_addr, (const std::byte*)&allocator.value());

          new_proxy.meta_ = pro::details::meta_ptr<typename facade_traits<NF>::meta>(i.meta_ptr);

          proxy.reset();

          return std::optional<pro::proxy<NF>>(std::move(new_proxy));
        }
      }
    }
    return std::optional<pro::proxy<NF>>();
  }

  const static_type_token proxiable_type;
  get_object_addr_fn *addr_fn;
};



}  // namespace details

template <class T, class F>
inline constexpr bool inplace_proxiable_target = proxiable<details::inplace_ptr<T>, F>;

template <typename F, typename T, class... Args>
proxy<F> make_proxy_inplace(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>) {
  static_assert(facade<F>, "F should be a valid facade");
  static_assert(inplace_proxiable_target<T, F>, "T should be inplace proxiable as type F");
  return proxy<F>{std::in_place_type<details::inplace_ptr<T>>,
      std::forward<Args>(args)...};
}
template <typename F, typename T, class U, class... Args>
proxy<F> make_proxy_inplace(std::initializer_list<U> il, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<
        T, std::initializer_list<U>&, Args...>) {
  static_assert(facade<F>, "F should be a valid facade");
  static_assert(inplace_proxiable_target<T, F>, "T should be inplace proxiable as type F");
  return proxy<F>{std::in_place_type<details::inplace_ptr<T>>,
      il, std::forward<Args>(args)...};
}
template <typename F, class T>
proxy<F> make_proxy_inplace(T&& value)
    noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
  static_assert(facade<F>, "F should be a valid facade");
  static_assert(inplace_proxiable_target<std::decay_t<T>, F>, "T should match form of F");
  return proxy<F>{std::in_place_type<details::inplace_ptr<std::decay_t<T>>>,
      std::forward<T>(value)};
}

#if __STDC_HOSTED__
template<class T, class F>
T remove_proxy(pro::proxy<F>&& proxy){
    return proxy.meta_->poly_cast_meta::template remove_proxy<T, F>(proxy);
}

/*template<class NF, class F, class Alloc = std::nullptr_t>
std::optional<pro::proxy<NF>> cast_copy([[maybe_unused]]pro::proxy<F>& proxy, std::optional<Alloc> allocator = std::optional<Alloc>()) const noexcept{
  
}*/

template <typename F, class T, class Alloc, class... Args>
proxy<F> allocate_proxy(const Alloc& alloc, Args&&... args) {
  static_assert(facade<F>, "F should be a valid facade");
  return details::allocate_proxy_impl<F, T>(alloc, std::forward<Args>(args)...);
}
template <typename F, class T, class Alloc, class U,
    class... Args>
proxy<F> allocate_proxy(const Alloc& alloc, std::initializer_list<U> il,
    Args&&... args) {
  static_assert(facade<F>, "F should be a valid facade");
  return details::allocate_proxy_impl<F, T>(
      alloc, il, std::forward<Args>(args)...);
}
template <typename F, class Alloc, class T>
proxy<F> allocate_proxy(const Alloc& alloc, T&& value) {
  static_assert(facade<F>, "F should be a valid facade");
  return details::allocate_proxy_impl<F, std::decay_t<T>>(
      alloc, std::forward<T>(value));
}
template <typename F, class T, class... Args>
proxy<F> make_proxy(Args&&... args)
    { 
        static_assert(facade<F>, "F should be a valid facade");
      return details::make_proxy_impl<F, T>(std::forward<Args>(args)...); 
    }
template <typename F, class T, class U, class... Args>
proxy<F> make_proxy(std::initializer_list<U> il, Args&&... args)
    { 
        static_assert(facade<F>, "F should be a valid facade");
      return details::make_proxy_impl<F, T>(il, std::forward<Args>(args)...); 
      }
template <typename F, class T>
proxy<F> make_proxy(T&& value) {
    static_assert(facade<F>, "F should be a valid facade");
  return details::make_proxy_impl<F, std::decay_t<T>>(std::forward<T>(value));
}
#endif  // __STDC_HOSTED__

template <class F>
struct observer_facade;

template <class F>
using proxy_view = proxy<observer_facade<F>>;

template<class... Os>
struct va_template_first;

template<class Head, class... Os>
struct va_template_first<Head, Os...>{
    using type = Head;
};

#define ___PRO_DIRECT_FUNC_IMPL(...) \
    noexcept(noexcept(__VA_ARGS__))  \
    -> decltype(__VA_ARGS__) \
    { return __VA_ARGS__; }


#define ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(__MACRO, ...) \
    template <class __F, bool __IsDirect, class __D, class __O> \
    struct accessor_single; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    struct accessor_multi; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    using accessor = typename std::conditional<(sizeof...(__Os) > 1u && \
             (::std::is_constructible_v<accessor_single<__F, __IsDirect, __D, __Os>> && \
              ...)), accessor_multi<__F, __IsDirect, __D, __Os...>, accessor_single<__F, __IsDirect, __D, typename pro::va_template_first<__Os...>::type>>::type; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    struct accessor_multi \
        : accessor_single<__F, __IsDirect, __D, __Os>... \
        { using accessor_single<__F, __IsDirect, __D, __Os>::__VA_ARGS__...; }; \
    __MACRO(, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(noexcept, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(&, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(& noexcept, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(&&, ::pro::access_proxy<__F>(::std::forward<accessor_single>(*this)), \
        __VA_ARGS__); \
    __MACRO(&& noexcept, ::pro::access_proxy<__F>( \
        ::std::forward<accessor_single>(*this)), __VA_ARGS__); \
    __MACRO(const, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(const noexcept, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(const&, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(const& noexcept, ::pro::access_proxy<__F>(*this), __VA_ARGS__); \
    __MACRO(const&&, ::pro::access_proxy<__F>( \
        ::std::forward<const accessor_single>(*this)), __VA_ARGS__); \
    __MACRO(const&& noexcept, ::pro::access_proxy<__F>( \
        ::std::forward<const accessor_single>(*this)), __VA_ARGS__);

#define ___PRO_ADL_ARG ::pro::details::adl_accessor_arg_t<__F, __IsDirect>
#define ___PRO_DEF_FREE_ACCESSOR_TEMPLATE(__MACRO, ...) \
    template <class __F, bool __IsDirect, class __D, class __O> \
    struct accessor_single; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    struct accessor_multi; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    using accessor = typename std::conditional<(sizeof...(__Os) > 1u \
                                  && (::std::is_constructible_v<accessor_single< \
                                        __F, __IsDirect, __D, __Os>> && ...)), \
        accessor_multi<__F, __IsDirect, __D, __Os...>, \
        accessor_single<__F, __IsDirect, __D, \
          typename pro::va_template_first<__Os...>::type>>::type; \
    template <class __F, bool __IsDirect, class __D, class... __Os> \
    struct accessor_multi \
        : accessor_single<__F, __IsDirect, __D, __Os>... {}; \
    __MACRO(,, ___PRO_ADL_ARG& __self, ::pro::access_proxy<__F>(__self), \
        __VA_ARGS__); \
    __MACRO(noexcept, noexcept, ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(&,, ___PRO_ADL_ARG& __self, ::pro::access_proxy<__F>(__self), \
        __VA_ARGS__); \
    __MACRO(& noexcept, noexcept, ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(&&,, ___PRO_ADL_ARG&& __self, ::pro::access_proxy<__F>( \
        ::std::forward<decltype(__self)>(__self)), __VA_ARGS__); \
    __MACRO(&& noexcept, noexcept, ___PRO_ADL_ARG&& __self, \
        ::pro::access_proxy<__F>(::std::forward<decltype(__self)>(__self)), \
        __VA_ARGS__); \
    __MACRO(const,, const ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(const noexcept, noexcept, const ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(const&,, const ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(const& noexcept, noexcept, const ___PRO_ADL_ARG& __self, \
        ::pro::access_proxy<__F>(__self), __VA_ARGS__); \
    __MACRO(const&&,, const ___PRO_ADL_ARG&& __self, ::pro::access_proxy<__F>( \
        ::std::forward<decltype(__self)>(__self)), __VA_ARGS__); \
    __MACRO(const&& noexcept, noexcept, const ___PRO_ADL_ARG&& __self, \
        ::pro::access_proxy<__F>(::std::forward<decltype(__self)>(__self)), \
        __VA_ARGS__);

#define ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(...) \
    ___PRO_DEBUG( \
        accessor_single() noexcept { ::std::ignore = &accessor_single::__VA_ARGS__; })

#ifdef __cpp_rtti
class bad_proxy_cast : public std::bad_cast {
 public:
  bad_proxy_cast() noexcept = default;
  char const* what() const noexcept override { return "pro::bad_proxy_cast"; }
};
#endif  // __cpp_rtti

namespace details {

template <class F, bool IsDirect>
using adl_accessor_arg_t =
    std::conditional_t<IsDirect, proxy<F>, proxy_indirect_accessor<F>>;

#define ___PRO_DEF_CAST_ACCESSOR(Q, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class T> \
    struct accessor_single<__F, __IsDirect, __D, T() Q> { \
      ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(operator T) \
      explicit operator T() Q { \
        if constexpr (Nullable) { \
          if (!SELF.has_value()) { return nullptr; } \
        } \
        return proxy_invoke<__IsDirect, __D, T() Q>(SELF); \
      }\
    }
template <bool Expl, bool Nullable>
struct cast_dispatch_base {
  ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_CAST_ACCESSOR,
      operator typename overload_traits<__Os>::return_type)
};
#undef ___PRO_DEF_CAST_ACCESSOR

struct upward_conversion_dispatch : cast_dispatch_base<false, true> {
  template <class T>
  T&& operator()(T&& self) noexcept { return std::forward<T>(self); }
};

template <class T>
struct explicit_conversion_adapter {
  explicit explicit_conversion_adapter(T&& value) noexcept
      : value_(std::forward<T>(value)) {}
  explicit_conversion_adapter(const explicit_conversion_adapter&) = delete;

  template <class U>
  operator typename std::enable_if<std::is_constructible_v<U, T>, U>::type() noexcept(std::is_nothrow_constructible_v<U, T>)
      { 
        return U{std::forward<T>(value_)}; }

 private:
  T&& value_;
};

constexpr std::size_t invalid_size = std::numeric_limits<std::size_t>::max();
constexpr constraint_level invalid_cl = static_cast<constraint_level>(
    std::numeric_limits<std::underlying_type_t<constraint_level>>::min());

template<auto src, auto cond, auto nv>
static constexpr auto substitute_if = (src == cond) ? nv : src;

template<typename C>
using normalize_t = proxiable_ptr_constraints_t<
  substitute_if<C::max_size, invalid_size, sizeof(ptr_prototype)>,
  substitute_if<C::max_align, invalid_size, alignof(ptr_prototype)>,
  substitute_if<C::copyability, invalid_cl, constraint_level::none>,
  substitute_if<C::relocatability, invalid_cl, constraint_level::nothrow>,
  substitute_if<C::destructibility, invalid_cl, constraint_level::nothrow>
>;

constexpr auto normalize(proxiable_ptr_constraints value) {
  if (value.max_size == invalid_size)
      { value.max_size = sizeof(ptr_prototype); }
  if (value.max_align == invalid_size)
      { value.max_align = alignof(ptr_prototype); }
  if (value.copyability == invalid_cl)
      { value.copyability = constraint_level::none; }
  if (value.relocatability == invalid_cl)
      { value.relocatability = constraint_level::nothrow; }
  if (value.destructibility == invalid_cl)
      { value.destructibility = constraint_level::nothrow; }
  return value;
}
template<auto a, auto b>
static constexpr auto static_min = a > b ? b : a;
template<auto a, auto b>
static constexpr auto static_max = a > b ? b : a;

template<typename A, std::size_t max_size, std::size_t max_align>
using make_restricted_layout_t = proxiable_ptr_constraints_t<
  static_min<A::max_size, max_size>,
  static_min<A::max_align, max_align>,
  A::copyability, A::relocatability, A::destructibility
>;

constexpr auto make_restricted_layout(proxiable_ptr_constraints value,
    std::size_t max_size, std::size_t max_align) {
  if (value.max_size > max_size) { value.max_size = max_size; }
  if (value.max_align > max_align) { value.max_align = max_align; }
  return value;
}
constexpr auto make_copyable(proxiable_ptr_constraints value,
    constraint_level cl) {
  if (value.copyability < cl) { value.copyability = cl; }
  return value;
}
constexpr auto make_relocatable(proxiable_ptr_constraints value,
    constraint_level cl) {
  if (value.relocatability < cl) { value.relocatability = cl; }
  return value;
}
constexpr auto make_destructible(proxiable_ptr_constraints value,
    constraint_level cl) {
  if (value.destructibility < cl) { value.destructibility = cl; }
  return value;
}

template<typename A, typename B>
using merge_constraints_t = proxiable_ptr_constraints_t<
  static_min<A::max_size, B::max_size>, 
  static_min<A::max_align, B::max_align>, 
  static_max<A::copyability, B::copyability>,
  static_max<A::relocatability, B::relocatability>,
  static_max<A::destructibility, B::destructibility>>;

constexpr auto merge_constraints(proxiable_ptr_constraints a,
    proxiable_ptr_constraints b) {
  a = make_restricted_layout(a, b.max_size, b.max_align);
  a = make_copyable(a, b.copyability);
  a = make_relocatable(a, b.relocatability);
  a = make_destructible(a, b.destructibility);
  return a;
}
constexpr std::size_t max_align_of(std::size_t value) {
  value &= ~value + 1u;
  return value < alignof(std::max_align_t) ? value : alignof(std::max_align_t);
}

template <class SFINAE, class T, class F, bool IsDirect, class... Args>
struct accessor_instantiation_traits : std::type_identity<void> {};
template <class T, class F, bool IsDirect, class... Args>
struct accessor_instantiation_traits<std::void_t<typename T::template accessor<
    F, IsDirect, T, Args...>>, T, F, IsDirect, Args...>
    : std::type_identity<typename T::template accessor<
          F, IsDirect, T, Args...>> {};
template <class T, class F, bool IsDirect, class... Args>
using instantiated_accessor_t =
    typename accessor_instantiation_traits<void, T, F, IsDirect, Args...>::type;

template <bool IsDirect, class D, class... Os>
struct conv_impl {
  static constexpr bool is_direct = IsDirect;
  using dispatch_type = D;
  using overload_types = std::tuple<Os...>;
  template <class F>
  using accessor = instantiated_accessor_t<D, F, IsDirect, Os...>;
};
template <bool IsDirect, class R>
struct refl_impl {
  static constexpr bool is_direct = IsDirect;
  using reflector_type = R;
  template <class F>
  using accessor = instantiated_accessor_t<R, F, IsDirect>;
};
template <class Cs, class Rs, typename C>
struct facade_impl {
  using convention_types = Cs;
  using reflection_types = Rs;
  using constraints = C;
};

template<class T, class U>
struct is_nonexist_in_tuple;


template <class O, class I>
struct add_tuple_reduction;
template <class... Os, class I>
struct add_tuple_reduction<std::tuple<Os...>, I>
    : std::type_identity<
      typename std::conditional< ((!std::is_same_v<I, Os>) && ...), std::tuple<Os..., I>, std::tuple<Os...>>::type
    > {};


template <class T, class U>
using add_tuple_t = typename add_tuple_reduction<T, U>::type;
template <class O, class... Is>
using merge_tuple_impl_t = recursive_reduction_t<add_tuple_t, O, Is...>;
template <class T, class U>
using merge_tuple_t = instantiated_t<merge_tuple_impl_t, U, T>;

template <bool IsDirect, class D>
struct merge_conv_traits
    { template <class... Os> using type = conv_impl<IsDirect, D, Os...>; };
template <class C1, class C2>
using merge_conv_t = instantiated_t<
    merge_conv_traits<C1::is_direct, typename C1::dispatch_type>::template type,
    merge_tuple_t<typename C1::overload_types, typename C2::overload_types>>;

template <class Cs1, class C2, class C> struct add_conv_reduction;

template <class... Cs1, class C2, class... Cs3, class C>
struct add_conv_reduction<std::tuple<Cs1...>, std::tuple<C2, Cs3...>, C>
    : std::conditional<(C::is_direct == C2::is_direct && std::is_same_v<
        typename C::dispatch_type, typename C2::dispatch_type>), 
        std::type_identity<std::tuple<Cs1..., merge_conv_t<C2, C>, Cs3...>>,
        add_conv_reduction<std::tuple<Cs1..., C2>, std::tuple<Cs3...>, C>>::type {};

template <class... Cs, class C>
struct add_conv_reduction<std::tuple<Cs...>, std::tuple<>, C>
    : std::type_identity<std::tuple<Cs..., merge_conv_t<
          conv_impl<C::is_direct, typename C::dispatch_type>, C>>> {};



template <class Cs, class C>
using add_conv_t = typename add_conv_reduction<std::tuple<>, Cs, C>::type;

template <class F, constraint_level CL>
using copy_conversion_overload =
    proxy<F>() const& noexcept(CL >= constraint_level::nothrow);
template <class F, constraint_level CL>
using move_conversion_overload =
    proxy<F>() && noexcept(CL >= constraint_level::nothrow);
template <class Cs, class F, constraint_level CCL, constraint_level RCL>
struct add_upward_conversion_conv
    : std::type_identity<add_conv_t<Cs, conv_impl<true,
          upward_conversion_dispatch, copy_conversion_overload<F, CCL>,
          move_conversion_overload<F, RCL>>>> {};
template <class Cs, class F, constraint_level RCL>
struct add_upward_conversion_conv<Cs, F, constraint_level::none, RCL>
    : std::type_identity<add_conv_t<Cs, conv_impl<true,
          upward_conversion_dispatch, move_conversion_overload<F, RCL>>>> {};
template <class Cs, class F, constraint_level CCL>
struct add_upward_conversion_conv<Cs, F, CCL, constraint_level::none>
    : std::type_identity<add_conv_t<Cs, conv_impl<true,
          upward_conversion_dispatch, copy_conversion_overload<F, CCL>>>> {};
template <class Cs, class F>
struct add_upward_conversion_conv<
    Cs, F, constraint_level::none, constraint_level::none>
    : std::type_identity<Cs> {};

template <class Cs1, class... Cs2>
using merge_conv_tuple_t = recursive_reduction_t<add_conv_t, Cs1, Cs2...>;
template <class Cs, class F, bool WithUpwardConversion>
using merge_facade_conv_t = typename add_upward_conversion_conv<
    instantiated_t<merge_conv_tuple_t, typename F::convention_types, Cs>, F,
    WithUpwardConversion ? F::constraints::copyability : constraint_level::none,
    (WithUpwardConversion &&
        F::constraints::copyability != constraint_level::trivial) ?
        F::constraints::relocatability : constraint_level::none>::type;

struct proxy_view_dispatch : cast_dispatch_base<false, true> {
  template <class T>
  auto operator()(T&& value)
      ___PRO_DIRECT_FUNC_IMPL(std::addressof(*std::forward<T>(value)))
};

template <class P> struct facade_of_traits;
template <class F>
struct facade_of_traits<proxy<F>> : std::type_identity<F> {};
template <class P> using facade_of_t = typename facade_of_traits<P>::type;

template <class F, bool IsDirect, class D, class O>
struct observer_overload_mapping_traits_impl
    : std::conditional<(IsDirect && (!std::is_same_v<D, proxy_view_dispatch> ||
        std::is_same_v<typename overload_traits<O>::return_type, proxy_view<F>>)), 
    std::type_identity<void>, 
    std::type_identity<typename overload_traits<O>::view_type>>::type {};

template <class F, bool IsDirect, class D, class O>
struct observer_overload_mapping_traits_helpers{
  template<class FF>
  static auto test(
    FF*,
    typename std::enable_if<(overload_traits<O>::qualifier ==
        (std::is_const_v<FF> ? qualifier_type::const_lv : qualifier_type::lv)), int>::type = 0)
         -> observer_overload_mapping_traits_impl<F, IsDirect, D, O>;

  template<class FF>
  static auto test(
    std::type_identity<FF> *,
    typename std::enable_if<IsDirect, int>::type = 0,
    typename std::enable_if<std::is_same_v<D, upward_conversion_dispatch>, int>::type = 0
  ) -> std::type_identity<proxy_view<std::conditional_t<std::is_const_v<F>,
          const facade_of_t<typename overload_traits<O>::return_type>,
          facade_of_t<typename overload_traits<O>::return_type>>>()
          const noexcept>;

  static auto test(...) -> std::type_identity<void>;

  using type = decltype(test((std::type_identity<F> *)nullptr));
};

template <class F, bool IsDirect, class D, class O>
struct observer_overload_mapping_traits : observer_overload_mapping_traits_helpers<F, IsDirect, D, O>::type {};



template <class D>
struct observer_dispatch_reduction : std::type_identity<D> {};
template <>
struct observer_dispatch_reduction<upward_conversion_dispatch>
    : std::type_identity<proxy_view_dispatch> {};

template <class O, class I>
struct observer_overload_ignore_void_reduction;

template <bool IsDirect, class D, class... Os, class O>
struct observer_overload_ignore_void_reduction<conv_impl<IsDirect, D, Os...>, O>
    : std::conditional<(!std::is_void_v<O>), std::type_identity<conv_impl<IsDirect, D, Os..., O>>, std::type_identity<conv_impl<IsDirect, D, Os...>>>::type {};

template <class O, class I>
using observer_overload_ignore_void_reduction_t =
    typename observer_overload_ignore_void_reduction<O, I>::type;

template <class F, class C, class... Os>
using observer_conv_impl = recursive_reduction_t<
    observer_overload_ignore_void_reduction_t,
    conv_impl<C::is_direct, typename observer_dispatch_reduction<
        typename C::dispatch_type>::type>,
    typename observer_overload_mapping_traits<
        F, C::is_direct, typename C::dispatch_type, Os>::type...>;


template <class O, class I>
struct observer_conv_reduction : std::conditional<(std::tuple_size_v<typename I::overload_types> != 0u), 
  std::type_identity<add_conv_t<O, I>>, std::type_identity<O>>::type {};

template <class F>
struct observer_conv_reduction_traits {
  template <class O, class I>
  using type = typename observer_conv_reduction<O, instantiated_t<
      observer_conv_impl, typename I::overload_types, F, I>>::type;
};

template <class O, class I>
struct observer_refl_reduction;
template <class... Rs, class R>
struct observer_refl_reduction<std::tuple<Rs...>, R>
    : std::type_identity<
      typename std::conditional<(!R::is_direct), std::tuple<Rs..., R>, std::tuple<Rs...>>::type
    > {};

template <class O, class I>
using observer_refl_reduction_t = typename observer_refl_reduction<O, I>::type;

template <class F, class... Cs>
struct observer_facade_conv_impl {
  using convention_types = recursive_reduction_t<
      observer_conv_reduction_traits<F>::template type, std::tuple<>, Cs...>;
};
template <class... Rs>
struct observer_facade_refl_impl {
  using reflection_types = recursive_reduction_t<
      observer_refl_reduction_t, std::tuple<>, Rs...>;
};

template <class F>
struct proxy_view_overload_traits
    : std::type_identity<proxy_view<F>() noexcept> {};
template <class F>
struct proxy_view_overload_traits<const F>
    : std::type_identity<proxy_view<const F>() const noexcept> {};
template <class F>
using proxy_view_overload = typename proxy_view_overload_traits<F>::type;

template <std::size_t N>
struct sign {
  constexpr sign(const char (&str)[N])
      { for (std::size_t i = 0; i < N; ++i) { value[i] = str[i]; } }

  char value[N];
};
template <std::size_t N>
sign(const char (&str)[N]) -> sign<N>;

/*#if __STDC_HOSTED__
template <class CharT> struct format_overload_traits;
template <>
struct format_overload_traits<char>
    : std::type_identity<std::format_context::iterator(
          std::string_view spec, std::format_context& fc) const> {};
template <>
struct format_overload_traits<wchar_t>
    : std::type_identity<std::wformat_context::iterator(
          std::wstring_view spec, std::wformat_context& fc) const> {};
template <class CharT>
using format_overload_t = typename format_overload_traits<CharT>::type;

struct format_dispatch {
  // Note: This function requires std::formatter<T, CharT> to be well-formed.
  // However, the standard did not provide such facility before C++23. In the
  // "required" clause of this function, std::formattable (C++23) is preferred
  // when available. Otherwise, when building with C++20, we simply check
  // whether std::formatter<T, CharT> is a disabled specialization of
  // std::formatter by std::is_default_constructible_v as per
  // [format.formatter.spec].
  template <class T, class CharT, class OutIt>
  OutIt operator()(const T& self, std::basic_string_view<CharT> spec,
      std::basic_format_context<OutIt, CharT>& fc)
      requires(
#if defined(__cpp_lib_format_ranges) && __cpp_lib_format_ranges >= 202207L
          std::formattable<T, CharT>
#else
          std::is_default_constructible_v<std::formatter<T, CharT>>
#endif  // defined(__cpp_lib_format_ranges) && __cpp_lib_format_ranges >= 202207L
      ) {
    std::formatter<T, CharT> impl;
    {
      std::basic_format_parse_context<CharT> pc{spec};
      impl.parse(pc);
    }
    return impl.format(self, fc);
  }
};
#endif  // __STDC_HOSTED__*/

#ifdef __cpp_rtti
struct proxy_cast_context {
  const std::type_info* type_ptr;
  bool is_ref;
  bool is_const;
  void* result_ptr;
};

struct proxy_cast_dispatch;
template <class F, bool IsDirect, class D, class O>
struct proxy_cast_accessor_impl {
  using _Self = add_qualifier_t<
      adl_accessor_arg_t<F, IsDirect>, overload_traits<O>::qualifier>;
  template <class T>
  friend T proxy_cast(_Self self) {
    static_assert(!std::is_rvalue_reference_v<T>);
    if (!access_proxy<F>(self).has_value()) { ___PRO_THROW(bad_proxy_cast{}); }
    if constexpr (std::is_lvalue_reference_v<T>) {
      using U = std::remove_reference_t<T>;
      void* result = nullptr;
      proxy_cast_context ctx{.type_ptr = &typeid(T), .is_ref = true,
          .is_const = std::is_const_v<U>, .result_ptr = &result};
      proxy_invoke<IsDirect, D, O>(
          access_proxy<F>(std::forward<_Self>(self)), ctx);
      if (result == nullptr) { ___PRO_THROW(bad_proxy_cast{}); }
      return *static_cast<U*>(result);
    } else {
      std::optional<std::remove_const_t<T>> result;
      proxy_cast_context ctx{.type_ptr = &typeid(T), .is_ref = false,
          .is_const = false, .result_ptr = &result};
      proxy_invoke<IsDirect, D, O>(
          access_proxy<F>(std::forward<_Self>(self)), ctx);
      if (!result.has_value()) { ___PRO_THROW(bad_proxy_cast{}); }
      return std::move(*result);
    }
  }
  template <class T>
  friend T* proxy_cast(std::remove_reference_t<_Self>* self) noexcept{
    static_assert(std::is_lvalue_reference_v<_Self>, "Self type should be lvalue reference");
    if (!access_proxy<F>(*self).has_value()) { return nullptr; }
    void* result = nullptr;
    proxy_cast_context ctx{.type_ptr = &typeid(T), .is_ref = true,
        .is_const = std::is_const_v<T>, .result_ptr = &result};
    proxy_invoke<IsDirect, D, O>(access_proxy<F>(*self), ctx);
    return static_cast<T*>(result);
  }
};

#define ___PRO_DEF_PROXY_CAST_ACCESSOR(Q, ...) \
    template <class F, bool IsDirect, class D> \
    struct accessor_single<F, IsDirect, D, void(proxy_cast_context) Q> \
        : proxy_cast_accessor_impl<F, IsDirect, D, \
              void(proxy_cast_context) Q> {}
struct proxy_cast_dispatch {
  template <class T>
  void operator()(T&& self, proxy_cast_context ctx) {
    if (typeid(T) == *ctx.type_ptr) {
      if (ctx.is_ref) {
        if constexpr (std::is_lvalue_reference_v<T>) {
          if (ctx.is_const || !std::is_const_v<T>) {
            *static_cast<void**>(ctx.result_ptr) = (void*)&self;
          }
        }
      } else {
        if constexpr (std::is_constructible_v<std::decay_t<T>, T>) {
          static_cast<std::optional<std::decay_t<T>>*>(ctx.result_ptr)
              ->emplace(std::forward<T>(self));
        }
      }
    }
  }
  ___PRO_DEF_FREE_ACCESSOR_TEMPLATE(___PRO_DEF_PROXY_CAST_ACCESSOR)
};
#undef ___PRO_DEF_PROXY_CAST_ACCESSOR

struct proxy_typeid_reflector {
  template <class T>
  constexpr explicit proxy_typeid_reflector(std::in_place_type_t<T>)
      : info(&typeid(T)) {}
  constexpr proxy_typeid_reflector(const proxy_typeid_reflector&) = default;

  template <class F, bool IsDirect, class R>
  struct accessor {
    friend const std::type_info& proxy_typeid(
        const adl_accessor_arg_t<F, IsDirect>& self) noexcept {
      const proxy<F>& p = access_proxy<F>(self);
      if (!p.has_value()) { return typeid(void); }
      const proxy_typeid_reflector& refl = proxy_reflect<IsDirect, R>(p);
      return *refl.info;
    }
___PRO_DEBUG(
    accessor() noexcept { std::ignore = &accessor::_symbol_guard; }

   private:
    static inline const std::type_info& _symbol_guard(
        const adl_accessor_arg_t<F, IsDirect>& self) noexcept
        { return proxy_typeid(self); }
)
  };

  const std::type_info* info;
};
#endif  // __cpp_rtti

struct wildcard {
  wildcard() = default;

  template <class T>
  [[noreturn]] operator T() {
#ifdef __cpp_lib_unreachable
    std::unreachable();
#else
    std::abort();
#endif  // __cpp_lib_unreachable
  }
};

}  // namespace details

template <class Cs, class Rs, typename C>
struct basic_facade_builder {
  template <class D, class... Os>
      
  using add_indirect_convention = typename std::enable_if<(sizeof...(Os) > 0u &&
          (details::overload_traits<Os>::applicable && ...)), basic_facade_builder<details::add_conv_t<
      Cs, details::conv_impl<false, D, Os...>>, Rs, C>>::type;
  template <class D, class... Os>
  using add_direct_convention = typename std::enable_if<(sizeof...(Os) > 0u &&
          (details::overload_traits<Os>::applicable && ...)), basic_facade_builder<details::add_conv_t<
      Cs, details::conv_impl<true, D, Os...>>, Rs, C>>::type;


  template <class D, class... Os>
  using add_convention = typename std::enable_if<(sizeof...(Os) > 0u &&
          (details::overload_traits<Os>::applicable && ...)), add_indirect_convention<D, Os...>>::type;
  template <class R>
  using add_indirect_reflection = basic_facade_builder<
      Cs, details::add_tuple_t<Rs, details::refl_impl<false, R>>, C>;
  template <class R>
  using add_direct_reflection = basic_facade_builder<
      Cs, details::add_tuple_t<Rs, details::refl_impl<true, R>>, C>;
  template <class R>
  using add_reflection = add_indirect_reflection<R>;
  template <typename F, bool WithUpwardConversion = false>
  using add_facade = basic_facade_builder<
      details::merge_facade_conv_t<Cs, F, WithUpwardConversion>,
      details::merge_tuple_t<Rs, typename F::reflection_types>,
      details::merge_constraints_t<C, typename F::constraints>>;

  template <std::size_t PtrSize, std::size_t PtrAlign = details::max_align_of(PtrSize)>
  struct restrict_layout_helpers{
    static_assert((std::has_single_bit(PtrAlign) && PtrSize % PtrAlign == 0u), "Ptr size doesn't match alignment requirements");
    using type = basic_facade_builder<Cs, Rs, details::make_restricted_layout_t<C, PtrSize, PtrAlign>>;
  };
  template <std::size_t PtrSize,
      std::size_t PtrAlign = details::max_align_of(PtrSize)>
  using restrict_layout = typename restrict_layout_helpers<PtrSize, PtrAlign>::type;

  template <constraint_level CL>
  using support_copy = basic_facade_builder<
      Cs, Rs, typename C::template with_copyability<CL>>;
  template <constraint_level CL>
  using support_relocation = basic_facade_builder<
      Cs, Rs, typename C::template with_relocatability<CL>>;
  template <constraint_level CL>
  using support_destruction = basic_facade_builder<
      Cs, Rs, typename C::template with_destructibility<CL>>;
/*#if __STDC_HOSTED__
  using support_format = add_convention<
      details::format_dispatch, details::format_overload_t<char>>;
  using support_wformat = add_convention<
      details::format_dispatch, details::format_overload_t<wchar_t>>;
#endif  // __STDC_HOSTED__*/
#ifdef __cpp_rtti
  using support_indirect_rtti = basic_facade_builder<
      details::add_conv_t<Cs, details::conv_impl<false,
          details::proxy_cast_dispatch, void(details::proxy_cast_context) &,
          void(details::proxy_cast_context) const&,
          void(details::proxy_cast_context) &&>>,
      details::add_tuple_t<Rs, details::refl_impl<false,
          details::proxy_typeid_reflector>>, C>;
  using support_direct_rtti = basic_facade_builder<
      details::add_conv_t<Cs, details::conv_impl<true,
          details::proxy_cast_dispatch, void(details::proxy_cast_context) &,
          void(details::proxy_cast_context) const&,
          void(details::proxy_cast_context) &&>>,
      details::add_tuple_t<Rs, details::refl_impl<true,
          details::proxy_typeid_reflector>>, C>;
  using support_rtti = support_indirect_rtti;
#endif  // __cpp_rtti
  template <class F>
  using add_view = add_direct_convention<
      details::proxy_view_dispatch, details::proxy_view_overload<F>>;
  using build = details::facade_impl<Cs, Rs, details::normalize_t<C>>;
  basic_facade_builder() = delete;
};

template <class F>
struct observer_facade
    : details::instantiated_t<details::observer_facade_conv_impl,
          typename F::convention_types, F>,
      details::instantiated_t<details::observer_facade_refl_impl,
          typename F::reflection_types> {
  static constexpr proxiable_ptr_constraints constraints{
      .max_size = sizeof(void*), .max_align = alignof(void*),
      .copyability = constraint_level::trivial,
      .relocatability = constraint_level::trivial,
      .destructibility = constraint_level::trivial};
};

using facade_builder = basic_facade_builder<std::tuple<>, std::tuple<>,
    proxiable_ptr_constraints_t<details::invalid_size, details::invalid_size, details::invalid_cl, details::invalid_cl, details::invalid_cl>>;

template<char... S>
struct const_string{};

template <typename Sign, bool Rhs = false>
struct operator_dispatch;

#define ___PRO_DEF_LHS_LEFT_OP_ACCESSOR(Q, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class R> \
    struct accessor_single<__F, __IsDirect, __D, R() Q> { \
      ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__) \
      R __VA_ARGS__() Q { return proxy_invoke<__IsDirect, __D, R() Q>(SELF); } \
    }
#define ___PRO_DEF_LHS_ANY_OP_ACCESSOR(Q, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class R, class... Args> \
    struct accessor_single<__F, __IsDirect, __D, R(Args...) Q> { \
      ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__) \
      R __VA_ARGS__(Args... args) Q { \
        return proxy_invoke<__IsDirect, __D, R(Args...) Q>( \
            SELF, std::forward<Args>(args)...); \
      } \
    }
#define ___PRO_DEF_LHS_UNARY_OP_ACCESSOR ___PRO_DEF_LHS_ANY_OP_ACCESSOR
#define ___PRO_DEF_LHS_BINARY_OP_ACCESSOR ___PRO_DEF_LHS_ANY_OP_ACCESSOR
#define ___PRO_DEF_LHS_ALL_OP_ACCESSOR ___PRO_DEF_LHS_ANY_OP_ACCESSOR
#define ___PRO_LHS_LEFT_OP_DISPATCH_BODY_IMPL(...) \
    template <class T> \
    auto operator()(T&& self) \
        ___PRO_DIRECT_FUNC_IMPL(__VA_ARGS__ std::forward<T>(self))
#define ___PRO_LHS_UNARY_OP_DISPATCH_BODY_IMPL(...) \
    template <class T> \
    auto operator()(T&& self) \
        ___PRO_DIRECT_FUNC_IMPL(__VA_ARGS__ std::forward<T>(self)) \
    template <class T> \
    auto operator()(T&& self, int) \
        ___PRO_DIRECT_FUNC_IMPL(std::forward<T>(self) __VA_ARGS__)
#define ___PRO_LHS_BINARY_OP_DISPATCH_BODY_IMPL(...) \
    template <class T, class Arg> \
    auto operator()(T&& self, Arg&& arg) \
        ___PRO_DIRECT_FUNC_IMPL( \
            std::forward<T>(self) __VA_ARGS__ std::forward<Arg>(arg))
#define ___PRO_LHS_ALL_OP_DISPATCH_BODY_IMPL(...) \
    ___PRO_LHS_LEFT_OP_DISPATCH_BODY_IMPL(__VA_ARGS__) \
    ___PRO_LHS_BINARY_OP_DISPATCH_BODY_IMPL(__VA_ARGS__)
#define ___PRO_LHS_OP_DISPATCH_IMPL(TYPE,OP, ...) \
    template <> \
    struct operator_dispatch<const_string<__VA_ARGS__>, false> { \
      ___PRO_LHS_##TYPE##_OP_DISPATCH_BODY_IMPL(OP) \
      ___PRO_DEF_MEM_ACCESSOR_TEMPLATE( \
          ___PRO_DEF_LHS_##TYPE##_OP_ACCESSOR, operator OP) \
    };

#define ___PRO_DEF_RHS_OP_ACCESSOR(Q, NE, SELF_ARG, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class R, class Arg> \
    struct accessor_single<__F, __IsDirect, __D, R(Arg) Q> { \
      friend R operator __VA_ARGS__(Arg arg, SELF_ARG) NE { \
        return proxy_invoke<__IsDirect, __D, R(Arg) Q>( \
            SELF, std::forward<Arg>(arg)); \
      } \
___PRO_DEBUG( \
      accessor_single() noexcept { std::ignore = &accessor_single::_symbol_guard; } \
    \
     private: \
      static inline R _symbol_guard(Arg arg, SELF_ARG) NE { \
        return std::forward<Arg>(arg) __VA_ARGS__ \
            std::forward<decltype(__self)>(__self); \
      } \
) \
    }
#define ___PRO_RHS_OP_DISPATCH_IMPL(OP, ...) \
    template <> \
    struct operator_dispatch<const_string<__VA_ARGS__>, true> { \
      template <class T, class Arg> \
      auto operator()(T&& self, Arg&& arg) \
          ___PRO_DIRECT_FUNC_IMPL( \
              std::forward<Arg>(arg) OP std::forward<T>(self)) \
      ___PRO_DEF_FREE_ACCESSOR_TEMPLATE( \
          ___PRO_DEF_RHS_OP_ACCESSOR, OP) \
    };

#define ___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL(OP, ...) \
    ___PRO_LHS_OP_DISPATCH_IMPL(ALL, OP, __VA_ARGS__) \
    ___PRO_RHS_OP_DISPATCH_IMPL(OP, __VA_ARGS__)

#define ___PRO_BINARY_OP_DISPATCH_IMPL(OP, ...) \
    ___PRO_LHS_OP_DISPATCH_IMPL(BINARY, OP, __VA_ARGS__) \
    ___PRO_RHS_OP_DISPATCH_IMPL(OP, __VA_ARGS__)

#define ___PRO_DEF_LHS_ASSIGNMENT_OP_ACCESSOR(Q, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class R, class Arg> \
    struct accessor_single<__F, __IsDirect, __D, R(Arg) Q> { \
      ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__) \
      decltype(auto) __VA_ARGS__(Arg arg) Q { \
        proxy_invoke<__IsDirect, __D, R(Arg) Q>(SELF, std::forward<Arg>(arg)); \
        if constexpr (__IsDirect) { \
          return SELF; \
        } else { \
          return *SELF; \
        } \
      } \
    }
#define ___PRO_DEF_RHS_ASSIGNMENT_OP_ACCESSOR(Q, NE, SELF_ARG, SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class R, class Arg> \
    struct accessor_single<__F, __IsDirect, __D, R(Arg&) Q> { \
      friend Arg& operator __VA_ARGS__(Arg& arg, SELF_ARG) NE { \
        proxy_invoke<__IsDirect, __D, R(Arg&) Q>(SELF, arg); \
        return arg; \
      } \
___PRO_DEBUG( \
      accessor_single() noexcept { std::ignore = &accessor_single::_symbol_guard; } \
    \
     private: \
      static inline Arg& _symbol_guard(Arg& arg, SELF_ARG) NE \
          { return arg __VA_ARGS__ std::forward<decltype(__self)>(__self); } \
) \
    }
#define ___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(OP, ...) \
    template <> \
    struct operator_dispatch<const_string<__VA_ARGS__>, false> { \
      template <class T, class Arg> \
      auto operator()(T&& self, Arg&& arg) \
          ___PRO_DIRECT_FUNC_IMPL(std::forward<T>(self) OP \
              std::forward<Arg>(arg)) \
      ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_LHS_ASSIGNMENT_OP_ACCESSOR, \
          operator OP) \
    }; \
    template <> \
    struct operator_dispatch<const_string<__VA_ARGS__>, true> { \
      template <class T, class Arg> \
      auto operator()(T&& self, Arg&& arg) \
          ___PRO_DIRECT_FUNC_IMPL( \
              std::forward<Arg>(arg) OP std::forward<T>(self)) \
      ___PRO_DEF_FREE_ACCESSOR_TEMPLATE(___PRO_DEF_RHS_ASSIGNMENT_OP_ACCESSOR, \
          OP) \
    };

___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL(+, '+')
___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL(-, '-')
___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL(*, '*')
___PRO_BINARY_OP_DISPATCH_IMPL(/, '/')
___PRO_BINARY_OP_DISPATCH_IMPL(%, '%')
___PRO_LHS_OP_DISPATCH_IMPL(UNARY, ++, '+', '+')
___PRO_LHS_OP_DISPATCH_IMPL(UNARY, --, '-', '-')
___PRO_BINARY_OP_DISPATCH_IMPL(==, '=', '=')
___PRO_BINARY_OP_DISPATCH_IMPL(!=, '!', '=')
___PRO_BINARY_OP_DISPATCH_IMPL(>, '>')
___PRO_BINARY_OP_DISPATCH_IMPL(<, '<')
___PRO_BINARY_OP_DISPATCH_IMPL(>=, '>', '=')
___PRO_BINARY_OP_DISPATCH_IMPL(<=, '<', '=')
// in c++17 spaceship operator is not supported
//___PRO_BINARY_OP_DISPATCH_IMPL(<=>, '<', '=', '>') 
___PRO_LHS_OP_DISPATCH_IMPL(LEFT, !, '!')
___PRO_BINARY_OP_DISPATCH_IMPL(&&, '&', '&')
___PRO_BINARY_OP_DISPATCH_IMPL(||, '|', '|')
___PRO_LHS_OP_DISPATCH_IMPL(LEFT, ~, '~')
___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL(&, '&')
___PRO_BINARY_OP_DISPATCH_IMPL(|, '|')
___PRO_BINARY_OP_DISPATCH_IMPL(^,'^')
___PRO_BINARY_OP_DISPATCH_IMPL(<<,'<','<')
___PRO_BINARY_OP_DISPATCH_IMPL(>>,'>','>')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(+=,'+','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(-=,'-','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(*=,'*','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(/=,'/','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(&=,'&','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(|=,'|','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(^=,'^','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(<<=,'<','<','=')
___PRO_ASSIGNMENT_OP_DISPATCH_IMPL(>>=,'>','>','=')
// ___PRO_BINARY_OP_DISPATCH_IMPL(,) Overloading ',' is evil!!! 
___PRO_BINARY_OP_DISPATCH_IMPL(->*,'-','>','*')

using operator_add = const_string<'+'>;
using operator_sub = const_string<'-'>;
using operator_mul = const_string<'*'>;
using operator_div = const_string<'/'>;
using operator_mod = const_string<'%'>;
using operator_pp = const_string<'+','+'>;
using operator_nn = const_string<'-','-'>;
using operator_eq = const_string<'=','='>;
using operator_ne = const_string<'!','='>;
using operator_lt = const_string<'<'>;
using operator_gt = const_string<'>'>;
using operator_le = const_string<'<','='>;
using operator_ge = const_string<'>','='>;
using operator_log_not = const_string<'!'>;
using operator_log_and = const_string<'&','&'>;
using operator_log_or = const_string<'|','|'>;

using operator_not = const_string<'~'>;
using operator_and = const_string<'&'>;
using operator_or = const_string<'|'>;
using operator_xor = const_string<'^'>;

using operator_shl = const_string<'<','<'>;
using operator_shr = const_string<'>','>'>;

using operator_set_add = const_string<'+','='>;
using operator_set_sub = const_string<'-','='>;
using operator_set_mul = const_string<'*','='>;
using operator_set_div = const_string<'/','='>;
using operator_set_mod = const_string<'%','='>;
using operator_set_not = const_string<'~','='>;
using operator_set_and = const_string<'&','='>;
using operator_set_or =  const_string<'|','='>;
using operator_set_xor = const_string<'^','='>;

using operator_set_shl = const_string<'<','<','='>;
using operator_set_shr = const_string<'>','>','='>;

using operator_call = const_string<'(',')'>;
using operator_index = const_string<'[',']'>;
using operator_memptr = const_string<'-','>','*'>;

template <>
struct operator_dispatch<const_string<'(',')'>, false> {
  template <class T, class... Args>
  auto operator()(T&& self, Args&&... args)
      ___PRO_DIRECT_FUNC_IMPL(
          std::forward<T>(self)(std::forward<Args>(args)...))
  ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_LHS_ANY_OP_ACCESSOR, operator())
};
template <>
struct operator_dispatch<const_string<'[',']'>, false> {
#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202110L
  template <class T, class... Args>
  auto operator()(T&& self, Args&&... args)
      ___PRO_DIRECT_FUNC_IMPL(
          std::forward<T>(self)[std::forward<Args>(args)...])
#else
  template <class T, class Arg>
  auto operator()(T&& self, Arg&& arg)
      ___PRO_DIRECT_FUNC_IMPL(std::forward<T>(self)[std::forward<Arg>(arg)])
#endif  // defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202110L
  ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_LHS_ANY_OP_ACCESSOR, operator[])
};

#undef ___PRO_ASSIGNMENT_OP_DISPATCH_IMPL
#undef ___PRO_DEF_RHS_ASSIGNMENT_OP_ACCESSOR
#undef ___PRO_DEF_LHS_ASSIGNMENT_OP_ACCESSOR
#undef ___PRO_BINARY_OP_DISPATCH_IMPL
#undef ___PRO_EXTENDED_BINARY_OP_DISPATCH_IMPL
#undef ___PRO_RHS_OP_DISPATCH_IMPL
#undef ___PRO_DEF_RHS_OP_ACCESSOR
#undef ___PRO_LHS_OP_DISPATCH_IMPL
#undef ___PRO_LHS_ALL_OP_DISPATCH_BODY_IMPL
#undef ___PRO_LHS_BINARY_OP_DISPATCH_BODY_IMPL
#undef ___PRO_LHS_UNARY_OP_DISPATCH_BODY_IMPL
#undef ___PRO_LHS_LEFT_OP_DISPATCH_BODY_IMPL
#undef ___PRO_DEF_LHS_ALL_OP_ACCESSOR
#undef ___PRO_DEF_LHS_BINARY_OP_ACCESSOR
#undef ___PRO_DEF_LHS_UNARY_OP_ACCESSOR
#undef ___PRO_DEF_LHS_ANY_OP_ACCESSOR
#undef ___PRO_DEF_LHS_LEFT_OP_ACCESSOR

struct implicit_conversion_dispatch
    : details::cast_dispatch_base<false, false> {
  template <class T>
  T&& operator()(T&& self) noexcept { return std::forward<T>(self); }
};
struct explicit_conversion_dispatch : details::cast_dispatch_base<true, false> {
  template <class T>
  auto operator()(T&& self) noexcept
      { return details::explicit_conversion_adapter<T>{std::forward<T>(self)}; }
};
using conversion_dispatch = explicit_conversion_dispatch;

class not_implemented : public std::exception {
 public:
  not_implemented() noexcept = default;
  char const* what() const noexcept override { return "pro::not_implemented"; }
};

template <class D>
struct weak_dispatch : D {
  using D::operator();
  template <class... Args>
  [[noreturn]] details::wildcard operator()(std::nullptr_t, Args&&...)
      { ___PRO_THROW(not_implemented{}); }
};

template <class D, class E>
struct weak_dispatch_with_handler : D {
  using D::operator();
  template <class... Args>
  details::wildcard operator()(std::nullptr_t, Args&&...)
      { (E{}); return details::wildcard(); }
};

#define ___PRO_EXPAND_IMPL(__X) __X
#define ___PRO_EXPAND_MACRO_IMPL( \
    __MACRO, __1, __2, __3, __NAME, ...) \
    __MACRO##_##__NAME
#define ___PRO_EXPAND_MACRO(__MACRO, ...) \
    ___PRO_EXPAND_IMPL(___PRO_EXPAND_MACRO_IMPL( \
        __MACRO, __VA_ARGS__, 3, 2)(__VA_ARGS__))

#define ___PRO_DEF_MEM_ACCESSOR(__Q, __SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class __R, \
        class... __Args> \
    struct accessor_single<__F, __IsDirect, __D, __R(__Args...) __Q> { \
      ___PRO_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__) \
      __R __VA_ARGS__(__Args... __args) __Q { \
        return ::pro::proxy_invoke<__IsDirect, __D, __R(__Args...) __Q>( \
            __SELF, ::std::forward<__Args>(__args)...); \
      } \
    }
#define ___PRO_DEF_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FNAME) \
    struct __NAME { \
      template <class __T, class... __Args> \
      auto operator()(__T&& __self, __Args&&... __args) \
          ___PRO_DIRECT_FUNC_IMPL(::std::forward<__T>(__self) \
              .__FUNC(::std::forward<__Args>(__args)...)) \
      ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_MEM_ACCESSOR, __FNAME) \
    }
#define ___PRO_DEF_MEM_DISPATCH_2(__NAME, __FUNC) \
    ___PRO_DEF_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FUNC)
#define ___PRO_DEF_MEM_DISPATCH_3(__NAME, __FUNC, __FNAME) \
    ___PRO_DEF_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FNAME)
#define PRO_DEF_MEM_DISPATCH(__NAME, ...) \
    ___PRO_EXPAND_MACRO(___PRO_DEF_MEM_DISPATCH, __NAME, __VA_ARGS__)

#define ___PRO_DEF_FREE_ACCESSOR(__Q, __NE, __SELF_ARG, __SELF, ...) \
    template <class __F, bool __IsDirect, class __D, class __R, \
        class... __Args> \
    struct accessor_single<__F, __IsDirect, __D, __R(__Args...) __Q> { \
      friend __R __VA_ARGS__(__SELF_ARG, __Args... __args) __NE { \
        return ::pro::proxy_invoke<__IsDirect, __D, __R(__Args...) __Q>( \
            __SELF, ::std::forward<__Args>(__args)...); \
      } \
___PRO_DEBUG( \
      accessor_single() noexcept { ::std::ignore = &accessor_single::_symbol_guard; } \
    \
     private: \
      static inline __R _symbol_guard(__SELF_ARG, __Args... __args) __NE { \
        return __VA_ARGS__(::std::forward<decltype(__self)>(__self), \
            ::std::forward<__Args>(__args)...); \
      } \
) \
    }
#define ___PRO_DEF_FREE_DISPATCH_IMPL(__NAME, __FUNC, __FNAME) \
    struct __NAME { \
      template <class __T, class... __Args> \
      auto operator()(__T&& __self, __Args&&... __args) \
          ___PRO_DIRECT_FUNC_IMPL(__FUNC(::std::forward<__T>(__self), \
              ::std::forward<__Args>(__args)...)) \
      ___PRO_DEF_FREE_ACCESSOR_TEMPLATE(___PRO_DEF_FREE_ACCESSOR, __FNAME) \
    }
#define ___PRO_DEF_FREE_DISPATCH_2(__NAME, __FUNC) \
    ___PRO_DEF_FREE_DISPATCH_IMPL(__NAME, __FUNC, __FUNC)
#define ___PRO_DEF_FREE_DISPATCH_3(__NAME, __FUNC, __FNAME) \
    ___PRO_DEF_FREE_DISPATCH_IMPL(__NAME, __FUNC, __FNAME)
#define PRO_DEF_FREE_DISPATCH(__NAME, ...) \
    ___PRO_EXPAND_MACRO(___PRO_DEF_FREE_DISPATCH, __NAME, __VA_ARGS__)

#define ___PRO_DEF_FREE_AS_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FNAME) \
    struct __NAME { \
      template <class __T, class... __Args> \
      auto operator()(__T&& __self, __Args&&... __args) \
          ___PRO_DIRECT_FUNC_IMPL(__FUNC(::std::forward<__T>(__self), \
              ::std::forward<__Args>(__args)...)) \
      ___PRO_DEF_MEM_ACCESSOR_TEMPLATE(___PRO_DEF_MEM_ACCESSOR, __FNAME) \
    }
#define ___PRO_DEF_FREE_AS_MEM_DISPATCH_2(__NAME, __FUNC) \
    ___PRO_DEF_FREE_AS_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FUNC)
#define ___PRO_DEF_FREE_AS_MEM_DISPATCH_3(__NAME, __FUNC, __FNAME) \
    ___PRO_DEF_FREE_AS_MEM_DISPATCH_IMPL(__NAME, __FUNC, __FNAME)
#define PRO_DEF_FREE_AS_MEM_DISPATCH(__NAME, ...) \
    ___PRO_EXPAND_MACRO(___PRO_DEF_FREE_AS_MEM_DISPATCH, __NAME, __VA_ARGS__)

#define PRO_DEF_WEAK_DISPATCH(__NAME, __D, __FUNC) \
    struct [[deprecated("'PRO_DEF_WEAK_DISPATCH' is deprecated. " \
        "Use pro::weak_dispatch<" #__D "> instead.")]] __NAME : __D { \
      using __D::operator(); \
      template <class... __Args> \
      auto operator()(::std::nullptr_t, __Args&&... __args) \
          ___PRO_DIRECT_FUNC_IMPL(__FUNC(::std::forward<__Args>(__args)...)) \
    }

}  // namespace pro

/*#if __STDC_HOSTED__
namespace std {

template <class F, class CharT>
    requires(pro::details::facade_traits<F>::template is_invocable<false,
        pro::details::format_dispatch, pro::details::format_overload_t<CharT>>)
struct formatter<pro::proxy_indirect_accessor<F>, CharT> {
  constexpr auto parse(basic_format_parse_context<CharT>& pc) {
    for (auto it = pc.begin(); it != pc.end(); ++it) {
      if (*it == '}') {
        spec_ = basic_string_view<CharT>{pc.begin(), it + 1};
        return it;
      }
    }
    return pc.end();
  }

  template <class OutIt>
  OutIt format(const pro::proxy_indirect_accessor<F>& ia,
      basic_format_context<OutIt, CharT>& fc) const {
    auto& p = pro::access_proxy<F>(ia);
    if (!p.has_value()) { ___PRO_THROW(format_error{"null proxy"}); }
    return pro::proxy_invoke<false, pro::details::format_dispatch,
        pro::details::format_overload_t<CharT>>(p, spec_, fc);
  }

 private:
  basic_string_view<CharT> spec_;
};

}  // namespace std
#endif  // __STDC_HOSTED__*/

#undef ___PRO_THROW
#undef ___PRO_NO_UNIQUE_ADDRESS_ATTRIBUTE


#define interface_def(name)                                                    \
    struct name;                                                               \
    struct ninf_##name{ struct inf_##name {                                                     \
        template <int N> struct TypeN {};                                      \
        template <> struct TypeN<__COUNTER__> {                                \
            using __T = pro::facade_builder;                                     \
        };                                                                     \
        [[maybe_unused]] const int Pad = __COUNTER__;                          \
                                                                              

#define interface_end(name)                                                    \
    }; }; struct name                                                                \
        : ninf_##name::inf_##name::TypeN<__COUNTER__                                        \
            - 2>::__T::build {  \
    };

#define interface_def_generic(name, ...)                                                    \
    template<typename ...Args> struct name;                                                               \
    namespace ninf_##name{ template<__VA_ARGS__> struct inf_##name {                                                     \
        template <int N> struct TypeN {};                                      \
        template <> struct TypeN<__COUNTER__> {                                \
            using __T = pro::facade_builder;                                     \
        };                                                                     \
        [[maybe_unused]] const int Pad = __COUNTER__;                          \
                                                                              

#define interface_end_generic(name)                                                    \
    }; }; template<typename ...Args>struct name                                                                \
        :  ninf_##name::inf_##name<Args...>::template TypeN<__COUNTER__                                        \
            - 2>::__T::build {  \
    };
#define restrict_layout(size, align)                                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__ - 3>::__T::template restrict_layout<size, align>;      \
    };

#define support_copy(copy)                                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__ - 3>::__T::template support_copy<copy>;      \
    };

#define support_relocate(rel)                                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__ - 3>::__T::template support_relocation<rel>;      \
    };
#define support_destroy(dest)                                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T =                                                              \
          typename TypeN<__COUNTER__ - 3>::__T::template support_destruction<dest>;       \
    };

#define add_direct_reflect(dest)                                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T =                                                              \
          typename TypeN<__COUNTER__ - 3>::__T::template add_direct_reflection<dest>;       \
    };
#define fn_def(name, ...)                                                \
    PRO_DEF_MEM_DISPATCH(Mem##name, name);                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<Mem##name, __VA_ARGS__>;                       \
    };

#define fn_def_weak(name, ...)                                                \
    PRO_DEF_MEM_DISPATCH(Mem##name, name);                                     \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<pro::weak_dispatch<Mem##name>, __VA_ARGS__>;                       \
    };

#define add_conv(name, ...)                                                \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<name, __VA_ARGS__>;                       \
    };

#define add_facade(name)                                                \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_facade<name>;                       \
    };

#define op_def(name, ...)                                                \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<pro::operator_dispatch<pro::operator_##name>, __VA_ARGS__>;   \
    };

#define op_def_weak(name, ...)                                                \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<pro::weak_dispatch<pro::operator_dispatch<pro::operator_##name>>, __VA_ARGS__>;   \
    };

#define op_direct_def(name, ...)                                                \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_direct_convention<pro::operator_dispatch<pro::operator_##name>, __VA_ARGS__>;   \
    };

#define econv_def(type)                                                        \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<pro::conversion_dispatch, type()>;           \
    };

#define iconv_def(type)                                                        \
    template <> struct TypeN<__COUNTER__> {                                    \
        using __T = typename TypeN<__COUNTER__                                   \
          - 3>::__T::template add_convention<pro::implicit_conversion_dispatch, type()>;  \
    };





#endif  // _MSFT_PROXY_
