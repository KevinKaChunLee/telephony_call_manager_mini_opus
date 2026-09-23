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

# Mini(LiteOS-M) self-test over the POSIX kernel adapter. Sources, include order, macros and warning
# set match the LiteOS harness gate (run_mini_cmsis.sh) and callmanager_mini.gni, so the only
# difference between the two gates is the kernel adapter; test/mini/tools/check_mini_layout.py keeps
# the three in step.
#
# Usage: bash test/mini/mini_selftest.sh [case-filter]
#   CXX  compiler (default g++), OUT  output directory (default ${TMPDIR:-/tmp}/call_manager_mini_posix)

set -euo pipefail

CALL_MANAGER=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
OUT=${OUT:-${TMPDIR:-/tmp}/call_manager_mini_posix}
CXX=${CXX:-g++}

mkdir -p "${OUT}"

# tel_os_adapter_cmsis.cpp is the target adapter and is mutually exclusive with the POSIX one;
# call_manager_service_lite.cpp is the samgr_lite binding, which the self-test replaces by driving
# LosEventHandler directly.
mapfile -t SOURCES < <(
    find "${CALL_MANAGER}/utils/mini/src" -name '*.cpp' ! -name 'tel_os_adapter_cmsis.cpp' | sort
    if [[ -d "${CALL_MANAGER}/services" ]]; then
        find "${CALL_MANAGER}/services" -path '*/src/mini/*.cpp' ! -name 'call_manager_service_lite.cpp' | sort
    fi
    echo "${CALL_MANAGER}/frameworks/native/src/call_manager_client.cpp"
    echo "${CALL_MANAGER}/frameworks/native/src/mini/call_manager_proxy.cpp"
    echo "${CALL_MANAGER}/test/mini/mini_selftest.cpp"
)

INCLUDES=(
    "-I${CALL_MANAGER}/utils/mini/include"
    "-I${CALL_MANAGER}/interfaces/innerkits/mini"
    "-I${CALL_MANAGER}/frameworks/native/include/mini"
    "-I${CALL_MANAGER}/services/audio/include/mini"
    "-I${CALL_MANAGER}/services/call/include/mini"
    "-I${CALL_MANAGER}/services/call_manager_service/include/mini"
    "-I${CALL_MANAGER}/services/call_report/include/mini"
    "-I${CALL_MANAGER}/services/telephony_interaction/include/mini"
    "-I${CALL_MANAGER}/interfaces/innerkits"
    "-I${CALL_MANAGER}/services/call/include"
    "-I${CALL_MANAGER}/test/mini/stub"
)

CXXFLAGS=(
    -std=c++17
    -g
    -O1
    -fno-exceptions
    -fno-rtti
    -fstack-protector-all
    -Wall
    -Wextra
    -Wunused
    -Wunreachable-code
    -Wno-unused-parameter
    -Werror
    -DTELEPHONY_MINI_SYSTEM
    -DTELEPHONY_MINI_LOG_PRINTF
    -DTELEPHONY_MINI_HOST_TEST
)

echo "[mini-posix] call_manager   ${CALL_MANAGER}"
echo "[mini-posix] compiling ${#SOURCES[@]} sources with ${CXX}"
"${CXX}" "${CXXFLAGS[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" -lpthread -o "${OUT}/mini_posix_selftest"

echo "[mini-posix] running"
"${OUT}/mini_posix_selftest" "$@"

# The trust model is compiled in, so its injection path needs builds of its own: the defines below
# have the exact form callmanager_mini.gni emits for call_manager_mini_granted_permissions = [ PLACE_CALL ].
PROBE_SOURCES=(
    "${CALL_MANAGER}/utils/mini/src/call_permission_adapter.cpp"
    "${CALL_MANAGER}/utils/mini/src/telephony_log_mini.cpp"
    "${CALL_MANAGER}/test/mini/probe/permission_macro_probe.cpp"
)
PROBE_GRANT="-DCALL_MANAGER_MINI_GRANTED_PERMISSIONS=\"ohos.permission.PLACE_CALL;\""
echo "[mini-posix] permission macro probe"
"${CXX}" "${CXXFLAGS[@]}" "${INCLUDES[@]}" "${PROBE_GRANT}" -DCALL_MANAGER_MINI_CALLER_BUNDLE_NAME='""' \
    "${PROBE_SOURCES[@]}" -o "${OUT}/permission_probe_default"
"${OUT}/permission_probe_default"
"${CXX}" "${CXXFLAGS[@]}" "${INCLUDES[@]}" "${PROBE_GRANT}" -DCALL_MANAGER_MINI_CALLER_IS_SYSTEM_APP \
    -DCALL_MANAGER_MINI_CALLER_BUNDLE_NAME='"com.example.dialer"' -DPROBE_EXPECT_SYSTEM_APP \
    -DPROBE_EXPECT_BUNDLE='"com.example.dialer"' "${PROBE_SOURCES[@]}" -o "${OUT}/permission_probe_system"
"${OUT}/permission_probe_system"

# The samgr_lite binding is not linked into the self-test; it is compiled against the upstream samgr_lite
# and utils_lite headers and the harness's LiteOS stubs when those checkouts are present.
DEPS=${DEPS:-${CALL_MANAGER}/../.deps}
LITEOS_STUBS=${LITEOS_STUBS:-${CALL_MANAGER}/../communication_mcu-main-test/test/stubs/include/liteos}
if [[ -d "${DEPS}/samgr_lite/interfaces/kits/samgr" && -d "${DEPS}/utils_lite/include" && -d "${LITEOS_STUBS}" ]]; then
    echo "[mini-posix] samgr_lite binding syntax check"
    LITE_FLAGS=()
    for flag in "${CXXFLAGS[@]}"; do
        [[ "${flag}" == -DTELEPHONY_MINI_HOST_TEST ]] || LITE_FLAGS+=("${flag}")
    done
    # Component directories as in the gates, then the LiteOS and samgr headers, the host stub last.
    LAST=$((${#INCLUDES[@]} - 1))
    "${CXX}" "${LITE_FLAGS[@]}" -fsyntax-only "${INCLUDES[@]:0:${LAST}}" "-I${LITEOS_STUBS}" \
        "-I${DEPS}/samgr_lite/interfaces/kits/samgr" "-I${DEPS}/utils_lite/include" "${INCLUDES[${LAST}]}" \
        "${CALL_MANAGER}/services/call_manager_service/src/mini/call_manager_service_lite.cpp"
else
    echo "[mini-posix] SKIP samgr_lite binding syntax check (no samgr_lite/utils_lite checkout under ${DEPS})"
fi
echo "[mini-posix] ok"
