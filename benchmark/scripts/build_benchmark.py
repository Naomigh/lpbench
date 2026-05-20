#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser(description="Build lpbench threshold benchmark binaries.")
    parser.add_argument("--source", default="benchmark")
    parser.add_argument("--build-dir", default="benchmark/build")
    parser.add_argument("--config", default="Release")
    args = parser.parse_args()

    pathlib.Path(args.build_dir).mkdir(parents=True, exist_ok=True)
    subprocess.run(
        ["cmake", "-S", args.source, "-B", args.build_dir, f"-DCMAKE_BUILD_TYPE={args.config}"],
        check=True,
    )
    subprocess.run(["cmake", "--build", args.build_dir, "--target", "threshold_verify_bench", "-j"], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
