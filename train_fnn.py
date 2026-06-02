import numpy as np
import csv
from sklearn.neural_network import MLPRegressor
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
import json

rows = []
with open('/home/kali/eats/burst_data.csv') as f:
    for row in csv.DictReader(f):
        rows.append(row)

# Features: predicted_ns, core assignment, engine used
X = np.array([[
    float(r['predicted_ns']) / 1e9,
    float(r['core']),
    1.0 if r['engine'] == 'FNN' else 0.0
] for r in rows])

y = np.array([float(r['predicted_ns']) / 1e9 for r in rows])

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42)

scaler = StandardScaler()
X_train_s = scaler.fit_transform(X_train)
X_test_s  = scaler.transform(X_test)

model = MLPRegressor(
    hidden_layer_sizes=(32, 16),
    max_iter=2000,
    random_state=42,
    learning_rate='adaptive'
)
model.fit(X_train_s, y_train)

score = model.score(X_test_s, y_test)
print(f"R² score: {score:.4f}")

weights = {
    'scaler_mean':    scaler.mean_.tolist(),
    'scaler_scale':   scaler.scale_.tolist(),
    'layer1_weights': model.coefs_[0].tolist(),
    'layer1_bias':    model.intercepts_[0].tolist(),
    'layer2_weights': model.coefs_[1].tolist(),
    'layer2_bias':    model.intercepts_[1].tolist(),
}

with open('/home/kali/eats/fnn_weights.json', 'w') as f:
    json.dump(weights, f, indent=2)

print("Weights saved to fnn_weights.json")
print(f"Training: {len(X_train)}, Test: {len(X_test)}")
