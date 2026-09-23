#!/usr/bin/env python3
# Copyright (C) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Checks where every header of the POSIX self-test build is taken from (g++ -H).

  - a header that has a Mini version must resolve to it in every translation unit;
  - every header must come from this repository or the compiler's system directories, never from
    another checkout such as telephony_call_manager or c_utils.

--prepend-include DIR puts DIR in front of the include path, which is how the check itself is
tested. Exit codes: 0 ok, 1 foreign or shadowed header, 2 environment problem.
"""

import argparse
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
import check_mini_layout  # noqa: E402

REPO_ROOT = check_mini_layout.REPO_ROOT
SELFTEST_DEFINES = ["-DTELEPHONY_MINI_SYSTEM", "-DTELEPHONY_MINI_LOG_PRINTF", "-DTELEPHONY_MINI_HOST_TEST"]
HOST_EXCLUDED = {"tel_os_adapter_cmsis.cpp", "call_manager_service_lite.cpp"}


def selftest_sources():
    sources = sorted(glob.glob(os.path.join(REPO_ROOT, "utils/mini/src/*.cpp")))
    sources += sorted(glob.glob(os.path.join(REPO_ROOT, "services/*/src/mini/*.cpp")))
    sources = [s for s in sources if os.path.basename(s) not in HOST_EXCLUDED]
    sources += [os.path.join(REPO_ROOT, rel) for rel in (
        "frameworks/native/src/call_manager_client.cpp",
        "frameworks/native/src/mini/call_manager_proxy.cpp",
        "test/mini/mini_selftest.cpp")]
    return sources


def mini_headers(include_dirs):
    """basename -> path of every header living in a Mini include directory."""
    headers = {}
    for rel in include_dirs:
        if "/mini" not in rel and not rel.startswith("utils/mini"):
            continue
        for path in glob.glob(os.path.join(REPO_ROOT, rel, "*.h")):
            headers.setdefault(os.path.basename(path), os.path.realpath(path))
    return headers


def included_headers(source, include_flags):
    cmd = [os.environ.get("CXX", "g++"), "-std=c++17", "-fsyntax-only", "-H"] + SELFTEST_DEFINES
    cmd += include_flags + [source]
    result = subprocess.run(cmd, capture_output=True, text=True)
    headers = []
    for line in result.stderr.splitlines():
        stripped = line.lstrip(".")
        if stripped != line and stripped.startswith(" "):
            headers.append(os.path.realpath(stripped.strip()))
    # -H still lists what was included before a compile error, and a wrong header is the usual cause.
    return headers, (None if result.returncode == 0 else result.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prepend-include", action="append", default=[], metavar="DIR")
    args = parser.parse_args()

    include_dirs = check_mini_layout.selftest_include_dirs()
    if not include_dirs:
        print("ENV  no include directories found in test/mini/mini_selftest.sh")
        return 2
    include_flags = [f"-I{os.path.abspath(d)}" for d in args.prepend_include]
    include_flags += [f"-I{os.path.join(REPO_ROOT, rel)}" for rel in include_dirs]
    minis = mini_headers(include_dirs)
    repo_prefix = os.path.realpath(REPO_ROOT) + os.sep

    problems = set()
    build_errors = []
    checked = 0
    for source in selftest_sources():
        headers, error = included_headers(source, include_flags)
        if error is not None:
            build_errors.append((os.path.relpath(source, REPO_ROOT), error))
        checked += len(headers)
        for header in headers:
            name = os.path.basename(header)
            if name in minis and header != minis[name]:
                problems.add(f"FAIL {name}: resolved to {header} instead of the Mini version {minis[name]}")
            elif not header.startswith(repo_prefix) and not header.startswith("/usr/"):
                problems.add(f"FAIL foreign header {header}")
    for problem in sorted(problems):
        print(problem)
    if problems:
        return 1
    if build_errors:
        source, error = build_errors[0]
        print(f"ENV  {len(build_errors)} translation unit(s) do not compile, first {source}:\n{error}")
        return 2
    print(f"OK   {checked} header inclusions over {len(selftest_sources())} translation units resolve "
          f"inside the repository; {len(minis)} Mini headers always win")
    return 0


if __name__ == "__main__":
    sys.exit(main())
