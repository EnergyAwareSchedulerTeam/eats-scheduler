#!/bin/bash
# ═══════════════════════════════════════════════════════════
# EATS — Energy-Aware Task Scheduler
# Run Script — use this after every reboot
# Usage: ./run_eats.sh [duration_seconds]
# Example: ./run_eats.sh 120   (run for 2 minutes)
#          ./run_eats.sh 300   (run for 5 minutes)
#          ./run_eats.sh       (default: 3 minutes)
# ═══════════════════════════════════════════════════════════

EATS_DIR="/home/kali/eats"
DURATION=${1:-180}   # default 3 minutes if no argument given

echo "=================================================="
echo "  EATS Scheduler — Starting Up"
echo "  Run duration: ${DURATION} seconds"
echo "=================================================="

# ── Step 1: Load kernel module ──────────────────────────────
echo ""
echo "[1/6] Loading EATS kernel module..."
sudo rmmod eats 2>/dev/null
sudo insmod $EATS_DIR/eats.ko
if lsmod | grep -q eats; then
    echo "      ✓ Kernel module loaded successfully"
else
    echo "      ✗ Failed to load module. Exiting."
    exit 1
fi

# ── Step 2: Verify /proc/eats_hints exists ──────────────────
if [ ! -f /proc/eats_hints ]; then
    echo "      ✗ /proc/eats_hints not found. Module error."
    exit 1
fi
echo "      ✓ /proc/eats_hints ready"

# ── Step 3: Start FNN daemon ────────────────────────────────
echo ""
echo "[2/6] Starting FNN prediction daemon..."
pkill -f fnn_daemon.py 2>/dev/null
sleep 1
sudo bash -c "python3 $EATS_DIR/fnn_daemon.py > /tmp/fnn.log 2>&1 &"
sleep 3
if pgrep -f fnn_daemon.py > /dev/null; then
    echo "      ✓ FNN daemon running"
else
    echo "      ✗ FNN daemon failed. Check /tmp/fnn.log"
    cat /tmp/fnn.log
    exit 1
fi

# ── Step 4: Clear old logs ──────────────────────────────────
echo ""
echo "[3/6] Clearing old kernel logs..."
sudo dmesg -C
echo "      ✓ Logs cleared"

# ── Step 5: Run for specified duration ──────────────────────
echo ""
echo "[4/6] System running — collecting data for ${DURATION} seconds..."
echo "      (You can run stress-ng in another terminal to test BIG core assignment)"
echo "      Watching for decisions..."
echo ""

# Show live decisions while waiting
END=$((SECONDS + DURATION))
LAST_COUNT=0
while [ $SECONDS -lt $END ]; do
    sleep 5
    COUNT=$(sudo dmesg | grep "EATS" | wc -l)
    FNN=$(sudo dmesg | grep "\[FNN\]" | wc -l)
    WPBA=$(sudo dmesg | grep "\[WPBA\]" | wc -l)
    LITTLE=$(sudo dmesg | grep "LITTLE" | wc -l)
    BIG=$(sudo dmesg | grep "-> BIG" | wc -l)
    REMAINING=$((END - SECONDS))
    echo "      [${REMAINING}s left] Total:${COUNT} | FNN:${FNN} | WPBA:${WPBA} | LITTLE:${LITTLE} | BIG:${BIG}"
done

# ── Step 6: Generate results ─────────────────────────────────
echo ""
echo "[5/6] Collecting data and generating graphs..."
python3 $EATS_DIR/collect_data.py
python3 $EATS_DIR/plot_results.py

echo ""
echo "[6/6] Done! Opening results graph..."
xdg-open $EATS_DIR/eats_results.png 2>/dev/null &

echo ""
echo "=================================================="
echo "  Results saved to: $EATS_DIR/eats_results.png"
echo "  Raw data saved to: $EATS_DIR/burst_data.csv"
echo "=================================================="
