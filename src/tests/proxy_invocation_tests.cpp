#include <algorithm>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <list>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <proxy.hpp>
#include "utils.hpp"
#include <gtest/gtest.h>

namespace proxy_invocation_tests {

    namespace details {
        template <class T> std::size_t get_size(const T& container) noexcept { return container.size(); }
        template <class T, class F> void ranges_for_each(T& container, F&& callback) noexcept {
            for (auto& i : container)
                callback(i);
        }

        interface_def_generic(Callable, class... Os)
            support_copy(pro::constraint_level::nontrivial);
            op_def(call, Os...);
        interface_end_generic(Callable);

        interface_def(ResourceDictionary)
            fn_def_weak(at, std::string(int));
        interface_end(ResourceDictionary);

        template <class... Args> std::vector<std::type_index> GetTypeIndices() {
            return { std::type_index { typeid(Args) }... };
        }

        PRO_DEF_FREE_DISPATCH(FreeSize, get_size, Size);
        PRO_DEF_FREE_DISPATCH(FreeForEach, ranges_for_each, ForEach);

        interface_def_generic(Iterable, class T)
            ;
            add_conv(FreeForEach, void(std::function<void(T&)>));
            add_conv(FreeSize, std::size_t() noexcept);
        interface_end_generic(Iterable);

        interface_def_generic(WeakCallable, class... Os)
            support_copy(pro::constraint_level::nontrivial);
            op_def_weak(call, Os...);
        interface_end_generic(WeakCallable);

        template <class F, class T> pro::proxy<F> LockImpl(const std::weak_ptr<T>& p) {
            auto result = p.lock();
            if (static_cast<bool>(result)) {
                return result;
            }
            return nullptr;
        }
        template <class F> PRO_DEF_FREE_DISPATCH(FreeLock, LockImpl<F>, Lock);
        template <class F>
        struct Weak : pro::facade_builder ::support_copy<pro::constraint_level::nontrivial>::add_convention<FreeLock<F>,
                          pro::proxy<F>()>::build {};
        template <class F, class T> pro::proxy<Weak<F>> GetWeakImpl(const std::shared_ptr<T>& p) {
            return pro::make_proxy<Weak<F>, std::weak_ptr<T>>(p);
        }
        template <class F> pro::proxy<Weak<F>> GetWeakImpl(std::nullptr_t) { return nullptr; }

        template <class F> PRO_DEF_FREE_DISPATCH(FreeGetWeak, GetWeakImpl<F>, GetWeak);
        struct SharedStringable
            : pro::facade_builder ::add_facade<utils::spec::Stringable>::add_direct_convention<FreeGetWeak<SharedStringable>,
                  pro::proxy<Weak<SharedStringable>>() const&>::build {};

        template <class T> std::string Dump(T&& value) noexcept {
            std::ostringstream out;
            out << std::boolalpha << "is_const=" << std::is_const_v<std::remove_reference_t<T>> << ", is_ref="
                << std::is_lvalue_reference_v<T> << ", value=" << value;
            return std::move(out).str();
        }

        PRO_DEF_FREE_DISPATCH(FreeDump, Dump);
    }
    TEST(ProxyInvocationTests, TestArgumentForwarding) {
        std::string arg1 = "My string";
        std::vector<int> arg2 = { 1, 2, 3 };
        std::vector<int> arg2_copy = arg2;
        std::string arg1_received;
        std::vector<int> arg2_received;
        int expected_result = 456;
        auto f = [&](std::string&& s, std::vector<int>&& v) -> int {
            arg1_received = std::move(s);
            arg2_received = std::move(v);
            return expected_result;
        };
        pro::proxy<details::Callable<int(std::string, std::vector<int>)>> p = &f;
        int result = (*p)(arg1, std::move(arg2));
        ASSERT_TRUE(p.has_value());
        ASSERT_EQ(arg1_received, arg1);
        ASSERT_TRUE(arg2.empty());
        ASSERT_EQ(arg2_received, arg2_copy);
        ASSERT_EQ(result, expected_result);
    }

    TEST(ProxyInvocationTests, TestThrow) {
        const char* expected_error_message = "My exception";
        auto f = [&] { throw std::runtime_error { expected_error_message }; };
        bool exception_thrown = false;
        pro::proxy<details::Callable<void()>> p = &f;
        try {
            (*p)();
        } catch (const std::runtime_error& e) {
            exception_thrown = true;
            ASSERT_STREQ(e.what(), expected_error_message);
        }
        ASSERT_TRUE(exception_thrown);
        ASSERT_TRUE(p.has_value());
    }

    TEST(ProxyInvocationTests, TestOverloadResolution) {
        struct OverloadedCallable : pro::facade_builder ::add_convention<pro::operator_dispatch<pro::operator_call>, void(int),
                                        void(double), void(const char*), void(char*), void(std::string, int)>::build {};
        std::vector<std::type_index> side_effect;
        auto p = pro::make_proxy<OverloadedCallable>(
            [&](auto&&... args) { side_effect = details::GetTypeIndices<std::decay_t<decltype(args)>...>(); });
        (*p)(123);
        ASSERT_EQ(side_effect, details::GetTypeIndices<int>());
        (*p)(1.23);
        ASSERT_EQ(side_effect, details::GetTypeIndices<double>());
        char foo[2];
        (*p)(foo);
        ASSERT_EQ(side_effect, details::GetTypeIndices<char*>());
        (*p)("lalala");
        ASSERT_EQ(side_effect, details::GetTypeIndices<const char*>());
        (*p)("lalala", 0);
        ASSERT_EQ(side_effect, (details::GetTypeIndices<std::string, int>()));
        ASSERT_FALSE((std::is_invocable_v<decltype(*p), std::vector<int>>));
    }

