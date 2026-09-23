#!/bin/bash
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

# Static rules of the Mini call manager (tasks 7.2 and 7.5):
#  1. the standard telephony_call_manager checkout is unmodified;
#  2. vendored files match their sources byte for byte and carry no Mini macro;
#  3. Mini code uses no exceptions, RTTI casts or std threading (the POSIX kernel adapter excepted);
#  4. Mini log calls pass no phone-number field as an argument;
#  5. the security options are in effect in callmanager_mini.gni and both gate scripts.
# Usage: bash test/mini/tools/check_mini_rules.sh
#   STANDARD_ROOT  standard checkout (default ../telephony_call_manager)
#   HARNESS_GATE   harness gate script (default ../communication_mcu-main-test/test/run_mini_cmsis.sh)

set -uo pipefail

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
STANDARD_ROOT=${STANDARD_ROOT:-${REPO}/../telephony_call_manager}
HARNESS_GATE=${HARNESS_GATE:-${REPO}/../communication_mcu-main-test/test/run_mini_cmsis.sh}
FAILED=0

fail() {
    echo "FAIL $*"
    FAILED=1
}

# 1. Standard checkout untouched.
if [[ ! -d "${STANDARD_ROOT}/.git" ]]; then
    fail "standard checkout not found at ${STANDARD_ROOT}"
elif [[ -n "$(git -C "${STANDARD_ROOT}" status --porcelain)" ]]; then
    fail "telephony_call_manager has local changes:"
    git -C "${STANDARD_ROOT}" status --short
else
    echo "OK   telephony_call_manager is unmodified"
fi

# 2. Vendored files: byte identity (vendor_files.py) and no Mini macro inside.
if python3 "${REPO}/test/mini/tools/vendor_files.py" check > /tmp/check_mini_rules_vendor.log 2>&1; then
    echo "OK   vendored files match their sources"
else
    fail "vendor_files.py check failed:"
    grep -v '^OK' /tmp/check_mini_rules_vendor.log
fi
mapfile -t VENDORED < <(python3 -c 'import json,sys; [print(f["path"]) for f in json.load(open(sys.argv[1]))["files"]]' \
    "${REPO}/vendor_manifest.json")
MACRO_HITS=$(cd "${REPO}" && grep -ln 'TELEPHONY_MINI_' "${VENDORED[@]}" 2>/dev/null)
if [[ -n "${MACRO_HITS}" ]]; then
    fail "vendored files mention TELEPHONY_MINI_: ${MACRO_HITS}"
else
    echo "OK   no vendored file mentions TELEPHONY_MINI_ (${#VENDORED[@]} files)"
fi

# 3 and 4 need the code without comments and string literals.
python3 - "${REPO}" "${VENDORED[@]}" <<'EOF' || FAILED=1
import os
import re
import sys

repo = sys.argv[1]
vendored = set(sys.argv[2:])
host_only = {"utils/mini/src/tel_os_adapter_posix.cpp"}
forbidden = [
    (re.compile(r"\btry\s*\{"), "try"),
    (re.compile(r"\bcatch\s*\("), "catch"),
    (re.compile(r"\bdynamic_cast\s*<"), "dynamic_cast"),
    (re.compile(r"\bstd::(recursive_|timed_|shared_)?mutex\b"), "std::mutex"),
    (re.compile(r"\bstd::(thread|jthread)\b"), "std::thread"),
    (re.compile(r"\bstd::condition_variable(_any)?\b"), "std::condition_variable"),
]
# A number field passed to a log call, unless only its length or emptiness is logged.
number_field = re.compile(r"\b(phoneNum|accountNum|accountNumber|phoneNumber|number|GetAccountNumber|"
                          r"newPhoneNum|accountNumber_|phoneString)\b(?!\s*(\.|->)\s*(length|size|empty)\s*\()")


def strip(text, keep_strings):
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            chunk = text[i:(n if j < 0 else j + 2)]
            out.append("\n" * chunk.count("\n"))
            i = n if j < 0 else j + 2
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1] if keep_strings else c + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def log_calls(code):
    """Yields (offset, arguments after the format string) of each log call; code has no strings."""
    for m in re.finditer(r"\bTELEPHONY_LOG[DIWEF]\s*\(", code):
        depth, j, first_comma = 1, m.end(), -1
        while j < len(code) and depth:
            c = code[j]
            depth += {"(": 1, ")": -1}.get(c, 0)
            if c == "," and depth == 1 and first_comma < 0:
                first_comma = j
            j += 1
        yield m.start(), ("" if first_comma < 0 else code[first_comma + 1:j - 1])


failed = False
files = []
for top in ("utils", "services", "frameworks", "interfaces", "test"):
    for base, _, names in os.walk(os.path.join(repo, top)):
        for name in names:
            if name.endswith((".h", ".cpp", ".inc")):
                rel = os.path.relpath(os.path.join(base, name), repo)
                if rel not in vendored and "/stub/" not in rel:
                    files.append(rel)
for rel in sorted(files):
    raw = open(os.path.join(repo, rel), encoding="utf-8").read()
    code = strip(raw, keep_strings=False)
    if rel not in host_only:
        for pattern, label in forbidden:
            for m in pattern.finditer(code):
                line = code.count("\n", 0, m.start()) + 1
                print(f"FAIL {rel}:{line}: {label} is not allowed in Mini code")
                failed = True
    for start, arguments in log_calls(code):
        if number_field.search(arguments):
            line = code.count("\n", 0, start) + 1
            print(f"FAIL {rel}:{line}: log call passes a phone-number field: {' '.join(arguments.split())}")
            failed = True
if not failed:
    print(f"OK   {len(files)} Mini files: no exceptions, RTTI casts, std threading or logged numbers")
sys.exit(1 if failed else 0)
EOF

# 5. Security options in the three build descriptions.
SECURITY_FLAGS=(-fno-exceptions -fno-rtti -fstack-protector-all -Wall -Wextra -Werror)
for file in "${REPO}/callmanager_mini.gni" "${REPO}/test/mini/mini_selftest.sh" "${HARNESS_GATE}"; do
    if [[ ! -f "${file}" ]]; then
        fail "build description not found: ${file}"
        continue
    fi
    missing=()
    for flag in "${SECURITY_FLAGS[@]}"; do
        grep -qE "(^|[\"[:space:]])${flag}([\"[:space:],]|$)" "${file}" || missing+=("${flag}")
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        fail "$(basename "${file}") lacks ${missing[*]}"
    else
        echo "OK   $(basename "${file}") sets ${SECURITY_FLAGS[*]}"
    fi
done

exit "${FAILED}"
