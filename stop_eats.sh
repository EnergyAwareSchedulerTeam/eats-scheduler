#!/bin/bash
# ═══════════════════════════════════════════════════════════
# EATS — Stop Script
# Cleanly stops all EATS components
# ═══════════════════════════════════════════════════════════

echo "Stopping EATS..."
pkill -f fnn_daemon.py 2>/dev/null && echo "✓ FNN daemon stopped"
sudo rmmod eats 2>/dev/null && echo "✓ Kernel module unloaded"
echo "EATS stopped cleanly."
