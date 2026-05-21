#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="benchmark/build")
    args = parser.parse_args()
    build_dir = pathlib.Path(args.build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        ["cmake", "-S", "benchmark", "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"],
        check=True,
    )
    subprocess.run(["cmake", "--build", str(build_dir), "-j"], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

