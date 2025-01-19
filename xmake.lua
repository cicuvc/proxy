add_rules("mode.debug", "mode.release")

target("proxy")
    set_kind("binary")
    set_toolchains('clang')
    add_includedirs("inc")
    add_files("src/*.cpp")
    add_cxxflags("-std=c++17","-Werror","-Wall","-Wextra","-fstrict-aliasing","-Wstrict-aliasing")

