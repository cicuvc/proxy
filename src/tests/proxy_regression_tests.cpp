#include "../proxy.hpp"
#include <gtest/gtest.h>

// https://github.com/microsoft/proxy/issues/213

namespace proxy_regression_tests {

    interface_def(MyTrivialFacade)
        op_def(call, void(), void() const);
        support_copy(pro::constraint_level::trivial);
        support_relocate(pro::constraint_level::trivial);
        support_destroy(pro::constraint_level::trivial);
    interface_end(MyTrivialFacade);

    TEST(ProxyRegressionTests, TestUnexpectedCompilerWarning) {
        int side_effect = 0;
        pro::proxy<MyTrivialFacade> p = pro::make_proxy<MyTrivialFacade>([&] { side_effect = 1; });
        (*p)();
        EXPECT_EQ(side_effect, 1);
    }
}