    TEST(ProxyInvocationTests, TestNoexcept) {
        std::vector<std::type_index> side_effect;
        auto p = pro::make_proxy<details::Callable<void(int) noexcept, void(double)>>(
            [&](auto&&... args) noexcept { side_effect = details::GetTypeIndices<std::decay_t<decltype(args)>...>(); });
        static_assert(noexcept((*p)(123)));
        (*p)(123);
        ASSERT_EQ(side_effect, details::GetTypeIndices<int>());
        static_assert(!noexcept((*p)(1.23)));
        (*p)(1.23);
        ASSERT_EQ(side_effect, details::GetTypeIndices<double>());
        ASSERT_FALSE((std::is_invocable_v<decltype(*p), char*>));
    }
    TEST(ProxyInvocationTests, TestMemberDispatchDefault) {
        std::vector<std::string> container1 { "hello", "world", "!" };
        std::list<std::string> container2 { "hello", "world" };
        pro::proxy<details::ResourceDictionary> p = &container1;
        ASSERT_EQ(p->at(0), "hello");
        p = &container2;
        {
            bool exception_thrown = false;
            try {
                p->at(0);
            } catch (const pro::not_implemented&) {
                exception_thrown = true;
            }
            ASSERT_TRUE(exception_thrown);
        }
    }

    TEST(ProxyInvocationTests, TestFunctionPointer) {
        struct TestFacade : details::Callable<std::vector<std::type_index>()> {};
        pro::proxy<TestFacade> p { &details::GetTypeIndices<int, double> };
        auto ret = (*p)();
        ASSERT_EQ(ret, (details::GetTypeIndices<int, double>()));
    }

    TEST(ProxyInvocationTests, TestMultipleDispatches_Unique) {
        std::list<int> l = { 1, 2, 3 };
        pro::proxy<details::Iterable<int>> p = &l;
        ASSERT_EQ(Size(*p), 3);
        int sum = 0;
        auto accumulate_sum = [&](int x) { sum += x; };
        ForEach(*p, accumulate_sum);
        ASSERT_EQ(sum, 6);
    }

    TEST(ProxyInvocationTests, TestMultipleDispatches_Duplicated) {
        struct DuplicatedIterable
            : pro::facade_builder ::add_convention<details::FreeForEach,
                  void(std::function<void(int&)>)>::add_convention<details::FreeSize,
                  std::size_t()>::add_convention<details::FreeForEach, void(std::function<void(int&)>)>::build {};
        static_assert(sizeof(pro::details::facade_traits<DuplicatedIterable>::meta)
            == sizeof(pro::details::facade_traits<details::Iterable<int>>::meta));
        std::list<int> l = { 1, 2, 3 };
        pro::proxy<DuplicatedIterable> p = &l;
        ASSERT_EQ(Size(*p), 3);
        int sum = 0;
        auto accumulate_sum = [&](int x) { sum += x; };
        ForEach(*p, accumulate_sum);
        ASSERT_EQ(sum, 6);
    }
    TEST(ProxyInvocationTests, TestFreeDispatchDefault) {
        {
            int side_effect = 0;
            auto p = pro::make_proxy<details::WeakCallable<void()>>([&] { side_effect = 1; });
            (*p)();
            ASSERT_EQ(side_effect, 1);
        }
        {
            bool exception_thrown = false;
            auto p = pro::make_proxy<details::WeakCallable<void()>>(123);
            try {
                (*p)();
            } catch (const pro::not_implemented&) {
                exception_thrown = true;
            }
            ASSERT_TRUE(exception_thrown);
        }
    }

    TEST(ProxyInvocationTests, TestQualifiedConvention_Member) {
        struct TestFacade : pro::facade_builder ::add_convention<pro::operator_dispatch<pro::operator_call>, int()&,
                                int() const&, int()&& noexcept, int() const&&>::build {};

        struct TestCallable {
            int operator()() & noexcept { return 0; }
            int operator()() const& noexcept { return 1; }
            int operator()() && noexcept { return 2; }
            int operator()() const&& noexcept { return 3; }
        };

        pro::proxy<TestFacade> p = pro::make_proxy<TestFacade, TestCallable>();
        static_assert(!noexcept((*p)()));
        static_assert(noexcept((*std::move(p))()));
        ASSERT_EQ((*p)(), 0);
        ASSERT_EQ((*std::as_const(p))(), 1);
        ASSERT_EQ((*std::move(p))(), 2);
        ASSERT_EQ((*std::move(std::as_const(p)))(), 3);
    }
    TEST(ProxyInvocationTests, TestQualifiedConvention_Free) {
        struct TestFacade : pro::facade_builder ::add_convention<details::FreeDump, std::string()&, std::string() const&,
                                std::string()&& noexcept, std::string() const&&>::build {};

        pro::proxy<TestFacade> p = pro::make_proxy<TestFacade>(123);
        static_assert(!noexcept(Dump(*p)));
        static_assert(noexcept(Dump(*std::move(p))));
        ASSERT_EQ(Dump(*p), "is_const=false, is_ref=true, value=123");
        ASSERT_EQ(Dump(*std::as_const(p)), "is_const=true, is_ref=true, value=123");
        ASSERT_EQ(Dump(*std::move(p)), "is_const=false, is_ref=false, value=123");
        ASSERT_EQ(Dump(*std::move(std::as_const(p))), "is_const=true, is_ref=false, value=123");
    }
}