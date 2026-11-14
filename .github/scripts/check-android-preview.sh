#!/usr/bin/env bash
set -euo pipefail

adb install -r "$QRCODE_APK"
# Clear a previous run's marker before launching the app.
adb shell am force-stop "$PREVIEW_ID"
adb shell pm clear "$PREVIEW_ID"
adb shell am start -S -W -n "$PREVIEW_ID/.MainActivity"
for attempt in {1..30}; do
    result=$(adb shell run-as "$PREVIEW_ID" cat cache/qrcode-reference-result.txt 2>/dev/null || true)
    if [[ "$result" == "$REFERENCE_RESULT" ]] && adb shell pidof "$PREVIEW_ID"; then
        echo "$result"
        exit 0
    fi
    sleep 1
done

printf 'Expected "%s", got "%s"\n' "$REFERENCE_RESULT" "$result" >&2
adb logcat -d -t 250 -s AndroidRuntime HuxerUI >&2
exit 1
