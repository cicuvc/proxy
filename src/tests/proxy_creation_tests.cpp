#include <memory_resource>

#include <proxy.hpp>
#include "utils.hpp"
#include <gtest/gtest.h>

namespace proxy_creation_tests_details {
    struct SboReflector {
    public:
        template <class T>
        constexpr explicit SboReflector(std::in_place_type_t<pro::details::inplace_ptr<T>>)
            : SboEnabled(true), AllocatorAllocatesForItself(false) {}
        template <class T, class Alloc>
        constexpr explicit SboReflector(std::in_place_type_t<pro::details::allocated_ptr<T, Alloc>>)
            : SboEnabled(false), AllocatorAllocatesForItself(false) {}
        template <class T, class Alloc>
        constexpr explicit SboReflector(std::in_place_type_t<pro::details::compact_ptr<T, Alloc>>)
            : SboEnabled(false), AllocatorAllocatesForItself(true) {}

        template <class F, bool IsDirect, class R> struct accessor {
            const SboReflector& ReflectSbo() const noexcept {
                return pro::proxy_reflect<IsDirect, R>(pro::access_proxy<F>(*this));
            }
        };

        bool SboEnabled;
        bool AllocatorAllocatesForItself; 
    };

    interface_def(TestLargeStringable)
        support_relocate(pro::constraint_level::nontrivial);
        support_copy(pro::constraint_level::nontrivial);
        add_direct_reflect(SboReflector);
        add_conv(utils::spec::FreeToString, std::string());
    interface_end(TestLargeStringable);

    interface_def(TestSmallStringable)
        add_facade(TestLargeStringable);
        support_copy(pro::constraint_level::nontrivial);
        restrict_layout(sizeof(void*), sizeof(void*));
    interface_end(TestSmallStringable);

