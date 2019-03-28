project('volund', ['cpp', 'c'], default_options: ['cpp_std=c++17'])

cpp = meson.get_compiler('cpp')
srcs = ['main.cpp', 'glad.c']
deps = [dependency('SDL2'), cpp.find_library('dl'), cpp.find_library('stdc++fs')]

volund_exe = executable('volund', srcs, dependencies: deps)
