#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p tests/.build
CXX="${CXX:-g++}"
COMMON=( -std=c++11 -Wall -Wextra -Werror -Itests/mock -include Arduino.h )
"$CXX" "${COMMON[@]}" -x c++ firmware/UTERUS_Master/UTERUS_Master.ino tests/mock/mock.cpp tests/test_master.cpp -o tests/.build/master_test
tests/.build/master_test
"$CXX" "${COMMON[@]}" -x c++ firmware/UTERUS_Slave/UTERUS_Slave.ino tests/mock/mock.cpp tests/test_slave.cpp -o tests/.build/slave_test
tests/.build/slave_test
# Verify Arduino sketch directories use the exact sketch name.
python3 - <<'PY'
from pathlib import Path
for path in list(Path('firmware').rglob('*.ino')) + list(Path('examples').rglob('*.ino')):
    assert path.stem == path.parent.name, f'Invalid Arduino sketch folder: {path}'
print('Sketch directory checks: PASS')
PY
