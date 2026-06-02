import subprocess
import csv
import re

result = subprocess.run(['dmesg'], capture_output=True, text=True)
lines = result.stdout.splitlines()

rows = []
for line in lines:
    m = re.search(r'EATS \[(\w+)\]: \[(.+?)\] PID:(\d+) -> (\w+) \(Pred:(\d+) ns\)', line)
    if m:
        rows.append({
            'engine':       m.group(1),
            'process':      m.group(2).strip(),
            'pid':          m.group(3),
            'core':         1 if m.group(4) == 'BIG' else (0 if m.group(4) == 'LITTLE' else 2),
            'predicted_ns': int(m.group(5)),
        })

if not rows:
    print("No EATS log entries found. Is the module loaded?")
else:
    with open('/home/kali/eats/burst_data.csv', 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"Saved {len(rows)} records to burst_data.csv")
