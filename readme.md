# 🐧 Energy-Aware Task Scheduler (EATS)
### Weighted Phase-Based Adaptive (WPBA) Predictor

This project is a custom **Linux Kernel Module** designed for heterogeneous (Big.LITTLE) CPU architectures. It solves the limitations of the default Linux scheduler by scientifically predicting task durations to optimize battery life.

## 🚀 Key Features
- **WPBA Algorithm:** A custom predictor that uses three signals (Current Burst, Phase Average, and I/O Ratio) with dynamic weighting.
- **Autonomous Sentinel:** A background kernel thread that automatically monitors and re-routes processes every 3 seconds.
- **Self-Correcting Logic:** The algorithm adjusts its own weights ($w1, w2, w3$) in real-time if predictions deviate from actual CPU usage.
- **Universal Filtering:** Automatically identifies and logs user applications like `python3`, `code`, and `firefox`.

## 📂 Project Structure
- `eats.c`: The core logic, WPBA math, and kernel thread implementation.
- `eats.h`: Definitions for the task history data structures and thresholds.
- `Makefile`: Instructions for the Linux Kbuild system.

## 🛠 Setup & Installation
Every team member must install kernel headers before compiling:
```bash
sudo apt update && sudo apt install linux-headers-$(uname -r) build-essential

Build and Load:
Compile: command " make"
Insert Module:command " sudo insmod eats.ko"
Monitor Logs:command  sudo dmesg -w | grep "EATS WPBA"
🧪 Demonstration
To see the WPBA react to a heavy workload:
Run a heavy task: python3 -c "while True: pass"
Watch the logs as the W1 weight increases and the task is moved to the BIG core.
Course: Linux Kernel Development
Algorithm: WPBA (Weighted Phase-Based Adaptive)

