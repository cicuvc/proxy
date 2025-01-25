#include <gtest/gtest.h>
#include <proxy.hpp>
#include <tuple>
#include <utility>
#include <vector>

namespace proxy_rtti_tests_details {

    struct TestFacade : pro::facade_builder ::support_rtti ::support_direct_rtti ::build {};

} // namespace proxy_rtti_tests_details

namespace details = proxy_rtti_tests_details;

TEST(ProxyRttiTests, TestIndirectCast_Void_Fail) {
    pro::proxy<details::TestFacade> p;
    bool exception_thrown = false;
    try {
        std::ignore = proxy_cast(*p, std::in_place_type<int>);
    } catch (const pro::bad_proxy_cast&) {
        exception_thrown = true;
    }
    ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestIndirectCast_Ref_Succeed) {
    int v = 123;
    pro::proxy<details::TestFacade> p = &v;
    proxy_cast(*p, std::in_place_type<int&>) = 456;
    ASSERT_EQ(v, 456);
}

TEST(ProxyRttiTests, TestIndirectCast_Ref_Fail) {
    int v = 123;
    pro::proxy<details::TestFacade> p = &v;
    bool exception_thrown = false;
    try {
        proxy_cast(*p, std::in_place_type<double&>);
    } catch (const pro::bad_proxy_cast&) {
        exception_thrown = true;
    }
    ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestIndirectCast_ConstRef_Succeed) {
    int v = 123;
    pro::proxy<details::TestFacade> p = &v;
    const int& r = proxy_cast(*p, std::in_place_type<const int&>);
    ASSERT_EQ(&v, &r);
    ASSERT_EQ(v, 123);
}

TEST(ProxyRttiTests, TestIndirectCast_ConstRef_Fail) {
    int v = 123;
    pro::proxy<details::TestFacade> p = &v;
    bool exception_thrown = false;
    try {
        proxy_cast(*p,std::in_place_type<const double&>);
    } catch (const pro::bad_proxy_cast&) {
        exception_thrown = true;
    }
    ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestIndirectCast_Copy_Succeed) {
  int v1 = 123;
  pro::proxy<details::TestFacade> p = &v1;
  int v2 = proxy_cast(*p, std::in_place_type<int>);
  ASSERT_EQ(v1, 123);
  ASSERT_EQ(v2, 123);
}

TEST(ProxyRttiTests, TestIndirectCast_Copy_Fail) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  bool exception_thrown = false;
  try {
    proxy_cast(*p, std::in_place_type<double>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}




TEST(ProxyRttiTests, TestIndirectTypeid_Void) {
  pro::proxy<details::TestFacade> p;
  ASSERT_EQ(proxy_typeid(*p), typeid(void));
}

TEST(ProxyRttiTests, TestIndirectTypeid_Value) {
  int a = 123;
  pro::proxy<details::TestFacade> p = &a;
  ASSERT_EQ(proxy_typeid(*p), typeid(int));
}

TEST(ProxyRttiTests, TestDirectCast_Void_Fail) {
  pro::proxy<details::TestFacade> p;
  bool exception_thrown = false;
  try {
    proxy_cast(p,std::in_place_type<int>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestDirectCast_Ref_Succeed) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  *proxy_cast(p,std::in_place_type<int*&>) = 456;
  ASSERT_EQ(v, 456);
}

TEST(ProxyRttiTests, TestDirectCast_Ref_Fail) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  bool exception_thrown = false;
  try {
    proxy_cast(p,std::in_place_type<double*&>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestDirectCast_ConstRef_Succeed) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  int* const& r = proxy_cast(p,std::in_place_type<int* const&>);
  ASSERT_EQ(&v, r);
  ASSERT_EQ(v, 123);
}

TEST(ProxyRttiTests, TestDirectCast_ConstRef_Fail) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  bool exception_thrown = false;
  try {
    proxy_cast(p,std::in_place_type<double* const&>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestDirectCast_Copy_Succeed) {
  int v1 = 123;
  pro::proxy<details::TestFacade> p = &v1;
  int* v2 = proxy_cast(p,std::in_place_type<int*>);
  ASSERT_EQ(v2, &v1);
  ASSERT_EQ(v1, 123);
}

TEST(ProxyRttiTests, TestDirectCast_Copy_Fail) {
  int v = 123;
  pro::proxy<details::TestFacade> p = &v;
  bool exception_thrown = false;
  try {
    proxy_cast(p,std::in_place_type<double*>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyRttiTests, TestDirectCast_Move_Succeed) {
  int v1 = 123;
  pro::proxy<details::TestFacade> p = &v1;
  auto v2 = proxy_cast(std::move(p),std::in_place_type<int*>);
  ASSERT_EQ(v2, &v1);
  ASSERT_EQ(v1, 123);
  ASSERT_FALSE(p.has_value());
}

TEST(ProxyRttiTests, TestDirectCast_Move_Fail) {
  int v1 = 123;
  pro::proxy<details::TestFacade> p = &v1;
  bool exception_thrown = false;
  try {
    proxy_cast(std::move(p),std::in_place_type<double*>);
  } catch (const pro::bad_proxy_cast&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
  ASSERT_FALSE(p.has_value());
}



TEST(ProxyRttiTests, TestDirectTypeid_Void) {
  pro::proxy<details::TestFacade> p;
  ASSERT_EQ(proxy_typeid(p), typeid(void));
}

TEST(ProxyRttiTests, TestDirectTypeid_Value) {
  int a = 123;
  pro::proxy<details::TestFacade> p = &a;
  ASSERT_EQ(proxy_typeid(p), typeid(int*));
}
