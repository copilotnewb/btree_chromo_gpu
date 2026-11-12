#!/usr/bin/env python3
"""Build CPU-only DNA loop regression tests; no LAMMPS or GPU is needed."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "src/binding_region.cpp",
    "src/loop_topology.cpp",
    "src/loop.cpp",
    "src/intra_step_distribution.cpp",
    "src/vec_quat_manipulator.cpp",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", action="store_true")
    parser.add_argument("--skip-legacy", action="store_true",
                        help="Skip the git-based trajectory comparison in source archives")
    parser.add_argument("--baseline-ref", default="fbc6f01b10a2c169e439c541f71e5b2a413fd0c9")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    flags = shlex.split(os.environ.get("TEST_CXXFLAGS", "-O2 -g"))
    with tempfile.TemporaryDirectory(prefix="btree-cpu-tests-") as work:
        def build(name, test, sources=SOURCES, link_flags=()):
            binary = str(Path(work) / name)
            subprocess.run(
                compiler + ["-std=c++17", "-Wall", "-Wextra"] + flags
                + ["-Iinclude", test] + list(sources) + list(link_flags)
                + ["-o", binary], cwd=ROOT, check=True,
            )
            return binary

        bounded_scan = build("test_loop_scan", "tests/test_loop_scan.cpp",
                            link_flags=["-Wl,--wrap=_ZN20vec_quat_manipulator4v_L2E3vec"])
        subprocess.run([bounded_scan], cwd=ROOT, check=True)
        candidates = build("test_intra_proximities", "tests/test_intra_proximities.cpp")
        subprocess.run([candidates], cwd=ROOT, check=True)
        if not args.skip_legacy:
            baseline_sources = []
            for source in SOURCES:
                baseline_file = Path(work) / ("baseline_" + Path(source).name)
                baseline_file.write_bytes(subprocess.check_output(
                    ["git", "show", f"{args.baseline_ref}:{source}"], cwd=ROOT))
                baseline_sources.append(str(baseline_file))
            baseline = build("legacy_trajectory", "tests/test_loop_trajectory.cpp",
                             sources=baseline_sources)
            current = build("current_trajectory", "tests/test_loop_trajectory.cpp")
            expected = subprocess.check_output([baseline], cwd=ROOT)
            actual = subprocess.check_output([current], cwd=ROOT)
            if actual != expected:
                raise AssertionError("Loop trajectories differ from the baseline")
            print(f"PASS: {len(actual.splitlines())} loop states match baseline exactly", flush=True)
        if args.benchmark:
            benchmark = build("benchmark", "tests/benchmark_loop_candidates.cpp")
            subprocess.run([benchmark], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
