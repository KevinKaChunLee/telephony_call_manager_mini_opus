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

# Checks that a Mini build contains no symbol of a cut module, of the IPC layer or of HiSysEvent,
# HiTrace and dump (specs/mini-feature-scope-build, task 7.4).
# Usage: bash test/mini/tools/check_mini_symbols.sh [binary-or-object ...]
#   Without arguments it checks the CMSIS gate's self-test binary
#   (${OUT:-${TMPDIR:-/tmp}/call_manager_mini_cmsis}/mini_cmsis_selftest).

set -uo pipefail

OUT=${OUT:-${TMPDIR:-/tmp}/call_manager_mini_cmsis}
if [[ $# -eq 0 ]]; then
    set -- "${OUT}/mini_cmsis_selftest"
fi
for input in "$@"; do
    if [[ ! -f "${input}" ]]; then
        echo "ENV  ${input} not found; run the CMSIS gate (run_mini_cmsis.sh) first"
        exit 2
    fi
done

# Classes of the cut modules and layers, matched as "Class::" so that retained API names that merely
# mention them (RegisterBluetoothCallManagerCallbackPtr, GetVoipCallNum, ...) do not count.
FORBIDDEN_CLASSES=(
    CallManagerServiceStub CallManagerServiceProxy CallAbilityCallbackStub CallAbilityCallbackProxy
    CallStatusCallbackStub CallStatusCallbackProxy CellularCallProxy IPCObjectStub IPCObjectProxy
    MessageParcel IRemoteObject SystemAbility
    HiSysEvent CallManagerHisysevent HiTraceChain CallManagerDumpHelper
    VoIPCall VoipCallConnection BluetoothCall BluetoothCallManager BluetoothConnection
    SatelliteCall SatelliteCallControl OTTCall OttCallConnection
    DistributedCallManager DistributedCommunicationManager InteroperableCommunicationManager
    AntiFraudService SpamCallAdapter CallRecordsManager MissedCallNotification
    VideoCallState VideoControlManager ImsRttManager RttCallListener CallSuperPrivacyControlManager
    CallIncomingFilterManager MotionRecogntion AudioProxy AudioDeviceManager
)
FORBIDDEN=('HiSysEventWrite' 'StartAsyncTrace' 'FinishAsyncTrace')
for class in "${FORBIDDEN_CLASSES[@]}"; do
    FORBIDDEN+=("\\b${class}::")
done
PATTERN=$(IFS='|'; echo "${FORBIDDEN[*]}")

HITS=$(nm -C --defined-only "$@" 2>/dev/null | grep -E "${PATTERN}" | sort -u)
TOTAL=$(nm -C --defined-only "$@" 2>/dev/null | wc -l)
if [[ -n "${HITS}" ]]; then
    echo "FAIL symbols of cut modules or layers found:"
    echo "${HITS}"
    exit 1
fi
echo "OK   ${TOTAL} defined symbols in $* contain no cut module, IPC, HiSysEvent, HiTrace or dump symbol"
