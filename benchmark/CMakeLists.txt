cmake_minimum_required(VERSION 3.16)
project(leapbench LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BENCH_USE_LEAP "Build native LEAP wrapper" ON)
option(BENCH_USE_EDLIB "Build native edlib wrapper" ON)
option(BENCH_USE_LV89 "Build native lv89 wrapper" ON)
option(BENCH_USE_MINIWFA "Build native miniwfa wrapper" ON)
option(BENCH_USE_KSW2 "Build native ksw2 wrapper" ON)
option(BENCH_USE_WFA2 "Build native WFA2 wrapper" ON)

add_compile_options($<$<COMPILE_LANGUAGE:C>:-O3> $<$<COMPILE_LANGUAGE:CXX>:-O3> -DNDEBUG -march=native)

set(BENCH_SOURCES
  src/bench_main.cpp
  src/dataset_io.cpp
  src/timer.cpp
  src/checksum.cpp
  src/edit_oracle.cpp
  src/affine_oracle.cpp
  src/wrappers/oracle_wrapper.cpp
  src/wrappers/leap_wrapper.cpp
  src/wrappers/edlib_wrapper.cpp
  src/wrappers/lv89_wrapper.cpp
  src/wrappers/miniwfa_wrapper.cpp
  src/wrappers/ksw2_wrapper.cpp
  src/wrappers/parasail_wrapper.cpp
  src/wrappers/wfa2_wrapper.cpp
)

add_executable(bench_align ${BENCH_SOURCES})
target_include_directories(bench_align PRIVATE include)

if(BENCH_USE_EDLIB AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../edlib/edlib/src/edlib.cpp")
  add_library(bench_edlib STATIC ../edlib/edlib/src/edlib.cpp)
  target_include_directories(bench_edlib PUBLIC ../edlib/edlib/include)
  target_link_libraries(bench_align PRIVATE bench_edlib)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_EDLIB)
endif()

if(BENCH_USE_LV89 AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../lv89/lv89.c")
  add_library(bench_lv89 STATIC ../lv89/lv89.c)
  target_include_directories(bench_lv89 PUBLIC ../lv89)
  target_link_libraries(bench_align PRIVATE bench_lv89)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_LV89)
endif()

if(BENCH_USE_MINIWFA AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../miniwfa/miniwfa.c")
  add_library(bench_miniwfa STATIC ../miniwfa/miniwfa.c ../miniwfa/kalloc.c)
  target_include_directories(bench_miniwfa PUBLIC ../miniwfa)
  target_link_libraries(bench_align PRIVATE bench_miniwfa)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_MINIWFA)
endif()

if(BENCH_USE_KSW2 AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../ksw2/ksw2_gg2.c")
  add_library(bench_ksw2 STATIC ../ksw2/ksw2_gg2.c)
  target_include_directories(bench_ksw2 PUBLIC ../ksw2)
  target_link_libraries(bench_align PRIVATE bench_ksw2)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_KSW2)
endif()

if(BENCH_USE_LEAP AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../leap/SIMD_ED.cc")
  add_library(bench_leap STATIC
    ../leap/SIMD_ED.cc
    ../leap/avx512_shift.c
    ../leap/print.c
    src/wrappers/leap_bit_convert_shim.cpp
  )
  target_include_directories(bench_leap PUBLIC ../leap)
  target_compile_options(bench_leap PRIVATE -mavx512f -mavx512bw -mavx512vl -mbmi)
  target_compile_options(bench_align PRIVATE -mavx512f -mavx512bw -mavx512vl -mbmi)
  target_link_libraries(bench_align PRIVATE bench_leap)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_LEAP)
endif()

if(BENCH_USE_WFA2 AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../WFA2-lib/CMakeLists.txt")
  add_subdirectory(../WFA2-lib wfa2-build EXCLUDE_FROM_ALL)
  target_link_libraries(bench_align PRIVATE wfa2cpp_static)
  target_compile_definitions(bench_align PRIVATE BENCH_ENABLE_WFA2)
endif()

target_link_libraries(bench_align PRIVATE m)
