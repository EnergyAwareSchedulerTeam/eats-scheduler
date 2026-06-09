import subprocess
import time
import json
import numpy as np
from sklearn.neural_network import MLPRegressor
from sklearn.preprocessing import StandardScaler

# Load model
with open('/home/kali/eats/fnn_weights.json') as f:
    w = json.load(f)

scaler = StandardScaler()
scaler.mean_  = np.array(w['scaler_mean'])
scaler.scale_ = np.array(w['scaler_scale'])

model = MLPRegressor()
model.coefs_        = [np.array(w['layer1_weights']), np.array(w['layer2_weights'])]
model.intercepts_   = [np.array(w['layer1_bias']),    np.array(w['layer2_bias'])]
model.n_layers_     = 3
model.n_outputs_    = 1
model.out_activation_ = 'identity'

print("EATS FNN Daemon started")

while True:
    try:
        result = subprocess.run(['ps', '-eo', 'pid,pcpu'],
                               capture_output=True, text=True)
        output = ''
        for line in result.stdout.splitlines()[1:]:
            parts = line.split()
            if len(parts) < 2:
                continue
            try:
                pid  = int(parts[0])
                pcpu = float(parts[1])

                if pcpu == 0.0:
                    pred_ns = 5000000
                elif pcpu < 2.0:
                    pred_ns = 30000000
                else:
                    pred_ns = 80000000

                output += f"{pid} {pred_ns}\n"
            except:
                pass

        # Write all PIDs in one atomic operation
        with open('/proc/eats_hints', 'w') as f:
            f.write(output)

    except Exception as e:
        print(f"Error: {e}")

    time.sleep(1)
