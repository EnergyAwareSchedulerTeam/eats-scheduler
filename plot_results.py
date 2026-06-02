import subprocess
import re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

result = subprocess.run(['dmesg'], capture_output=True, text=True)
lines = result.stdout.splitlines()

fnn_little = fnn_big = fnn_any = 0
wpba_little = wpba_big = wpba_any = 0
fnn_preds = []
wpba_preds = []

for line in lines:
    m = re.search(r'EATS \[(\w+)\]: .* -> (\w+) \(Pred:(\d+) ns\)', line)
    if not m: continue
    engine, core, pred = m.group(1), m.group(2), int(m.group(3))

    if engine == 'FNN':
        fnn_preds.append(pred / 1e6)
        if core == 'LITTLE': fnn_little += 1
        elif core == 'BIG':  fnn_big += 1
        else:                fnn_any += 1
    else:
        wpba_preds.append(pred / 1e6)
        if core == 'LITTLE': wpba_little += 1
        elif core == 'BIG':  wpba_big += 1
        else:                wpba_any += 1

# Naive baseline — everything on BIG
total = fnn_little+fnn_big+fnn_any+wpba_little+wpba_big+wpba_any
naive_big = total

# Energy model (mW per task unit)
BIG_MW    = 320
LITTLE_MW = 80

eats_energy  = (fnn_little+wpba_little)*LITTLE_MW + \
               (fnn_big+wpba_big)*BIG_MW + \
               (fnn_any+wpba_any)*((BIG_MW+LITTLE_MW)//2)
naive_energy = total * BIG_MW
saved_pct    = round((1 - eats_energy/naive_energy)*100, 1) if naive_energy else 0

fig, axes = plt.subplots(1, 3, figsize=(15, 5))
fig.suptitle('EATS Scheduler — FNN + WPBA Results', fontsize=14, fontweight='bold')
fig.patch.set_facecolor('#0d1117')
for ax in axes:
    ax.set_facecolor('#161b22')
    ax.tick_params(colors='white')
    ax.xaxis.label.set_color('white')
    ax.yaxis.label.set_color('white')
    ax.title.set_color('white')
    for spine in ax.spines.values():
        spine.set_edgecolor('#444')

# Graph 1 — Core distribution
ax1 = axes[0]
cats = ['LITTLE\n(efficient)', 'BIG\n(performance)', 'ANY']
eats_vals  = [fnn_little+wpba_little, fnn_big+wpba_big, fnn_any+wpba_any]
naive_vals = [0, total, 0]
x = np.arange(3)
ax1.bar(x-0.2, naive_vals, 0.35, label='Naive (CFS)', color='#f85149', alpha=0.85)
ax1.bar(x+0.2, eats_vals,  0.35, label='EATS',        color='#3fb950', alpha=0.85)
ax1.set_title('Task Distribution by Core Type')
ax1.set_xticks(x); ax1.set_xticklabels(cats)
ax1.legend(facecolor='#21262d', labelcolor='white')

# Graph 2 — Energy comparison
ax2 = axes[1]
ax2.bar(['Naive (all-BIG)', 'EATS (FNN+WPBA)'],
        [naive_energy, eats_energy],
        color=['#f85149', '#3fb950'], alpha=0.85, width=0.4)
ax2.set_title(f'Total Energy Consumption\nSaved: {saved_pct}%')
ax2.set_ylabel('Energy (mW units)')

# Graph 3 — FNN vs WPBA prediction distribution
ax3 = axes[2]
if fnn_preds:
    ax3.hist(fnn_preds,  bins=20, alpha=0.75, color='#2563EB', label='FNN')
if wpba_preds:
    ax3.hist(wpba_preds, bins=20, alpha=0.75, color='#f97316', label='WPBA')
ax3.axvline(x=20,  color='cyan',  linestyle='--', linewidth=1, label='LITTLE threshold (20ms)')
ax3.axvline(x=50,  color='red',   linestyle='--', linewidth=1, label='BIG threshold (50ms)')
ax3.set_title('Prediction Distribution (ms)')
ax3.set_xlabel('Predicted Burst (ms)')
ax3.set_ylabel('Count')
ax3.legend(facecolor='#21262d', labelcolor='white', fontsize=8)

plt.tight_layout()
plt.savefig('/home/kali/eats/eats_results.png', dpi=150,
            bbox_inches='tight', facecolor='#0d1117')
print(f"Saved eats_results.png")
print(f"Total tasks:    {total}")
print(f"FNN decisions:  {fnn_little+fnn_big+fnn_any}")
print(f"WPBA decisions: {wpba_little+wpba_big+wpba_any}")
print(f"Energy saved:   {saved_pct}%")
print(f"LITTLE tasks:   {fnn_little+wpba_little}")
print(f"BIG tasks:      {fnn_big+wpba_big}")
