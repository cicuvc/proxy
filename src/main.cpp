#include "proxy.hpp"
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

interface_def(Drawable)
    support_copy(pro::constraint_level::nontrivial);
    support_relocate(pro::constraint_level::nontrivial);
    support_destroy(pro::constraint_level::nontrivial);
    fn_def(Area, float() const, float(float) const);
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
    Rectangle(float width_, float height_) : width(width_), height(height_) {}
    Rectangle(const Rectangle&) = default;
    Rectangle(Rectangle&& rhs) : width(rhs.width), height(rhs.height) {
        rhs.width = 0;
        rhs.height = 0;
    }
    float Area() const { return width * height; }
    float Area(float bias) const { return width * height + bias; }

    void Info() const {
        auto s = string_format("Rectangle[w = %.2f, h = %.2f]", width, height);
        std::cout << s << std::endl;
    }

    ~Rectangle() = default;

private:
    float width;
    float height;
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

int main() {
    auto vec_p = pro::make_proxy<Vec<int>>(std::vector<int> {});
    vec_p->push_back(12);
    std::cout << (*vec_p)[0] << std::endl;

    custom_allocator<Rectangle> allocator;
    pro::details::static_meta_manager::register_facade_allocated<Rectangle, InfoOnly>(allocator);

    auto proxy = pro::make_proxy_inplace<Drawable, Rectangle>(1.0, 2.0);

    auto nf = proxy.meta_->poly_cast_meta::cast_move<InfoOnly>(proxy, std::optional(allocator));

    (nf.value())->Info();
}