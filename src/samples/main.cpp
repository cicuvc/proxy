#include <cstddef>
#include <proxy.hpp>

struct TestFacade : pro::facade_builder ::support_relocation<
                            pro::constraint_level::trivial>::support_copy<pro::constraint_level::trivial>
                        ::add_direct_convention<pro::explicit_conversion_dispatch, long() const>
                        ::build {};

int main(){
    pro::proxy<TestFacade> a = pro::make_proxy<TestFacade>(12);
    long gex = long{a};
    std::cout << (gex) << std::endl;

    return 0;
}