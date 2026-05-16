cmake_minimum_required(VERSION 3.20)
project(zx_basic_c C)

# We gebruiken de C11 of C17 standaard (modern genoeg, overal ondersteund)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Handig voor CachyOS/Linux: voeg waarschuwingen toe zodat de compiler streng is
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dit definieert onze executable en welke bestanden erbij horen
add_executable(zx_basic_c main.c)
