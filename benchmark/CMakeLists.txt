cmake_minimum_required(VERSION 3.16)
project(lpbench_threshold_benchmark LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-O3 -DNDEBUG -march=native -Wall -Wextra -Wpedantic)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" BENCH_COMPILER_HAS_AVX2)
check_cxx_compiler_flag("-mavx512f" BENCH_COMPILER_HAS_AVX512F)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(BENCH_EXTERNAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(BENCH_LINK_LIBS)
set(BENCH_DEFINES)

if(EXISTS ${BENCH_EXTERNAL_ROOT}/lv89/lv89.c)
  add_library(lv89_impl STATIC ${BENCH_EXTERNAL_ROOT}/lv89/lv89.c)
  target_include_directories(lv89_impl PUBLIC ${BENCH_EXTERNAL_ROOT}/lv89)
  list(APPEND BENCH_LINK_LIBS lv89_impl)
  list(APPEND BENCH_DEFINES BENCH_HAS_LV89=1)
endif()

if(EXISTS ${BENCH_EXTERNAL_ROOT}/miniwfa/miniwfa.c)
  add_library(miniwfa_impl STATIC
    ${BENCH_EXTERNAL_ROOT}/miniwfa/miniwfa.c
    ${BENCH_EXTERNAL_ROOT}/miniwfa/kalloc.c
  )
  target_include_directories(miniwfa_impl PUBLIC ${BENCH_EXTERNAL_ROOT}/miniwfa)
  target_compile_options(miniwfa_impl PRIVATE -msse4)
  list(APPEND BENCH_LINK_LIBS miniwfa_impl)
  list(APPEND BENCH_DEFINES BENCH_HAS_MINIWFA=1)
endif()

if(EXISTS ${BENCH_EXTERNAL_ROOT}/WFA2-lib/CMakeLists.txt)
  add_subdirectory(${BENCH_EXTERNAL_ROOT}/WFA2-lib ${CMAKE_CURRENT_BINARY_DIR}/WFA2-lib EXCLUDE_FROM_ALL)
  list(APPEND BENCH_LINK_LIBS wfa2cpp_static)
  list(APPEND BENCH_DEFINES BENCH_HAS_WFA2=1)
endif()

set(LEAP_COMMON_SOURCES
  ${BENCH_EXTERNAL_ROOT}/leap/SIMD_ED.cc
  ${BENCH_EXTERNAL_ROOT}/leap/SHD.cc
  ${BENCH_EXTERNAL_ROOT}/leap/mask.c
  ${BENCH_EXTERNAL_ROOT}/leap/print.c
  ${BENCH_EXTERNAL_ROOT}/leap/bit_convert.c
  ${BENCH_EXTERNAL_ROOT}/leap/shift.c
  ${BENCH_EXTERNAL_ROOT}/leap/popcount.c
)
set_source_files_properties(${LEAP_COMMON_SOURCES} PROPERTIES LANGUAGE CXX)

if(EXISTS ${BENCH_EXTERNAL_ROOT}/leap/SIMD_ED.cc)
  add_library(leap_avx2_impl STATIC
    src/leap_avx2_bridge.cpp
    ${LEAP_COMMON_SOURCES}
  )
  target_include_directories(leap_avx2_impl PUBLIC ${BENCH_EXTERNAL_ROOT}/leap)
  target_compile_options(leap_avx2_impl PRIVATE -mbmi -mavx2 -msse4.2 -fpermissive)
  target_compile_definitions(leap_avx2_impl PRIVATE SIMD_ED=SIMD_ED_AVX2)
  list(APPEND BENCH_LINK_LIBS leap_avx2_impl)
  list(APPEND BENCH_DEFINES BENCH_HAS_LEAP_AVX2=1)

  add_library(leap_avx512_impl STATIC
    src/leap_avx512_bridge.cpp
    ${LEAP_COMMON_SOURCES}
  )
  target_include_directories(leap_avx512_impl PUBLIC ${BENCH_EXTERNAL_ROOT}/leap)
  target_compile_options(leap_avx512_impl PRIVATE -mbmi -mavx512f -mavx512bw -mavx512vl -msse4.2 -fpermissive)
  target_compile_definitions(leap_avx512_impl PRIVATE
    USE_AVX512
    SIMD_ED=SIMD_ED_AVX512
    c_convert2bit=leap512_c_convert2bit
    sse_convert2bit=leap512_sse_convert2bit
    avx_convert2bit=leap512_avx_convert2bit
    avx512_convert2bit=leap512_avx512_convert2bit
    _mm256_loadu2_m128i=leap512_mm256_loadu2_m128i
    _mm256_storeu2_m128i=leap512_mm256_storeu2_m128i
    shift_right_sse=leap512_shift_right_sse
    shift_left_sse=leap512_shift_left_sse
    shift_right_avx=leap512_shift_right_avx
    shift_left_avx=leap512_shift_left_avx
    shift_right_avx512=leap512_shift_right_avx512
    shift_left_avx512=leap512_shift_left_avx512
    flip_false_zero=leap512_flip_false_zero
    bit_vec_filter_sse=leap512_bit_vec_filter_sse
    bit_vec_filter_avx=leap512_bit_vec_filter_avx
    bit_vec_filter_avx512=leap512_bit_vec_filter_avx512
    printbytevector=leap512_printbytevector
    print128_bit=leap512_print128_bit
    print256_bit=leap512_print256_bit
    print128_hex=leap512_print128_hex
    ssse3_popcount_m256_core=leap512_ssse3_popcount_m256_core
    ssse3_popcount_m128_core=leap512_ssse3_popcount_m128_core
    ssse3_popcount_m512_core=leap512_ssse3_popcount_m512_core
    popcount_m128i_sse=leap512_popcount_m128i_sse
    popcount_m256i_avx=leap512_popcount_m256i_avx
    popcount_m512i_avx512=leap512_popcount_m512i_avx512
    popcount_SHD_sse=leap512_popcount_SHD_sse
    popcount_SHD_avx=leap512_popcount_SHD_avx
    popcount_SHD_avx512=leap512_popcount_SHD_avx512
    builtin_popcount=leap512_builtin_popcount
    popcount=leap512_popcount
    MASK_SSE_END=leap512_MASK_SSE_END
    MASK_SSE_BEG=leap512_MASK_SSE_BEG
    MASK_AVX_END=leap512_MASK_AVX_END
    MASK_AVX_BEG=leap512_MASK_AVX_BEG
    MASK_AVX512_END=leap512_MASK_AVX512_END
    MASK_AVX512_BEG=leap512_MASK_AVX512_BEG
    MASK_SRS=leap512_MASK_SRS
    __MASK_SSE_END_=leap512___MASK_SSE_END_
    __MASK_SSE_BEG_=leap512___MASK_SSE_BEG_
    __MASK_AVX_END_=leap512___MASK_AVX_END_
    __MASK_AVX_BEG_=leap512___MASK_AVX_BEG_
    __MASK_AVX512_END_=leap512___MASK_AVX512_END_
    __MASK_AVX512_BEG_=leap512___MASK_AVX512_BEG_
    __MASK_SRS_=leap512___MASK_SRS_
    BASE_SHIFT1=leap512_BASE_SHIFT1
    BASE_SHIFT2=leap512_BASE_SHIFT2
    BIT_FF=leap512_BIT_FF
    LOC_MASK_SSE=leap512_LOC_MASK_SSE
    LOC_MASK_AVX=leap512_LOC_MASK_AVX
    MASK_A=leap512_MASK_A
    MASK_C=leap512_MASK_C
    MASK_G=leap512_MASK_G
    MASK_T=leap512_MASK_T
    __MASK_0F_=leap512___MASK_0F_
    POPCOUNT=leap512_POPCOUNT
    POPCOUNT_SHD=leap512_POPCOUNT_SHD
  )
  list(APPEND BENCH_LINK_LIBS leap_avx512_impl)
  list(APPEND BENCH_DEFINES BENCH_HAS_LEAP_AVX512=1)
endif()

set(BENCH_COMMON_SRC
  src/trusted_edit_distance.cpp
)

add_executable(dataset_generate
  src/dataset_generate.cpp
  ${BENCH_COMMON_SRC}
)

add_executable(threshold_bench
  src/threshold_bench.cpp
  src/method_leap_avx2.cpp
  src/method_leap_avx512.cpp
  src/method_lv89.cpp
  src/method_miniwfa.cpp
  src/method_wfa2.cpp
  ${BENCH_COMMON_SRC}
)
target_compile_definitions(threshold_bench PRIVATE ${BENCH_DEFINES})
target_include_directories(threshold_bench PRIVATE
  ${BENCH_EXTERNAL_ROOT}/lv89
  ${BENCH_EXTERNAL_ROOT}/miniwfa
  ${BENCH_EXTERNAL_ROOT}/WFA2-lib
)
target_link_libraries(threshold_bench PRIVATE ${BENCH_LINK_LIBS})

add_executable(leap_phase_bench
  src/leap_phase_bench.cpp
  ${BENCH_COMMON_SRC}
)
target_compile_definitions(leap_phase_bench PRIVATE ${BENCH_DEFINES})
target_link_libraries(leap_phase_bench PRIVATE ${BENCH_LINK_LIBS})

add_executable(leap_raw_warm_kernel_bench
  src/leap_raw_warm_kernel_bench.cpp
  ${BENCH_COMMON_SRC}
)
target_compile_definitions(leap_raw_warm_kernel_bench PRIVATE ${BENCH_DEFINES})
target_link_libraries(leap_raw_warm_kernel_bench PRIVATE ${BENCH_LINK_LIBS})

if(BENCH_COMPILER_HAS_AVX2)
  set_source_files_properties(src/method_leap_avx2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2")
else()
  message(WARNING "Compiler does not accept -mavx2; leap_avx2 will fail clearly at runtime if selected")
endif()

if(BENCH_COMPILER_HAS_AVX512F)
  set_source_files_properties(src/method_leap_avx512.cpp PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw")
else()
  message(WARNING "Compiler does not accept AVX512 flags; leap_avx512 will fail clearly at runtime if selected")
endif()

add_custom_target(threshold_verify_bench DEPENDS
  threshold_bench
  leap_phase_bench
  leap_raw_warm_kernel_bench
  dataset_generate
)
