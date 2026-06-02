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
        with open('/proc/eats_hints', 'w') as proc:
            for line in result.stdout.splitlines()[1:]:
                parts = line.split()
                if len(parts) < 2:
                    continue
                try:
                    pid  = int(parts[0])
                    pcpu = float(parts[1])

                    pred_s = pcpu / 100.0
                    core   = 1 if pcpu > 10 else 0

                    X = np.array([[pred_s, core, 1.0]])
                    X_s = scaler.transform(X)
                    pred = model.predict(X_s)[0]
                    pred_ns = max(1000000, int(abs(pred) * 1e9))

                    proc.write(f"{pid} {pred_ns}\n")
                except:
                    pass
    except Exception as e:
        print(f"Error: {e}")

    time.sleep(1)
