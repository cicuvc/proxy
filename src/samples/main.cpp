#include <cstddef>
#include <proxy.hpp>

interface_def(TestFacade)
    fn_def(TestFunc, void() noexcept);
interface_end(TestFacade);


int main(){
    pro::proxy<TestFacade> a;
    std::cout << (nullptr == a) << std::endl;
    return 0;
}