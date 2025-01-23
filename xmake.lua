add_rules("mode.debug", "mode.release")
add_requires("gtest >=1.8.1")

target("proxy")
    set_kind("binary")
    set_toolchains('clang')
    add_includedirs("inc")
    add_files("src/*.cpp")
    add_cxxflags("-std=c++17","-Werror","-Wall","-Wextra","-fstrict-aliasing","-Wstrict-aliasing","-ftemplate-backtrace-limit=0")
    add_ldflags("-lgtest")

