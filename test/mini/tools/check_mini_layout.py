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
"""Keeps callmanager_mini.gni, test/mini/mini_selftest.sh and the harness gate in step.

Checks that
  - the component include directories appear in the same order in the gni, the POSIX self-test
    script and the LiteOS harness's run_mini_cmsis.sh (test/mini/stub only in the two scripts, last);
  - call_manager_mini_sources lists exactly the Mini sources in the tree (all of utils/mini/src,
    services/*/src/mini and frameworks/native/src/mini except the POSIX kernel adapter, plus the
    vendored call_manager_client.cpp);
  - every declare_args() build argument is registered in bundle.json's features, once bundle.json exists.

Exit codes: 0 ok, 1 mismatch, 2 environment problem.
"""

import argparse
import glob
import json
import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
GNI = os.path.join(REPO_ROOT, "callmanager_mini.gni")
SELFTEST_SCRIPT = os.path.join(REPO_ROOT, "test", "mini", "mini_selftest.sh")
DEFAULT_HARNESS = os.path.join(os.path.dirname(REPO_ROOT), "communication_mcu-main-test", "test",
                               "run_mini_cmsis.sh")
GNI_PREFIX = "${call_manager_mini_path}/"
TEST_ONLY_INCLUDE = "test/mini/stub"
HOST_ONLY_SOURCES = {"utils/mini/src/tel_os_adapter_posix.cpp"}
VENDORED_SOURCES = {"frameworks/native/src/call_manager_client.cpp"}


def gni_list(text, name):
    match = re.search(r"^" + re.escape(name) + r"\s*=\s*\[(.*?)\]", text, re.S | re.M)
    if match is None:
        return None
    return re.findall(r'"([^"]+)"', match.group(1))


def gni_args(text):
    match = re.search(r"declare_args\(\)\s*\{(.*?)\n\}", text, re.S)
    if match is None:
        return []
    return re.findall(r"^\s*(\w+)\s*=", match.group(1), re.M)


def component_relative(entries, prefix):
    return [entry[len(prefix):] for entry in entries if entry.startswith(prefix)]


def script_includes(path, root_var):
    """Component-relative include directories of a gate script, in order."""
    with open(path, encoding="utf-8") as f:
        text = f.read()
    return re.findall(r'"-I\$\{' + root_var + r'\}/([^"]+)"', text)


def gni_include_dirs():
    with open(GNI, encoding="utf-8") as f:
        text = f.read()
    return component_relative(gni_list(text, "call_manager_mini_include_dirs") or [], GNI_PREFIX)


def selftest_include_dirs():
    return script_includes(SELFTEST_SCRIPT, "CALL_MANAGER")


def tree_sources():
    patterns = ["utils/mini/src/*.cpp", "services/*/src/mini/*.cpp", "frameworks/native/src/mini/*.cpp"]
    found = set()
    for pattern in patterns:
        for path in glob.glob(os.path.join(REPO_ROOT, pattern)):
            found.add(os.path.relpath(path, REPO_ROOT))
    return found | VENDORED_SOURCES


def check_includes(harness_path):
    failed = False
    gni_dirs = gni_include_dirs()
    if TEST_ONLY_INCLUDE in gni_dirs:
        print(f"FAIL gni lists the host-only include {TEST_ONLY_INCLUDE}")
        failed = True
    for name, dirs in (("mini_selftest.sh", selftest_include_dirs()),
                       ("run_mini_cmsis.sh", script_includes(harness_path, "CALL_MANAGER"))):
        if not dirs or dirs[-1] != TEST_ONLY_INCLUDE:
            print(f"FAIL {name}: {TEST_ONLY_INCLUDE} must be the last component include")
            failed = True
        component_dirs = [d for d in dirs if d != TEST_ONLY_INCLUDE]
        if component_dirs != gni_dirs:
            print(f"FAIL {name} include order differs from callmanager_mini.gni")
            print(f"     gni:    {gni_dirs}")
            print(f"     script: {component_dirs}")
            failed = True
        else:
            print(f"OK   {name} include order == callmanager_mini.gni ({len(gni_dirs)} dirs)")
    return failed


def check_sources():
    with open(GNI, encoding="utf-8") as f:
        listed = set(component_relative(gni_list(f.read(), "call_manager_mini_sources") or [], GNI_PREFIX))
    expected = tree_sources() - HOST_ONLY_SOURCES
    failed = False
    for missing in sorted(expected - listed):
        print(f"FAIL {missing} exists but is not in call_manager_mini_sources")
        failed = True
    for extra in sorted(listed - expected):
        print(f"FAIL call_manager_mini_sources lists {extra}, which is not a Mini source in the tree")
        failed = True
    if not failed:
        print(f"OK   call_manager_mini_sources == Mini sources in the tree ({len(listed)} files)")
    return failed


def check_bundle():
    bundle_path = os.path.join(REPO_ROOT, "bundle.json")
    if not os.path.exists(bundle_path):
        print("SKIP bundle.json not created yet")
        return False
    with open(GNI, encoding="utf-8") as f:
        args = gni_args(f.read())
    with open(bundle_path, encoding="utf-8") as f:
        features = set(json.load(f).get("component", {}).get("features", []))
    missing = [arg for arg in args if arg not in features]
    for arg in missing:
        print(f"FAIL build argument {arg} is not registered in bundle.json features")
    if not missing:
        print(f"OK   all {len(args)} build arguments are registered in bundle.json features")
    return bool(missing)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--harness", default=DEFAULT_HARNESS, help="path of the harness run_mini_cmsis.sh")
    args = parser.parse_args()
    if not os.path.exists(args.harness):
        print(f"ENV  harness gate not found: {args.harness}")
        return 2
    failed = check_includes(args.harness)
    failed = check_sources() or failed
    failed = check_bundle() or failed
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
