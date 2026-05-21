cmake_minimum_required(VERSION 3.16)
project(lpbench_threshold_benchmark C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

add_compile_options(-O3 -DNDEBUG -march=native)

set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(LEAP_DIR "${ROOT_DIR}/leap")
set(LV89_DIR "${ROOT_DIR}/lv89")
set(MINIWFA_DIR "${ROOT_DIR}/miniwfa")
set(WFA2_DIR "${ROOT_DIR}/WFA2-lib")

add_library(bench_common STATIC
  src/trusted_edit_distance.cpp
)
target_include_directories(bench_common PUBLIC include)

add_executable(dataset_generate
  src/dataset_generate.cpp
)
target_link_libraries(dataset_generate PRIVATE bench_common)

set_source_files_properties(
  "${LEAP_DIR}/bit_convert.c"
  "${LEAP_DIR}/shift.c"
  "${LEAP_DIR}/popcount.c"
  "${LEAP_DIR}/print.c"
  "${LEAP_DIR}/mask.c"
  PROPERTIES LANGUAGE CXX)

add_library(leap_helpers OBJECT
  "${LEAP_DIR}/bit_convert.c"
  "${LEAP_DIR}/shift.c"
  "${LEAP_DIR}/popcount.c"
  "${LEAP_DIR}/print.c"
  "${LEAP_DIR}/mask.c"
  "${LEAP_DIR}/SHD.cc"
)
target_include_directories(leap_helpers PUBLIC "${LEAP_DIR}")
target_compile_definitions(leap_helpers PRIVATE USE_AVX512)
target_compile_options(leap_helpers PRIVATE
  -mavx2 -mavx512f -mavx512bw -mavx512vl -mbmi -mpopcnt)

add_library(leap_avx2_core OBJECT "${LEAP_DIR}/SIMD_ED.cc")
target_include_directories(leap_avx2_core PUBLIC "${LEAP_DIR}")
target_compile_definitions(leap_avx2_core PRIVATE SIMD_ED=SIMD_ED_AVX2 _MAX_LENGTH_=256)
target_compile_options(leap_avx2_core PRIVATE -mavx2 -mbmi -mpopcnt)

add_library(leap_avx512_core OBJECT "${LEAP_DIR}/SIMD_ED.cc")
target_include_directories(leap_avx512_core PUBLIC "${LEAP_DIR}")
target_compile_definitions(leap_avx512_core PRIVATE
  SIMD_ED=SIMD_ED_AVX512 USE_AVX512 _MAX_LENGTH_=512)
target_compile_options(leap_avx512_core PRIVATE
  -mavx512f -mavx512bw -mavx512vl -mbmi -mpopcnt)

add_library(lv89_obj OBJECT "${LV89_DIR}/lv89.c")
target_include_directories(lv89_obj PUBLIC "${LV89_DIR}")

add_library(miniwfa_obj OBJECT "${MINIWFA_DIR}/miniwfa.c" "${MINIWFA_DIR}/kalloc.c")
target_include_directories(miniwfa_obj PUBLIC "${MINIWFA_DIR}")

set(WFA2_SOURCES
  "${WFA2_DIR}/wavefront/wavefront_align.c"
  "${WFA2_DIR}/wavefront/wavefront_aligner.c"
  "${WFA2_DIR}/wavefront/wavefront_attributes.c"
  "${WFA2_DIR}/wavefront/wavefront_backtrace_buffer.c"
  "${WFA2_DIR}/wavefront/wavefront_backtrace.c"
  "${WFA2_DIR}/wavefront/wavefront_backtrace_offload.c"
  "${WFA2_DIR}/wavefront/wavefront_bialign.c"
  "${WFA2_DIR}/wavefront/wavefront_bialigner.c"
  "${WFA2_DIR}/wavefront/wavefront.c"
  "${WFA2_DIR}/wavefront/wavefront_components.c"
  "${WFA2_DIR}/wavefront/wavefront_compute_affine2p.c"
  "${WFA2_DIR}/wavefront/wavefront_compute_affine.c"
  "${WFA2_DIR}/wavefront/wavefront_compute.c"
  "${WFA2_DIR}/wavefront/wavefront_compute_edit.c"
  "${WFA2_DIR}/wavefront/wavefront_compute_linear.c"
  "${WFA2_DIR}/wavefront/wavefront_debug.c"
  "${WFA2_DIR}/wavefront/wavefront_display.c"
  "${WFA2_DIR}/wavefront/wavefront_extend.c"
  "${WFA2_DIR}/wavefront/wavefront_heuristic.c"
  "${WFA2_DIR}/wavefront/wavefront_pcigar.c"
  "${WFA2_DIR}/wavefront/wavefront_penalties.c"
  "${WFA2_DIR}/wavefront/wavefront_plot.c"
  "${WFA2_DIR}/wavefront/wavefront_sequences.c"
  "${WFA2_DIR}/wavefront/wavefront_slab.c"
  "${WFA2_DIR}/wavefront/wavefront_unialign.c"
  "${WFA2_DIR}/wavefront/wavefront_termination.c"
  "${WFA2_DIR}/wavefront/wavefront_extend_kernels_avx.c"
  "${WFA2_DIR}/wavefront/wavefront_extend_kernels.c"
  "${WFA2_DIR}/system/mm_stack.c"
  "${WFA2_DIR}/system/mm_allocator.c"
  "${WFA2_DIR}/system/profiler_counter.c"
  "${WFA2_DIR}/system/profiler_timer.c"
  "${WFA2_DIR}/utils/bitmap.c"
  "${WFA2_DIR}/utils/dna_text.c"
  "${WFA2_DIR}/utils/sequence_buffer.c"
  "${WFA2_DIR}/utils/vector.c"
  "${WFA2_DIR}/utils/commons.c"
  "${WFA2_DIR}/utils/heatmap.c"
  "${WFA2_DIR}/alignment/affine2p_penalties.c"
  "${WFA2_DIR}/alignment/affine_penalties.c"
  "${WFA2_DIR}/alignment/cigar.c"
  "${WFA2_DIR}/alignment/cigar_utils.c"
  "${WFA2_DIR}/alignment/score_matrix.c"
)

add_library(wfa2_obj OBJECT ${WFA2_SOURCES})
target_include_directories(wfa2_obj PUBLIC "${WFA2_DIR}")
target_compile_definitions(wfa2_obj PRIVATE _FILE_OFFSET_BITS=64)

add_executable(threshold_bench
  src/threshold_bench.cpp
  src/method_leap_avx2.cpp
  src/method_leap_avx512.cpp
  src/method_lv89.cpp
  src/method_miniwfa.cpp
  src/method_wfa2.cpp
  $<TARGET_OBJECTS:leap_helpers>
  $<TARGET_OBJECTS:leap_avx2_core>
  $<TARGET_OBJECTS:leap_avx512_core>
  $<TARGET_OBJECTS:lv89_obj>
  $<TARGET_OBJECTS:miniwfa_obj>
  $<TARGET_OBJECTS:wfa2_obj>
)
target_include_directories(threshold_bench PRIVATE
  include "${LEAP_DIR}" "${LV89_DIR}" "${MINIWFA_DIR}" "${WFA2_DIR}")
target_compile_options(threshold_bench PRIVATE
  -mavx2 -mavx512f -mavx512bw -mavx512vl -mbmi -mpopcnt)
target_link_libraries(threshold_bench PRIVATE bench_common m)

add_executable(leap_phase_bench
  src/leap_phase_bench.cpp
  src/leap_phase_avx2.cpp
  src/leap_phase_avx512.cpp
  $<TARGET_OBJECTS:leap_helpers>
  $<TARGET_OBJECTS:leap_avx2_core>
  $<TARGET_OBJECTS:leap_avx512_core>
)
target_include_directories(leap_phase_bench PRIVATE include "${LEAP_DIR}")
target_compile_options(leap_phase_bench PRIVATE
  -mavx2 -mavx512f -mavx512bw -mavx512vl -mbmi -mpopcnt)
target_link_libraries(leap_phase_bench PRIVATE bench_common)

