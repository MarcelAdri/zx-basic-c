cmake_minimum_required(VERSION 3.20)
project(zx_basic_c VERSION 0.4.6 LANGUAGES C)

# Vertel CMake dat hij een gegenereerd headerbestand moet maken
# op basis van onze template 'version.h.in'
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
)

# Zorg ervoor dat de compiler de map met het gegenereerde bestand kan vinden
include_directories("${CMAKE_CURRENT_BINARY_DIR}/generated")

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
add_executable(zx_basic_c src/main.c
        include/machine.h
        src/machine.c
        include/errors.h
        src/errors.c
        src/execute.c
        include/execute.h
        src/expressions.c
        include/expressions.h
        src/helpers.c
        include/helpers.h
        include/functions.h
        src/functions.c
        src/characters.c
        include/characters.h
        include/zx_types.h
        src/zx_types.c
        include/main.h
)

target_include_directories(zx_basic_c PRIVATE include)

