import os
import subprocess
import time

def run_demo():
    print("🚀 Starting EATS Scheduler Demo...")
    
    # 1. Start a dummy 'heavy' process (a simple math loop)
    proc = subprocess.Popen(["python3", "-c", "while True: pass"])
    pid = proc.pid
    print(f"✅ Created test process with PID: {pid}")
    
    try:
        # 2. Tell our Kernel Module to handle this PID
        print(f"📡 Sending PID {pid} to /proc/eats_control...")
        with open("/proc/eats_control", "w") as f:
            f.write(str(pid))
        
        # 3. Wait and show the logs
        time.sleep(1)
        print("\n📜 Kernel Log Output:")
        os.system("sudo dmesg | tail -n 3")
        
        print(f"\n✨ Task managed! Check how EATS moved PID {pid}")
        
    finally:
        proc.terminate()
        print("\n🛑 Demo stopped.")

if __name__ == "__main__":
    run_demo()
