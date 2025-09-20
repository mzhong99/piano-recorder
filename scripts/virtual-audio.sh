#!/usr/bin/env bash
#
# Usage:
#    ./piano-recorder-test.sh setup
#    ./piano-recorder-test.sh loop-audio path/to/file.wav
#    ./piano-recorder-test.sh loop-midi path/to/file.mid
#    ./piano-recorder-test.sh loop-both path/to/file.wav path/to/file.mid
#

set -e

setup_devices() {
    echo "[*] Loading ALSA loopback (audio) and virmidi (MIDI) modules..."
    sudo modprobe snd-aloop
    sudo modprobe snd-virmidi
    echo "[*] Devices created:"
    aplay -l
    aconnect -l
}

loop_audio() {
    local wav="$1"
    if [ -z "$wav" ]; then
        echo "Usage: $0 loop-audio file.wav"
        exit 1
    fi
    echo "[*] Looping audio file '$wav' into ALSA Loopback..."
    while true; do
        aplay -D hw:Loopback,0,0 "$wav"
    done
}

loop_midi() {
    local mid="$1"
    if [ -z "$mid" ]; then
        echo "Usage: $0 loop-midi file.mid"
        exit 1
    fi
    echo "[*] Looping MIDI file '$mid' into Virtual MIDI device..."
    local port
    port=$(aconnect -l | grep -m1 "Virtual Raw MIDI" | awk -F'[][]' '{print $2}'):0
    if [ -z "$port" ]; then
        echo "Could not find virmidi port. Did you run '$0 setup'?"
        exit 1
    fi
    while true; do
        aplaymidi -p "$port" "$mid"
    done
}

loop_both() {
    local wav="$1"
    local mid="$2"
    if [ -z "$wav" ] || [ -z "$mid" ]; then
        echo "Usage: $0 loop-both file.wav file.mid"
        exit 1
    fi

    echo "[*] Starting both audio ('$wav') and MIDI ('$mid') loops..."
    loop_audio "$wav" &
    pid_audio=$!
    loop_midi "$mid" &
    pid_midi=$!

    echo "[*] Audio loop PID: $pid_audio"
    echo "[*] MIDI loop PID:  $pid_midi"
    echo "[*] Press Ctrl-C to stop both."

    # Forward SIGINT/SIGTERM to children
    trap "kill $pid_audio $pid_midi 2>/dev/null" INT TERM
    wait
}

main() {
    local cmd="$1"
    shift || true

    case "$cmd" in
        setup)       setup_devices "$@" ;;
        loop-audio)  loop_audio "$@" ;;
        loop-midi)   loop_midi "$@" ;;
        loop-both)   loop_both "$@" ;;
        *)
            echo "Usage: $0 {setup|loop-audio file.wav|loop-midi file.mid|loop-both file.wav file.mid}"
            exit 1
            ;;
    esac
}

main "$@"

