#include <vector>

#include <proxy.hpp>
#include "utils.hpp"
#include <gtest/gtest.h>

namespace proxy_dispatch_tests {
    PRO_DEF_FREE_AS_MEM_DISPATCH(FreeMemToString, std::to_string, ToString);

    namespace op_plus {
        interface_def(TestFacade)
            op_def(add, int(), int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpPlus) {
            int value = 12;
            pro::proxy<TestFacade> p = &value;
            ASSERT_EQ(+*p, 12);
            ASSERT_EQ(*p + 2, 14);
        }
    }

    namespace op_sub {
        interface_def(TestFacade)
            op_def(sub, int(), int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpMinus) {
            int value = 12;
            pro::proxy<TestFacade> p = &value;
            ASSERT_EQ(-*p, -12);
            ASSERT_EQ(*p - 2, 10);
        }
    }

    namespace op_mul {
        interface_def(TestFacade)
            op_def(mul, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpAsterisk) {
            int value = 12;
            pro::proxy<TestFacade> p = &value;
            ASSERT_EQ(*p * 2, 24);
        }
    }
    namespace op_div {
        interface_def(TestFacade)
            op_def(div, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpSlash) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p / 2, 6);
        }
    }
    namespace op_mod {
        interface_def(TestFacade)
            op_def(mod, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpPercent) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p % 5, 2);
        }
    }
    namespace op_plusplus {
        interface_def(TestFacade)
            op_def(pp, int(), int(int));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpIncrement) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(++(*p), 13);
            ASSERT_EQ((*p)++, 13);
            ASSERT_EQ(v, 14);
        }
    }

    namespace op_nn {
        interface_def(TestFacade)
            op_def(nn, int(), int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpDecrement) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(--(*p), 11);
            ASSERT_EQ((*p)--, 11);
            ASSERT_EQ(v, 10);
        }
    }

    namespace op_eq {
        interface_def(TestFacade)
            op_def(eq, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpEqualTo) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p == 12, true);
            ASSERT_EQ(*p == 11, false);
        }
    }

    namespace op_ne {
        interface_def(TestFacade)
            op_def(ne, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpNotEqualTo) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p != 12, false);
            ASSERT_EQ(*p != 11, true);
        }
    }

    namespace op_gt {
        interface_def(TestFacade)
            op_def(gt, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpGreaterThan) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p > 2, true);
            ASSERT_EQ(*p > 20, false);
        }
    }

    namespace op_lt {
        interface_def(TestFacade)
            op_def(lt, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLessThan) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p < 2, false);
            ASSERT_EQ(*p < 20, true);
        }
    }

    namespace op_ge {
        interface_def(TestFacade)
            op_def(ge, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpGreaterThanOrEqualTo) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p >= 20, false);
            ASSERT_EQ(*p >= 12, true);
        }
    }

    namespace op_le {
        interface_def(TestFacade)
            op_def(le, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLessThanOrEqualTo) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p <= 2, false);
            ASSERT_EQ(*p <= 12, true);
        }
    }

    namespace op_lognot {
        interface_def(TestFacade)
            op_def(log_not, bool());
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLogicalNot) {
            int v1 = 12, v2 = 0;
            pro::proxy<TestFacade> p1 = &v1, p2 = &v2;
            ASSERT_EQ(!*p1, false);
            ASSERT_EQ(!*p2, true);
        }
    }

    namespace op_logand {
        interface_def(TestFacade)
            op_def(log_and, bool(bool val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLogicalAnd) {

            bool v = true;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p && true, true);
            ASSERT_EQ(*p && false, false);
        }
    }

    namespace op_logor {
        interface_def(TestFacade)
            op_def(log_or, bool(bool val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLogicalOr) {
            bool v = false;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p || true, true);
            ASSERT_EQ(*p || false, false);
        }
    }

    namespace op_not {
        interface_def(TestFacade)
            op_def(not, int());
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpTilde) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(~*p, -13);
        }
    }

    namespace op_and {
        interface_def(TestFacade)
            op_def(and, const void*() noexcept, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpAmpersand) {

            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(&*p, &v);
            ASSERT_EQ(*p & 4, 4);
        }
    }

    namespace op_or {
        interface_def(TestFacade)
            op_def(or, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpPipe) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p | 6, 14);
        }
    }

    namespace op_xor {
        interface_def(TestFacade)
            op_def(xor, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpCaret) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p ^ 5, 9);
        }
    }

    namespace op_shl {
        interface_def(TestFacade)
            op_def(shl, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLeftShift) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p << 2, 48);
        }
    }

    namespace op_shr {
        interface_def(TestFacade)
            op_def(shr, int(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpRightShift) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(*p >> 2, 3);
        }
    }

    namespace op_sadd {
        interface_def(TestFacade)
            op_def(set_add, void(int val));
            op_direct_def(set_add, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpPlusAssignment) {
            int v[3] = { 12, 0, 7 };
            pro::proxy<TestFacade> p = v;
            (*p += 2) += 3;
            p += 2;
            *p += 100;
            ASSERT_EQ(v[0], 17);
            ASSERT_EQ(v[1], 0);
            ASSERT_EQ(v[2], 107);
        }
    }

    namespace op_ssub {
        interface_def(TestFacade)
            op_def(set_sub, void(int val));
            op_direct_def(set_sub, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpMinusAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p -= 2) -= 3;
            ASSERT_EQ(v, 7);
        }
    }

    namespace op_smul {
        interface_def(TestFacade)
            op_def(set_mul, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpMultiplicationAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p *= 2) *= 3;
            ASSERT_EQ(v, 72);
        }
    }

    namespace op_sdiv {
        interface_def(TestFacade)
            op_def(set_div, void(int val));
        interface_end(TestFacade);

        TEST(ProxyDispatchTests, TestOpDivisionAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p /= 2) /= 2;
            ASSERT_EQ(v, 3);
        }
    }

    namespace op_sand {
        interface_def(TestFacade)
            op_def(set_and, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpBitwiseAndAssignment) {
            int v = 15;
            pro::proxy<TestFacade> p = &v;
            (*p &= 11) &= 14;
            ASSERT_EQ(v, 10);
        }
    }

    namespace op_sor {
        interface_def(TestFacade)
            op_def(set_or, void(int val));
        interface_end(TestFacade);

        TEST(ProxyDispatchTests, TestOpBitwiseOrAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p |= 2) |= 1;
            ASSERT_EQ(v, 15);
        }
    }

    namespace op_sxor {
        interface_def(TestFacade)
            op_def(set_xor, void(int val));
        interface_end(TestFacade);

        TEST(ProxyDispatchTests, TestOpBitwiseXorAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p ^= 6) ^= 1;
            ASSERT_EQ(v, 11);
        }

    }
    namespace op_sshl {
        interface_def(TestFacade)
            op_def(set_shl, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpLeftShiftAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p <<= 2) <<= 1;
            ASSERT_EQ(v, 96);
        }
    }

    namespace op_sshr {
        interface_def(TestFacade)
            op_def(set_shr, void(int val));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpRightShiftAssignment) {
            int v = 12;
            pro::proxy<TestFacade> p = &v;
            (*p >>= 2) >>= 1;
            ASSERT_EQ(v, 1);
        }
    }

    namespace op_memptr {
        struct Base1 {
            int a;
            int b;
            int c;
        };
        struct Base2 {
            double x;
        };
        struct Derived1 : Base1 {
            int x;
        };
        struct Derived2 : Base2, Base1 {
            int d;
        };

        interface_def(TestFacade)
            support_copy(pro::constraint_level::nontrivial);
            op_direct_def(memptr, int&(int Base1::*ptm));
        interface_end(TestFacade);

        TEST(ProxyDispatchTests, TestOpPtrToMem) {
            Derived1 v1 {};
            Derived2 v2 {};
            pro::proxy<TestFacade> p1 = &v1;
            pro::proxy<TestFacade> p2 = &v2;
            std::vector<int Base1::*> fields { &Base1::a, &Base1::b, &Base1::c };
            for (auto i = 0u; i < (fields.size()); ++i) {
                p1->*fields[i] = i + 1;
                p2->*fields[i] = i + 1;
            }
            ASSERT_EQ(v1.a, 1);
            ASSERT_EQ(v1.b, 2);
            ASSERT_EQ(v1.c, 3);
            ASSERT_EQ(v2.a, 1);
            ASSERT_EQ(v2.b, 2);
            ASSERT_EQ(v2.c, 3);
        }
    }

    namespace op_call {
        interface_def(TestFacade)
            op_def(call, int(int a, int b));
        interface_end(TestFacade);

        TEST(ProxyDispatchTests, TestOpParentheses) {
            auto v = [](auto&&... args) { return (args + ...); };
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ((*p)(2, 3), 5);
        }
    }

    namespace op_index {
        interface_def(TestFacade)
            op_def(index, int&(int idx));
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestOpBrackets) {
            std::unordered_map<int, int> v;
            pro::proxy<TestFacade> p = &v;
            (*p)[3] = 12;
            ASSERT_EQ(v.size(), 1u);
            ASSERT_EQ(v.at(3), 12);
        }
    }

    namespace free2mem {
        interface_def(TestFacade)
            add_conv(FreeMemToString, std::string() const);
        interface_end(TestFacade);
        TEST(ProxyDispatchTests, TestFreeAsMemDispatch) {
            int v = 123;
            pro::proxy<TestFacade> p = &v;
            ASSERT_EQ(p->ToString(), "123");
        }
    }
}