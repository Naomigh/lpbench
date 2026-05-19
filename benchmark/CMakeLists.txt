cmake_minimum_required(VERSION 3.16)
project(threshold_verify_bench C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(BENCH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/threshold_verify_bench.cpp)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(LEAP_SOURCES
  ${ROOT_DIR}/leap/SIMD_ED.cc
  ${ROOT_DIR}/leap/SHD.cc
  ${ROOT_DIR}/leap/bit_convert.c
  ${ROOT_DIR}/leap/mask.c
  ${ROOT_DIR}/leap/popcount.c
  ${ROOT_DIR}/leap/print.c
  ${ROOT_DIR}/leap/shift.c
)
set_source_files_properties(${LEAP_SOURCES} PROPERTIES LANGUAGE CXX)

function(add_leap_target target method_flag)
  add_executable(${target} ${BENCH_SRC} ${LEAP_SOURCES})
  target_include_directories(${target} PRIVATE ${ROOT_DIR}/leap)
  target_compile_definitions(${target} PRIVATE ${method_flag})
  target_compile_options(${target} PRIVATE -O3 -mbmi -mavx2 -msse4.2 -fpermissive)
endfunction()

add_leap_target(threshold_verify_leap_avx2 METHOD_LEAP_AVX2)

add_executable(threshold_verify_leap_avx512 ${BENCH_SRC} ${LEAP_SOURCES})
target_include_directories(threshold_verify_leap_avx512 PRIVATE ${ROOT_DIR}/leap)
target_compile_definitions(threshold_verify_leap_avx512 PRIVATE METHOD_LEAP_AVX512 USE_AVX512)
target_compile_options(threshold_verify_leap_avx512 PRIVATE -O3 -mbmi -mavx512f -mavx512bw -mavx512vl -fpermissive)

add_executable(threshold_verify_miniwfa
  ${BENCH_SRC}
  ${ROOT_DIR}/miniwfa/miniwfa.c
  ${ROOT_DIR}/miniwfa/kalloc.c
)
target_include_directories(threshold_verify_miniwfa PRIVATE ${ROOT_DIR}/miniwfa)
target_compile_definitions(threshold_verify_miniwfa PRIVATE METHOD_MINIWFA)
target_compile_options(threshold_verify_miniwfa PRIVATE -O3 -march=native)

add_executable(threshold_verify_lv89
  ${BENCH_SRC}
  ${ROOT_DIR}/lv89/lv89.c
)
target_include_directories(threshold_verify_lv89 PRIVATE ${ROOT_DIR}/lv89)
target_compile_definitions(threshold_verify_lv89 PRIVATE METHOD_LV89)
target_compile_options(threshold_verify_lv89 PRIVATE -O3 -march=native)

set(OPENMP OFF CACHE BOOL "" FORCE)
add_subdirectory(${ROOT_DIR}/WFA2-lib ${CMAKE_CURRENT_BINARY_DIR}/wfa2-build EXCLUDE_FROM_ALL)

add_executable(threshold_verify_wfa2 ${BENCH_SRC})
target_include_directories(threshold_verify_wfa2 PRIVATE ${ROOT_DIR}/WFA2-lib)
target_compile_definitions(threshold_verify_wfa2 PRIVATE METHOD_WFA2)
target_compile_options(threshold_verify_wfa2 PRIVATE -O3 -march=native)
target_link_libraries(threshold_verify_wfa2 PRIVATE wfa2_static m pthread)

add_custom_target(threshold_verify_bench DEPENDS
  threshold_verify_leap_avx2
  threshold_verify_leap_avx512
  threshold_verify_miniwfa
  threshold_verify_lv89
  threshold_verify_wfa2
)
