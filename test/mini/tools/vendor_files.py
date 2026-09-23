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
"""Checks and refreshes the files vendored verbatim from other repositories.

vendor_manifest.json lists every copied file with its source repository, path and commit.

  vendor_files.py check          every copy must equal its source at the recorded commit, and the
                                 error codes seen through the host stubs must equal the upstream ones;
                                 a copy whose source has changed since (at the checkout's HEAD) is
                                 reported as STALE
  vendor_files.py sync SOURCE    re-copy SOURCE's files from its checkout's HEAD and record that commit

Source checkouts default to the manifest's defaultCheckout (relative to the repository root) and can
be overridden with --checkout NAME=DIR. Exit codes: 0 ok, 1 mismatch, 2 environment problem.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
MANIFEST = os.path.join(REPO_ROOT, "vendor_manifest.json")

CHECKED_CODES = [
    "TELEPHONY_SUCCESS",
    "TELEPHONY_ERR_FAIL",
    "TELEPHONY_ERR_LOCAL_PTR_NULL",
    "TELEPHONY_ERR_PERMISSION_ERR",
    "TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL",
    "TELEPHONY_ERR_REGISTER_CALLBACK_FAIL",
    "TELEPHONY_ERR_UNINIT",
    "TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API",
    "TELEPHONY_ERR_ARGUMENT_INVALID",
    "CALL_ERR_PHONE_NUMBER_EMPTY",
    "CALL_ERR_NUMBER_OUT_OF_RANGE",
    "CALL_ERR_CALL_COUNTS_EXCEED_LIMIT",
    "CALL_ERR_FUNCTION_NOT_SUPPORTED",
    "CALL_ERR_VIDEO_NOT_SUPPORTED",
]


def load_manifest():
    with open(MANIFEST, encoding="utf-8") as f:
        return json.load(f)


def checkout_dirs(manifest, overrides):
    dirs = {name: os.path.normpath(os.path.join(REPO_ROOT, src["defaultCheckout"]))
            for name, src in manifest["sources"].items()}
    for item in overrides:
        name, _, path = item.partition("=")
        if name not in dirs or not path:
            raise SystemExit(f"bad --checkout {item!r}; known sources: {', '.join(dirs)}")
        dirs[name] = os.path.abspath(path)
    return dirs


def git(repo_dir, *args):
    return subprocess.run(["git", "-C", repo_dir, *args], capture_output=True)


def blob_at(repo_dir, commit, path):
    result = git(repo_dir, "show", f"{commit}:{path}")
    return result.stdout if result.returncode == 0 else None


def error_code_values(include_dirs, workdir, tag):
    probe = os.path.join(workdir, f"probe_{tag}.cpp")
    with open(probe, "w", encoding="utf-8") as f:
        f.write('#include <cstdio>\n#include "call_manager_errors.h"\n')
        f.write("using namespace OHOS::Telephony;\nint main()\n{\n")
        for code in CHECKED_CODES:
            f.write(f'    std::printf("{code}=%d\\n", static_cast<int>({code}));\n')
        f.write("    return 0;\n}\n")
    binary = os.path.join(workdir, f"probe_{tag}")
    cmd = [os.environ.get("CXX", "g++"), "-std=c++17", "-o", binary, probe]
    for directory in include_dirs:
        cmd += ["-I", directory]
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        return None, build.stderr
    run = subprocess.run([binary], capture_output=True, text=True)
    if run.returncode != 0:
        return None, run.stderr
    return dict(line.split("=", 1) for line in run.stdout.split()), None


def check(manifest, dirs):
    failed = False
    for name, directory in dirs.items():
        if not os.path.isdir(os.path.join(directory, ".git")):
            print(f"ENV  source checkout for {name} missing: {directory}")
            return 2
    for entry in manifest["files"]:
        source = manifest["sources"][entry["source"]]
        repo_dir = dirs[entry["source"]]
        with open(os.path.join(REPO_ROOT, entry["path"]), "rb") as f:
            copy = f.read()
        pinned = blob_at(repo_dir, source["commit"], entry["sourcePath"])
        if pinned is None:
            print(f"ENV  {entry['source']}: cannot read {entry['sourcePath']} at {source['commit'][:12]}")
            return 2
        if copy != pinned:
            print(f"FAIL {entry['path']} differs from {entry['source']}:{entry['sourcePath']} @ {source['commit'][:12]}")
            failed = True
            continue
        head = blob_at(repo_dir, "HEAD", entry["sourcePath"])
        state = "OK   " if head == pinned else "STALE"
        print(f"{state} {entry['path']} == {entry['source']}:{entry['sourcePath']} @ {source['commit'][:12]}")

    innerkits = os.path.join(REPO_ROOT, "interfaces", "innerkits")
    stub_dirs = [os.path.join(REPO_ROOT, "test", "mini", "stub"),
                 os.path.join(REPO_ROOT, "utils", "mini", "include"), innerkits]
    upstream_dirs = [os.path.join(dirs["core_service"], "interfaces", "innerkits", "include"),
                     os.path.join(dirs["c_utils"], "base", "include"), innerkits]
    with tempfile.TemporaryDirectory() as workdir:
        stub_values, error = error_code_values(stub_dirs, workdir, "stub")
        if stub_values is None:
            print(f"ENV  cannot build the error code probe against the stubs:\n{error}")
            return 2
        upstream_values, error = error_code_values(upstream_dirs, workdir, "upstream")
        if upstream_values is None:
            print(f"ENV  cannot build the error code probe against the upstream headers:\n{error}")
            return 2
    for code in CHECKED_CODES:
        same = stub_values.get(code) == upstream_values.get(code)
        failed = failed or not same
        print(f"{'OK   ' if same else 'FAIL '} {code:40s} stub={stub_values.get(code)} upstream={upstream_values.get(code)}")
    return 1 if failed else 0


def sync(manifest, dirs, source_name):
    if source_name not in manifest["sources"]:
        print(f"unknown source {source_name}; known: {', '.join(manifest['sources'])}")
        return 2
    repo_dir = dirs[source_name]
    head = git(repo_dir, "rev-parse", "HEAD")
    if head.returncode != 0:
        print(f"ENV  {repo_dir} is not a git checkout")
        return 2
    commit = head.stdout.decode().strip()
    for entry in manifest["files"]:
        if entry["source"] != source_name:
            continue
        blob = blob_at(repo_dir, commit, entry["sourcePath"])
        if blob is None:
            print(f"FAIL {entry['sourcePath']} not found at {commit[:12]}")
            return 1
        target = os.path.join(REPO_ROOT, entry["path"])
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "wb") as f:
            f.write(blob)
        print(f"sync {entry['path']} <- {source_name}:{entry['sourcePath']} @ {commit[:12]}")
    manifest["sources"][source_name]["commit"] = commit
    with open(MANIFEST, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
        f.write("\n")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--checkout", action="append", default=[], metavar="NAME=DIR",
                        help="use DIR as the checkout of source NAME")
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("check")
    sync_parser = sub.add_parser("sync")
    sync_parser.add_argument("source")
    args = parser.parse_args()
    manifest = load_manifest()
    dirs = checkout_dirs(manifest, args.checkout)
    if args.command == "sync":
        return sync(manifest, dirs, args.source)
    return check(manifest, dirs)


if __name__ == "__main__":
    sys.exit(main())
