#!/usr/bin/env bash
set -euo pipefail

app=$(python3 - <<'PY'
import json
from pathlib import Path

plans = list(Path("examples/preview/.huxerui/build").glob("*/ios-simulator/*/huxerui-integration/app.json"))
if len(plans) != 1:
    raise SystemExit(f"Expected one iOS Simulator build plan, got {plans}")
print(json.loads(plans[0].read_text())["bundle"])
PY
)
test -f "$app/HuxerUI/huxerui/resources.bin"

simulator=$(xcrun simctl list devices available --json | python3 -c '
import json
import sys

for runtime, devices in reversed(list(json.load(sys.stdin)["devices"].items())):
    if ".iOS-" not in runtime:
        continue
    for device in devices:
        if device["name"].startswith("iPhone"):
            print(device["udid"])
            sys.exit(0)
sys.exit("No available iPhone Simulator")
')
xcrun simctl boot "$simulator"
trap 'xcrun simctl shutdown "$simulator" || true' EXIT
xcrun simctl bootstatus "$simulator" -b
xcrun simctl install "$simulator" "$app"
container=$(xcrun simctl get_app_container "$simulator" "$PREVIEW_ID" data)
result_file="$container/Library/Caches/qrcode-reference-result.txt"
rm -f "$result_file"
xcrun simctl launch "$simulator" "$PREVIEW_ID"
for attempt in {1..30}; do
    if [[ -f "$result_file" && "$(cat "$result_file")" == "$REFERENCE_RESULT" ]]; then
        cat "$result_file"
        echo "QRCODE_APP=$app" >> "$GITHUB_ENV"
        exit 0
    fi
    sleep 1
done

xcrun simctl spawn "$simulator" log show --last 1m --predicate 'process == "Lib-QRCode Preview"' >&2 || true
echo 'The preview did not report the expected reference image content.' >&2
exit 1
