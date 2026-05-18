cmake_minimum_required(VERSION 3.20)
project(leapbench_edit_distance LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_compile_options(-O3)

set(EDLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EDLIB_BUILD_UTILITIES OFF CACHE BOOL "" FORCE)
set(EDLIB_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${REPO_ROOT}/edlib ${CMAKE_BINARY_DIR}/edlib-build EXCLUDE_FROM_ALL)

add_executable(edlib_gt wrappers/edlib_gt.cpp)
target_link_libraries(edlib_gt PRIVATE edlib)

add_library(lv89_obj OBJECT ${REPO_ROOT}/lv89/lv89.c)
target_include_directories(lv89_obj PUBLIC ${REPO_ROOT}/lv89)
target_compile_options(lv89_obj PRIVATE -O3)

add_executable(lv89_bench wrappers/lv89_bench.cpp $<TARGET_OBJECTS:lv89_obj>)
target_include_directories(lv89_bench PRIVATE wrappers ${REPO_ROOT}/lv89)

add_library(miniwfa_obj OBJECT ${REPO_ROOT}/miniwfa/miniwfa.c ${REPO_ROOT}/miniwfa/kalloc.c)
target_include_directories(miniwfa_obj PUBLIC ${REPO_ROOT}/miniwfa)
target_compile_options(miniwfa_obj PRIVATE -O3)

add_executable(miniwfa_bench wrappers/miniwfa_bench.cpp $<TARGET_OBJECTS:miniwfa_obj>)
target_include_directories(miniwfa_bench PRIVATE wrappers ${REPO_ROOT}/miniwfa)

set(OPENMP OFF CACHE BOOL "" FORCE)
add_subdirectory(${REPO_ROOT}/WFA2-lib ${CMAKE_BINARY_DIR}/wfa2-build EXCLUDE_FROM_ALL)

add_executable(wfa2_bench wrappers/wfa2_bench.cpp)
target_include_directories(wfa2_bench PRIVATE wrappers ${REPO_ROOT}/WFA2-lib)
target_link_libraries(wfa2_bench PRIVATE wfa2_static)

set(LEAP_SOURCES
  ${REPO_ROOT}/leap/SIMD_ED.cc
  ${REPO_ROOT}/leap/SHD.cc
  ${REPO_ROOT}/leap/mask.c
  ${REPO_ROOT}/leap/print.c
  ${REPO_ROOT}/leap/bit_convert.c
  ${REPO_ROOT}/leap/shift.c
  ${REPO_ROOT}/leap/popcount.c
)
set_source_files_properties(
  ${REPO_ROOT}/leap/mask.c
  ${REPO_ROOT}/leap/print.c
  ${REPO_ROOT}/leap/bit_convert.c
  ${REPO_ROOT}/leap/shift.c
  ${REPO_ROOT}/leap/popcount.c
  PROPERTIES LANGUAGE CXX
)

add_library(leap_avx2_obj OBJECT ${LEAP_SOURCES})
target_include_directories(leap_avx2_obj PUBLIC ${REPO_ROOT}/leap)
target_compile_options(leap_avx2_obj PRIVATE -O3 -std=c++11 -mbmi -mavx2 -msse4.2 -fpermissive)

add_library(leap_avx512_obj OBJECT ${LEAP_SOURCES})
target_include_directories(leap_avx512_obj PUBLIC ${REPO_ROOT}/leap)
target_compile_definitions(leap_avx512_obj PRIVATE USE_AVX512)
target_compile_options(leap_avx512_obj PRIVATE -O3 -std=c++11 -mbmi -mavx2 -msse4.2 -mavx512f -mavx512bw -mavx512vl -fpermissive)

add_executable(leap_avx2_bench wrappers/leap_bench.cpp $<TARGET_OBJECTS:leap_avx2_obj>)
target_include_directories(leap_avx2_bench PRIVATE wrappers ${REPO_ROOT}/leap)
target_compile_options(leap_avx2_bench PRIVATE -mbmi -mavx2 -msse4.2)

add_executable(leap_avx512_bench wrappers/leap_bench.cpp $<TARGET_OBJECTS:leap_avx512_obj>)
target_include_directories(leap_avx512_bench PRIVATE wrappers ${REPO_ROOT}/leap)
target_compile_definitions(leap_avx512_bench PRIVATE USE_AVX512)
target_compile_options(leap_avx512_bench PRIVATE -mbmi -mavx2 -msse4.2 -mavx512f -mavx512bw -mavx512vl)