    TEST(ProxyCreationTests, TestMakeProxyInplace_FromValue) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        utils::LifetimeTracker::Session session { &tracker };
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
        {
            auto p = pro::make_proxy_inplace<TestLargeStringable>(session);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 2");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxyInplace_InPlace) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy_inplace<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxyInplace_InPlaceInitializerList) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy_inplace<TestLargeStringable, utils::LifetimeTracker::Session>({ 1, 2, 3 }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kInitializerListConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxyInplace_Lifetime_Copy) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy_inplace<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = p1;
            ASSERT_TRUE(p1.has_value());
            ASSERT_EQ(ToString(*p1), "Session 1");
            ASSERT_TRUE(p1.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_TRUE(p2.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }
    TEST(ProxyCreationTests, TestMakeProxyInplace_Lifetime_Move) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy_inplace<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = std::move(p1);
            ASSERT_FALSE(p1.has_value());
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_TRUE(p2.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kMoveConstruction);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_DirectAllocator_FromValue) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        utils::LifetimeTracker::Session session { &tracker };
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
        {
            auto p = pro::allocate_proxy<TestSmallStringable>(std::allocator<void> {}, session);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 2");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_DirectAllocator_InPlace) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p
                = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(std::allocator<void> {}, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }
    TEST(ProxyCreationTests, TestAllocateProxy_DirectAllocator_InPlaceInitializerList) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(
                std::allocator<void> {}, { 1, 2, 3 }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kInitializerListConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_DirectAllocator_Lifetime_Copy) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1
                = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(std::allocator<void> {}, &tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = p1;
            ASSERT_TRUE(p1.has_value());
            ASSERT_EQ(ToString(*p1), "Session 1");
            ASSERT_FALSE(p1.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p1.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p2.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_DirectAllocator_Lifetime_Move) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1
                = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(std::allocator<void> {}, &tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = std::move(p1);
            ASSERT_FALSE(p1.has_value());
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 1");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p2.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_IndirectAllocator_FromValue) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        utils::LifetimeTracker::Session session { &tracker };
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
        {
            std::pmr::unsynchronized_pool_resource memory_pool;
            auto p = pro::allocate_proxy<TestSmallStringable>(
                std::pmr::polymorphic_allocator<utils::LifetimeTracker::Session> { &memory_pool }, session);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 2");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_IndirectAllocator_InPlace) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            std::pmr::unsynchronized_pool_resource memory_pool;
            auto p = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(
                std::pmr::polymorphic_allocator<utils::LifetimeTracker> { &memory_pool }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_IndirectAllocator_InPlaceInitializerList) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            std::pmr::unsynchronized_pool_resource memory_pool;
            auto p = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(
                std::pmr::polymorphic_allocator<utils::LifetimeTracker> { &memory_pool }, { 1, 2, 3 }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kInitializerListConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_IndirectAllocator_Lifetime_Copy) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            std::pmr::unsynchronized_pool_resource memory_pool;
            auto p1 = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(
                std::pmr::polymorphic_allocator<utils::LifetimeTracker> { &memory_pool }, &tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = p1;
            ASSERT_TRUE(p1.has_value());
            ASSERT_EQ(ToString(*p1), "Session 1");
            ASSERT_FALSE(p1.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p1.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p2.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestAllocateProxy_IndirectAllocator_Lifetime_Move) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            std::pmr::unsynchronized_pool_resource memory_pool;
            auto p1 = pro::allocate_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(
                std::pmr::polymorphic_allocator<utils::LifetimeTracker> { &memory_pool }, &tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = std::move(p1);
            ASSERT_FALSE(p1.has_value());
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 1");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p2.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithSBO_FromValue) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        utils::LifetimeTracker::Session session { &tracker };
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
        {
            auto p = pro::make_proxy<TestLargeStringable>(session);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 2");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithSBO_InPlace) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithSBO_InPlaceInitializerList) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy<TestLargeStringable, utils::LifetimeTracker::Session>({ 1, 2, 3 }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_TRUE(p.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kInitializerListConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithSBO_Lifetime_Copy) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = p1;
            ASSERT_TRUE(p1.has_value());
            ASSERT_EQ(ToString(*p1), "Session 1");
            ASSERT_TRUE(p1.ReflectSbo().SboEnabled);
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_TRUE(p2.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithSBO_Lifetime_Move) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy<TestLargeStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = std::move(p1);
            ASSERT_FALSE(p1.has_value());
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_TRUE(p2.ReflectSbo().SboEnabled);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kMoveConstruction);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithoutSBO_FromValue) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        utils::LifetimeTracker::Session session { &tracker };
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
        {
            auto p = pro::make_proxy<TestSmallStringable>(session);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 2");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithoutSBO_InPlace) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(&tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithoutSBO_InPlaceInitializerList) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p = pro::make_proxy<TestSmallStringable, utils::LifetimeTracker::Session>({ 1, 2, 3 }, &tracker);
            ASSERT_TRUE(p.has_value());
            ASSERT_EQ(ToString(*p), "Session 1");
            ASSERT_FALSE(p.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kInitializerListConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithoutSBO_Lifetime_Copy) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = p1;
            ASSERT_TRUE(p1.has_value());
            ASSERT_EQ(ToString(*p1), "Session 1");
            ASSERT_FALSE(p1.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p1.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 2");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p2.ReflectSbo().AllocatorAllocatesForItself);
            expected_ops.emplace_back(2, utils::LifetimeOperationType::kCopyConstruction);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

    TEST(ProxyCreationTests, TestMakeProxy_WithoutSBO_Lifetime_Move) {
        utils::LifetimeTracker tracker;
        std::vector<utils::LifetimeOperation> expected_ops;
        {
            auto p1 = pro::make_proxy<TestSmallStringable, utils::LifetimeTracker::Session>(&tracker);
            expected_ops.emplace_back(1, utils::LifetimeOperationType::kValueConstruction);
            auto p2 = std::move(p1);
            ASSERT_FALSE(p1.has_value());
            ASSERT_TRUE(p2.has_value());
            ASSERT_EQ(ToString(*p2), "Session 1");
            ASSERT_FALSE(p2.ReflectSbo().SboEnabled);
            ASSERT_FALSE(p2.ReflectSbo().AllocatorAllocatesForItself);
            ASSERT_TRUE(tracker.GetOperations() == expected_ops);
        }
        expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
        ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    }

}
