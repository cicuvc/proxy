#include <proxy.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include <unordered_map>
#include <vector>

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
        const SboReflector& ReflectSbo() const noexcept { return pro::proxy_reflect<IsDirect, R>(pro::access_proxy<F>(*this)); }
    };

    bool SboEnabled;
    bool AllocatorAllocatesForItself;
};


interface_def(Drawable)
    support_copy(pro::constraint_level::nontrivial);
    support_relocate(pro::constraint_level::nontrivial);
    support_destroy(pro::constraint_level::nontrivial);
    add_direct_reflect(SboReflector);
    fn_def(Area, double() const, double(double) const);
    fn_def(Info, void() const);
interface_end(Drawable);

interface_def(InfoOnly)
    support_copy(pro::constraint_level::nontrivial);
    support_relocate(pro::constraint_level::nontrivial);
    support_destroy(pro::constraint_level::nontrivial);
    fn_def(Info, void() const);
interface_end(InfoOnly);

interface_def_generic(Vec, class T)
    support_copy(pro::constraint_level::nontrivial);
    support_relocate(pro::constraint_level::nontrivial);
    support_destroy(pro::constraint_level::nontrivial);
    fn_def(push_back, void(const T&));
    op_def(index, T & (size_t index) noexcept);
interface_end_generic(Vec);

interface_def(InfoOnlyLowSize)
    restrict_layout(8, 8);
    support_copy(pro::constraint_level::nontrivial);
    support_relocate(pro::constraint_level::nontrivial);
    support_destroy(pro::constraint_level::nontrivial);
    add_direct_reflect(SboReflector);
    fn_def(Info, void() const);
interface_end(InfoOnlyLowSize);

template <typename... Args> std::string string_format(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

struct Rectangle {
    Rectangle(double width_, double height_) : width(width_), height(height_) {}
    Rectangle(const Rectangle&) = default;
    Rectangle(Rectangle&& rhs) : width(rhs.width), height(rhs.height) {
        rhs.width = 0;
        rhs.height = 0;
    }
    double Area() const { return width * height; }
    double Area(double bias) const { return width * height + bias; }

    void Info() const {
        auto s = string_format("Rectangle[w = %.2f, h = %.2f]", width, height);
        std::cout << s << std::endl;
    }

    ~Rectangle() = default;

private:
    double width;
    double height;
};

template <typename Tp> struct custom_allocator {
    std::allocator<Tp> internal_alloc;
    typedef Tp value_type;

    Tp* allocate(size_t n) {
        auto ptr = internal_alloc.allocate(n);
        std::cout << string_format("Allocate ptr %p", ptr) << std::endl;
        return ptr;
    }

    void deallocate(Tp* ptr, size_t n) {
        std::cout << string_format("Deallocate ptr %p", ptr) << std::endl;
        return internal_alloc.deallocate(ptr, n);
    }
};

template <typename Tp> struct large_allocator {
    std::byte padding[8];
    std::allocator<Tp> internal_alloc;
    
    typedef Tp value_type;

    large_allocator(){}
    template<typename T>
    large_allocator(const large_allocator<T>&){}

    Tp* allocate(size_t n) {
        auto ptr = internal_alloc.allocate(n);
        std::cout << string_format("Allocate ptr %p", ptr) << std::endl;
        return ptr;
    }

    void deallocate(Tp* ptr, size_t n) {
        std::cout << string_format("Deallocate ptr %p", ptr) << std::endl;
        return internal_alloc.deallocate(ptr, n);
    }
};

PRO_DEF_FREE_DISPATCH(MemToString, std::to_string, ToString);

interface_def(TestLargeStringable);
    support_relocate(pro::constraint_level::nontrivial);
    support_copy(pro::constraint_level::nontrivial);
    add_direct_reflect(SboReflector);
    add_conv(MemToString, std::string());
interface_end(TestLargeStringable);

int main() {
    auto tos = pro::make_proxy<TestLargeStringable>(12138);
    std::cout << ToString(*tos) << std::endl;


    auto low_size = pro::allocate_proxy<InfoOnlyLowSize>(large_allocator<Rectangle>(),Rectangle(1.0, 2.0));
    [[maybe_unused]] const auto& ref_lows = low_size.ReflectSbo();
    
    custom_allocator<Rectangle> allocator;
    pro::details::static_meta_manager::register_facade_allocated<Rectangle, InfoOnly>(allocator);

    auto proxy = pro::make_proxy_inplace<Drawable, Rectangle>(1.0, 2.0);
    auto proxy_alloc = pro::allocate_proxy<Drawable, Rectangle>(allocator, 1.0, 2.0);

    [[maybe_unused]] const auto& reflector = proxy.ReflectSbo();
    [[maybe_unused]] const auto& reflector_ac = proxy_alloc.ReflectSbo();

    auto nf = proxy.meta_->poly_cast_meta::cast_move<InfoOnly>(proxy, std::optional(allocator));

    (nf.value())->Info();
}