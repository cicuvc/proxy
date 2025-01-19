# Proxy: Next Generation Polymorphism in C++

This repository is an adapted version of the MSFT non-invasive polymorphism library [proxy](https://github.com/microsoft/proxy), allowing it to be compiled and used with C++17 standard. 
The code is currently experimental. **DO NOT use it for production.**

Changes:
- Remove all require/concept semantics to adapt to c++17
- Better interface definition syntax
    ```cpp
    // ====================================================
    // Original synatx
    PRO_DEF_MEM_DISPATCH(MemArea, Area);
    PRO_DEF_MEM_DISPATCH(MemInfo, Info);

    struct Drawable: pro::facade_builder
        ::add_convention<MemArea, double() const, double(double) const>
        ::add_convention<MemInfo, void() const>
        ::support_copy<pro::constraint_level::nontrivial>
        ::build {};

    // ====================================================
    // New synatx
    interface_def(Drawable){
        support_copy(pro::constraint_level::nontrivial);

        fn_def(Area, double() const, double(double) const);
        fn_def(Info, void() const);
    } interface_end(Drawable);
    ```
- Add conversion support between proxy<facades>

Known issues:
- Potential issues with interface explicit/implicit type conversion
- Potential issues with qualifier-checking in function signature match
- Compile error messages not friendly enough



## Introduction for official proxy repo

"Proxy" is a modern C++ library that helps you use polymorphism (a way to use different types of objects interchangeably) without needing inheritance.

"Proxy" was created by Microsoft engineers and has been used in the Windows operating system since 2022. For many years, using inheritance was the main way to achieve polymorphism in C++. However, new programming languages like [Rust](https://doc.rust-lang.org/book/ch10-02-traits.html) offer better ways to do this. We have improved our understanding of object-oriented programming and decided to use *pointers* in C++ as the foundation for "Proxy". Specifically, the "Proxy" library is designed to be:

- **Portable**: "Proxy" was implemented as a single-header library in standard C++20. It can be used on any platform while the compiler supports C++20. The majority of the library is [freestanding](https://en.cppreference.com/w/cpp/freestanding), making it feasible for embedded engineering or kernel design of an operating system.
- **Non-intrusive**: An implementation type is no longer required to inherit from an abstract binding.
- **Well-managed**: "Proxy" provides a GC-like capability that manages the lifetimes of different objects efficiently without the need for an actual garbage collector.
- **Fast**: With typical compiler optimizations, "Proxy" produces high-quality code that is as good as or better than hand-written code. In many cases, "Proxy" performs better than traditional inheritance-based approaches, especially in managing the lifetimes of objects.
- **Accessible**: Learned from user feedback, accessibility has been significantly improved in "Proxy 3" with intuitive syntax, good IDE compatibility, and accurate diagnostics.
- **Flexible**: Not only member functions, the "abstraction" of "Proxy" allows *any* expression to be polymorphic, including free functions, operators, conversions, etc. Different abstractions can be freely composed on demand. Performance tuning is supported for experts to balance between extensibility and performance.


