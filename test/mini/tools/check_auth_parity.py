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
"""Compares the authorization decision points of the Mini CallManagerService with the standard one.

For every entry of the design D6 table, both call_manager_service.cpp files must agree on whether the
entry checks the caller is a system application and on the permission constants it tests, in order.
CheckSetTelephonyStatePermission() counts as OHOS_PERMISSION_SET_TELEPHONY_STATE.

Exit codes: 0 ok, 1 mismatch, 2 environment problem.
"""

import argparse
import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
MINI_FILE = os.path.join(REPO_ROOT, "services", "call_manager_service", "src", "mini", "call_manager_service.cpp")
STANDARD_FILE = os.path.join("services", "call_manager_service", "src", "call_manager_service.cpp")
DEFAULT_STANDARD_ROOT = os.path.join(os.path.dirname(REPO_ROOT), "telephony_call_manager")

# (name, has parameters) of the D6 entries.
ENTRIES = [
    ("RegisterCallBack", True),
    ("UnRegisterCallBack", False),
    ("ObserverOnCallDetailsChange", False),
    ("DialCall", True),
    ("AnswerCall", True),
    ("AnswerCall", False),
    ("RejectCall", True),
    ("RejectCall", False),
    ("HangUpCall", True),
    ("HangUpCall", False),
    ("HoldCall", True),
    ("UnHoldCall", True),
    ("SwitchCall", True),
    ("EndCall", False),
    ("IsRinging", True),
    ("IsNewCallAllowed", True),
    ("GetCallState", False),
    ("HasCall", True),
    ("IsEmergencyPhoneNumber", True),
]


def strip_comments_and_strings(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(\\.|[^"\\])*"', '""', text)


def function_bodies(path):
    """Maps (name, has parameters) to the body of CallManagerService::name."""
    code = strip_comments_and_strings(open(path, encoding="utf-8").read())
    bodies = {}
    for m in re.finditer(r"^[\w:<>]+\s+CallManagerService::(\w+)\(([^)]*)\)\s*\{", code, re.M):
        depth, j = 1, m.end()
        while j < len(code) and depth:
            depth += {"{": 1, "}": -1}.get(code[j], 0)
            j += 1
        key = (m.group(1), bool(m.group(2).strip()))
        bodies.setdefault(key, code[m.end():j - 1])
    return bodies


def decision_points(body):
    # Only permissions that are checked count: the standard DialCall also passes the constant to a
    # HiSysEvent fault record.
    system_app = "CheckCallerIsSystemApp" in body
    permissions = []
    for m in re.finditer(r"\bCheckPermission\s*\(\s*(OHOS_PERMISSION_\w+)|\bCheckSetTelephonyStatePermission\s*\(",
                         body):
        permissions.append(m.group(1) or "OHOS_PERMISSION_SET_TELEPHONY_STATE")
    return system_app, permissions


def describe(entry):
    return f"{entry[0]}({'...' if entry[1] else ''})"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--standard-root", default=DEFAULT_STANDARD_ROOT,
                        help="checkout of telephony_call_manager (default ../telephony_call_manager)")
    args = parser.parse_args()
    standard_path = os.path.join(args.standard_root, STANDARD_FILE)
    for path in (standard_path, MINI_FILE):
        if not os.path.exists(path):
            print(f"ENV  {path} not found")
            return 2
    standard = function_bodies(standard_path)
    mini = function_bodies(MINI_FILE)
    failed = False
    for entry in ENTRIES:
        if entry not in standard:
            print(f"ENV  standard entry {describe(entry)} not found")
            return 2
        if entry not in mini:
            print(f"FAIL Mini entry {describe(entry)} not found")
            failed = True
            continue
        expected = decision_points(standard[entry])
        actual = decision_points(mini[entry])
        if expected != actual:
            print(f"FAIL {describe(entry)}: standard system-app={expected[0]} permissions={expected[1]}")
            print(f"     {' ' * len(describe(entry))}  Mini     system-app={actual[0]} permissions={actual[1]}")
            failed = True
        else:
            print(f"OK   {describe(entry):32} system-app={expected[0]!s:5} permissions={expected[1]}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
