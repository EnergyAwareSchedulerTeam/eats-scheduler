#!/bin/bash
# ═══════════════════════════════════════════════════════════
# EATS — Heavy Load Test
# Runs a CPU-intensive task to demonstrate BIG core assignment
# Usage: ./heavy_test.sh [duration_seconds]
# ═══════════════════════════════════════════════════════════

DURATION=${1:-60}

echo "=================================================="
echo "  EATS Heavy Load Test"
echo "  This will demonstrate BIG core assignment"
echo "  Duration: ${DURATION} seconds"
echo "=================================================="

# Check module is loaded
if ! lsmod | grep -q eats; then
    echo "✗ EATS module not loaded. Run ./run_eats.sh first."
    exit 1
fi

echo ""
echo "Starting CPU stress test..."
stress-ng --cpu 2 --timeout ${DURATION}s &
STRESS_PID=$!
echo "stress-ng PID: $STRESS_PID"

echo ""
echo "Watching for BIG core assignments..."
for i in {1..15}; do
    sleep 2
    BIG=$(sudo dmesg | grep "stress-ng" | grep "BIG" | tail -3)
    if [ ! -z "$BIG" ]; then
        echo ""
        echo "✓ BIG core assignments detected:"
        echo "$BIG"
        break
    else
        echo "  Waiting... (tick $i)"
    fi
done

wait $STRESS_PID
echo ""
echo "Test complete. Generating updated graph..."
python3 /home/kali/eats/collect_data.py
python3 /home/kali/eats/plot_results.py
xdg-open /home/kali/eats/eats_results.png 2>/dev/null &
