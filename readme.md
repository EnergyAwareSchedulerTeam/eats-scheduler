# EATS — Energy-Aware Task Scheduler

A Linux kernel module that brings energy-aware, AI-assisted scheduling to heterogeneous (big.LITTLE) CPU architectures. EATS predicts how long each process will run and routes it to the most energy-appropriate CPU core — using a trained neural network as the primary engine and an original adaptive algorithm (WPBA) as an automatic fallback.

> Final-year B.Tech Software Engineering project — Linux Kernel Development course, HIMS Buea, Cameroon, 2026.

## Overview

Modern processors increasingly pair high-performance "big" cores with energy-efficient "LITTLE" cores on the same chip. The Linux kernel's default scheduler (CFS) was designed for identical cores and has no concept of this distinction — it routes tasks without regard for energy cost.

**EATS** solves this by:
1. Predicting how long each running process will need the CPU
2. Classifying that prediction as short, medium, or long
3. Routing the task to an energy-efficient or high-performance core accordingly

It does this through a hybrid prediction system — a trained **Feedforward Neural Network (FNN)** as the primary predictor, and a custom algorithm we designed called **WPBA (Weighted Phase-Based Adaptive Predictor)** as a fallback for processes the network hasn't learned yet.

---

## How It Works

```
Linux Kernel
     │
     ▼
EATS Kernel Module (timer fires every 2s)
     │
     ├─► Check /proc/eats_hints for FNN prediction
     │        │
     │        ├─ Found  → use FNN prediction (primary)
     │        └─ Not found → run WPBA formula (fallback)
     │
     ▼
Classify prediction
     │
     ├─ < 20ms  → LITTLE core (CPU 0, 1) — energy efficient
     ├─ > 35ms  → BIG core    (CPU 2, 3) — high performance
     └─ middle  → ANY core
     │
     ▼
set_cpus_allowed_ptr() enforces the assignment
     │
     ▼
Logged to dmesg for analysis

Meanwhile, in parallel:
Python FNN Daemon (runs every 1s)
     │
     ├─► Reads all running PIDs and CPU usage
     ├─► Runs neural network inference
     └─► Writes predictions to /proc/eats_hints
```

---

## Features

- ✅ Loadable kernel module — no kernel source modification or recompilation required
- ✅ Hybrid AI + algorithmic prediction with automatic graceful fallback
- ✅ Original WPBA algorithm — self-adjusting weights based on prediction accuracy
- ✅ Live `/proc` interface bridging kernel space and Python inference
- ✅ Built-in data collection, training, and visualization pipeline
- ✅ Safe to test in a VM — a crash only affects the VM, never the host
- ✅ Generates before/after energy comparison graphs with matplotlib

---

## Requirements

### Hardware / Environment
- A Linux machine or VM with **at least 4 CPU cores** (used to simulate LITTLE/BIG core split)
- Minimum 4GB RAM
- **Strongly recommended:** run inside a VirtualBox VM, not on bare metal — kernel module bugs can crash the system

### Software
| Tool | Purpose |
|---|---|
| Linux kernel 6.x (tested on Kali 6.18.12) | Target OS |
| Matching `linux-headers` package | Required to build kernel modules |
| GCC + `build-essential` | Compiles the kernel module |
| Python 3.10+ | Runs the FNN daemon, training, and plotting scripts |
| `scikit-learn`, `numpy` | Neural network training and inference |
| `matplotlib` | Results visualization |
| `stress-ng` (optional) | For testing BIG-core assignment under heavy load |

---

## Installation

### 1. Clone the repository
```bash
git clone https://github.com/EnergyAwareSchedulerTeam/eats-scheduler.git
cd eats-scheduler
```

### 2. Install kernel headers and build tools
```bash
sudo apt update
sudo apt install -y build-essential gcc make
sudo apt install -y linux-headers-$(uname -r)
```

> If your distro splits headers into `common` and `arch` packages (common on Kali), also run:
> ```bash
> sudo apt install -y linux-headers-$(uname -r | sed 's/+kali[0-9]*//') 2>/dev/null || true
> ```

### 3. Install Python dependencies
```bash
pip install scikit-learn numpy matplotlib --break-system-packages
```

### 4. Build the kernel module
```bash
make clean && make
```
A successful build produces `eats.ko` in the project directory.

### 5. Make the helper scripts executable
```bash
chmod +x run_eats.sh heavy_test.sh stop_eats.sh
```

---

## Usage

### Quick start
```bash
./run_eats.sh 180
```
This loads the kernel module, starts the FNN prediction daemon, runs for 180 seconds (adjust as needed), then automatically collects data and generates a results graph.

### Manual step-by-step
```bash
# 1. Load the kernel module
sudo insmod eats.ko

# 2. Verify it loaded
lsmod | grep eats
sudo dmesg | tail -5

# 3. Start the FNN prediction daemon
sudo bash -c "python3 fnn_daemon.py > /tmp/fnn.log 2>&1 &"

# 4. Let it run, then inspect live decisions
sudo dmesg | grep EATS | tail -20

# 5. Collect data into a CSV
python3 collect_data.py

# 6. (Optional) Retrain the model with fresh data
python3 train_fnn.py

# 7. Generate the comparison graph
python3 plot_results.py
xdg-open eats_results.png
```

