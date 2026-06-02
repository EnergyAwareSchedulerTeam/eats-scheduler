import json
import numpy as np

SCALE = 1000000  # fixed-point scale

with open('/home/kali/eats/fnn_weights.json') as f:
    w = json.load(f)

def to_fixed(data):
    flat = np.array(data).flatten()
    return [int(round(v * SCALE)) for v in flat]

def arr_to_c(name, data):
    vals = ', '.join(str(v) for v in data)
    return f"static long long {name}[] = {{{vals}}};\n"

l1w = np.array(w['layer1_weights'])
l2w = np.array(w['layer2_weights'])

out  = "/* Auto-generated fixed-point FNN weights */\n"
out += "#ifndef FNN_WEIGHTS_H\n#define FNN_WEIGHTS_H\n\n"
out += f"#define FNN_SCALE       {SCALE}LL\n"
out += f"#define FNN_INPUT_SIZE  3\n"
out += f"#define FNN_HIDDEN_SIZE {l1w.shape[1]}\n"
out += f"#define FNN_OUTPUT_SIZE 1\n\n"
out += arr_to_c("fnn_scaler_mean",  to_fixed(w['scaler_mean']))
out += arr_to_c("fnn_scaler_scale", to_fixed(w['scaler_scale']))
out += arr_to_c("fnn_l1_weights",   to_fixed(w['layer1_weights']))
out += arr_to_c("fnn_l1_bias",      to_fixed(w['layer1_bias']))
out += arr_to_c("fnn_l2_weights",   to_fixed(w['layer2_weights']))
out += arr_to_c("fnn_l2_bias",      to_fixed(w['layer2_bias']))
out += "\n#endif /* FNN_WEIGHTS_H */\n"

with open('/home/kali/eats/fnn_weights.h', 'w') as f:
    f.write(out)

print("Exported fixed-point fnn_weights.h")
