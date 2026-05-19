.PHONY: threshold_verify_bench

threshold_verify_bench:
	cmake -S benchmark -B benchmark/build -DCMAKE_BUILD_TYPE=Release
	cmake --build benchmark/build --target threshold_verify_bench -j