### Test heavy-task routing (BIG core assignment)
```bash
./heavy_test.sh 60
```
Runs a CPU-intensive `stress-ng` workload and confirms it gets routed to BIG cores while idle processes stay on LITTLE cores.

### Stop everything cleanly
```bash
./stop_eats.sh
```

---

## Project Structure

```
eats/
├── eats.c              # Kernel module — core scheduling logic
├── eats.h               # Constants, thresholds, data structures
├── Makefile              # Kernel module build script
├── fnn_weights.h         # Auto-generated FNN weights (C header)
├── fnn_weights.json      # Trained FNN weights (Python-readable)
├── collect_data.py       # Parses dmesg logs into burst_data.csv
├── train_fnn.py          # Trains the neural network
├── fnn_daemon.py         # Runs FNN inference, feeds /proc/eats_hints
├── plot_results.py       # Generates before/after comparison graphs
├── run_eats.sh           # One-command full run script
├── heavy_test.sh         # Heavy-load BIG-core test script
├── stop_eats.sh          # Clean shutdown script
├── burst_data.csv        # Collected training data (generated)
├── eats_results.png      # Output graph (generated)
└── README.md
```

---

## Results

On a 34-minute test session (simulated 4-core heterogeneous environment, Kali Linux 6.18):

| Metric | Value |
|---|---|
| Total scheduling decisions | 1,709 |
| FNN-driven decisions | 1,599 (93.6%) |
| WPBA fallback decisions | 110 (6.4%) |
| Tasks routed to LITTLE cores | 1,521 (89.0%) |
| Tasks routed to BIG cores | 30 (1.8%) |
| **Energy saved vs naive baseline** | **71.2%** |
| FNN model accuracy (R²) | 0.9999 |

See `eats_results.png` after running the system for the full visual breakdown.

---

## Known Limitations

 **Simulated hardware** - EATS runs on standard x86 cores with software-simulated LITTLE/BIG topology; there is no physical power measurement. Development and testing were conducted on an Intel Core i7-7820HQ (7th-gen Kaby Lake), a **homogeneous** quad-core processor with no physical big.LITTLE or P-core/E-core split. This applies whether the module runs inside a VM or on bare-metal Linux on the same machine - the host CPU itself has no heterogeneous cores to exploit. Intel's first hybrid architecture (P-cores/E-cores) appeared with 12th-gen Alder Lake (2021); true ARM big.LITTLE designs are found on boards such as the ODROID-XU4 (Exynos 5422), which is the exact hardware used in Jiang et al.'s 2025 study cited in this project's references. Validating EATS on such hardware is the most direct next step toward physically measured (rather than modelled) energy results.
- **Timer-based, not event-driven** - decisions are made every 2 seconds rather than on every context switch, due to kernel/VM stability constraints
- **Small training dataset** - model trained on a single session (~1,700 records); production use would need far more data
- **No DVFS integration** 0 core assignment only; does not yet control CPU frequency scaling

See the full research paper for a detailed discussion and proposed future work.

---

## Troubleshooting

**`insmod: Unknown symbol in module`**
Your kernel may not export `__tracepoint_sched_switch`. EATS uses a timer-based fallback specifically to avoid this — make sure you're using the version of `eats.c` in this repo, not an older tracepoint-based build.

**System freezes/crashes on `insmod`**
Always test inside a VM. If it happens, force-restart the VM — your host machine is unaffected.

**`/proc/eats_hints` not found**
The kernel module isn't loaded. Run `sudo insmod eats.ko` first.

**FNN daemon writes but kernel shows 0 FNN decisions**
Make sure the daemon is run with `sudo` — writes to `/proc` require root privileges.

**`ModuleNotFoundError: No module named 'sklearn'`**
Install dependencies for the root Python environment too:
```bash
sudo pip install scikit-learn numpy --break-system-packages
```

---

## Authors

- Akoh Rawlings Enow
- Alfred Besong Egbe
-Awunjia Zora Ngosong
- Grace Wozwie
- Ndimbieh Shey Fabien
- Voule-Mo Mumbou Iris Carla
- Bryant Mabought Mbaku

Department of Software Engineering, Higher Institute of Management Studies (HIMS), Buea, Cameroon - 2026.

---

## References

1. Greenhalgh, P. (2011). *big.LITTLE Processing with ARM Cortex-A15 and Cortex-A7.* ARM White Paper.
2. Lozi, J-P. et al. (2016). *The Linux Scheduler: a Decade of Wasted Cores.* EuroSys 2016.
3. Kumar, R. et al. (2003). *Single-ISA Heterogeneous Multi-Core Architectures.* MICRO-36, IEEE.
4. Silberschatz, A., Galvin, P.B. & Gagne, G. (2018). *Operating System Concepts*, 10th Edition. Wiley.
5. Musamih, A. et al. (2025). *Predicting CPU Burst Times with ML to Enhance SJF and SRTF Scheduling.* Journal of Computer Science, Vol. 23.
6. Jiang, L. et al. (2025). *Energy Efficient Task Scheduling for Heterogeneous Multicore Processors in Edge Computing.* Scientific Reports, Nature.
7. Adileh, A. et al. (2016). *Maximizing Heterogeneous Processor Performance Under Power Constraints.* ACM TACO, Vol. 13.

Full citations with links are available in the project's research paper.

---

## License

This project was developed for academic purposes as part of a Linux Kernel Development course. Contact the authors for reuse permissions.
