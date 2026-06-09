#!/bin/bash
echo "[1/4] Loading kernel module..."
sudo insmod ~/eats/eats.ko

echo "[2/4] Starting FNN daemon..."
sudo bash -c "python3 /home/kali/eats/fnn_daemon.py > /tmp/fnn.log 2>&1 &"

echo "[3/4] Waiting 5 minutes to collect data..."
sleep 130

echo "[4/4] Generating results..."
sudo dmesg -C
sleep 120
python3 ~/eats/collect_data.py
python3 ~/eats/train_fnn.py
python3 ~/eats/plot_results.py
xdg-open ~/eats/eats_results.png
