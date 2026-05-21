cmake_minimum_required(VERSION 3.16)
project(leap_method_benchmark LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_subdirectory("${REPO_ROOT}/WFA2-lib" "${CMAKE_BINARY_DIR}/wfa2" EXCLUDE_FROM_ALL)

add_library(lv89_bench STATIC "${REPO_ROOT}/lv89/lv89.c")
target_include_directories(lv89_bench PUBLIC "${REPO_ROOT}/lv89")
target_compile_options(lv89_bench PRIVATE -O3)

add_library(miniwfa_bench STATIC
  "${REPO_ROOT}/miniwfa/miniwfa.c"
  "${REPO_ROOT}/miniwfa/kalloc.c")
target_include_directories(miniwfa_bench PUBLIC "${REPO_ROOT}/miniwfa")
target_compile_options(miniwfa_bench PRIVATE -O3)

set(LEAP_SOURCES
  "${REPO_ROOT}/leap/SIMD_ED.cc"
  "${REPO_ROOT}/leap/SHD.cc"
  "${REPO_ROOT}/leap/bit_convert.c"
  "${REPO_ROOT}/leap/mask.c"
  "${REPO_ROOT}/leap/popcount.c"
  "${REPO_ROOT}/leap/print.c"
  "${REPO_ROOT}/leap/shift.c"
  src/leap_backend.cpp)
set_source_files_properties(
  "${REPO_ROOT}/leap/bit_convert.c"
  "${REPO_ROOT}/leap/mask.c"
  "${REPO_ROOT}/leap/popcount.c"
  "${REPO_ROOT}/leap/print.c"
  "${REPO_ROOT}/leap/shift.c"
  PROPERTIES LANGUAGE CXX)

function(add_leap_backend target_name)
  add_library(${target_name} SHARED ${LEAP_SOURCES})
  target_include_directories(${target_name} PRIVATE "${REPO_ROOT}/leap" src)
  target_compile_options(${target_name} PRIVATE -O3 -mbmi -msse4.2 -fpermissive)
endfunction()

add_leap_backend(leap_backend_avx2)
target_compile_options(leap_backend_avx2 PRIVATE -mavx2)
target_compile_definitions(leap_backend_avx2 PRIVATE LEAP_CHUNK_SIZE=256)

add_leap_backend(leap_backend_avx512)
target_compile_options(leap_backend_avx512 PRIVATE -mavx2 -mavx512f -mavx512bw -mavx512vl)
target_compile_definitions(leap_backend_avx512 PRIVATE USE_AVX512 LEAP_CHUNK_SIZE=512)

add_executable(bench_methods src/bench_methods.cpp)
target_include_directories(bench_methods PRIVATE
  src
  "${REPO_ROOT}/lv89"
  "${REPO_ROOT}/miniwfa"
  "${REPO_ROOT}/WFA2-lib")
target_compile_options(bench_methods PRIVATE -O3)
target_link_libraries(bench_methods PRIVATE lv89_bench miniwfa_bench wfa2_static dl m)
add_dependencies(bench_methods leap_backend_avx2 leap_backend_avx512)

