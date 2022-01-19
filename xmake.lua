add_requires("zig 0.7.x")

target("hello")
    set_kind("binary")
    add_files("src/*.cpp")
