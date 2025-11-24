#!/usr/bin/env python3
"""TPMS Type 2 Pressure Calibration Analysis"""

# Pomiary rzeczywiste
measurements = [
    {
        "psi": 0,
        "mac": "37:39:02:00:d7:6a",
        "data": "02 1C 18 00 92 6A D7 00 02 39 37",
        "label": "0 PSI (Sensor 2)"
    },
    {
        "psi": 20,
        "mac": "37:39:01:00:aa:13",
        "data": "01 1F 17 01 53 13 AA 00 01 39 37",
        "label": "20 PSI (Sensor 1)"
    },
    {
        "psi": 30,
        "mac": "37:39:02:00:d7:6a",
        "data": "01 1C 17 01 B3 6A D7 00 02 39 37",
        "label": "30 PSI (Sensor 2)"
    }
]

# Parsuj dane
for m in measurements:
    bytes_list = [int(b, 16) for b in m["data"].split()]
    m["bytes"] = bytes_list
    print(f"\n{m['label']}:")
    print(f"  PSI: {m['psi']}")
    print(f"  Hex: {' '.join(f'{b:02X}' for b in bytes_list)}")
    print(f"  Dec: {bytes_list}")

# Testuj różne kombinacje bajtów
print("\n" + "="*70)
print("TESTOWANIE KOMBINACJI BAJTÓW")
print("="*70)

combinations = [
    ("b[0]", lambda b: b[0]),
    ("b[1]", lambda b: b[1]),
    ("b[2]", lambda b: b[2]),
    ("b[3]", lambda b: b[3]),
    ("b[4]", lambda b: b[4]),
    ("b[1:2] LE", lambda b: b[1] + b[2]*256),
    ("b[1:2] BE", lambda b: b[2] + b[1]*256),
    ("b[2:3] LE", lambda b: b[2] + b[3]*256),
    ("b[2:3] BE", lambda b: b[3] + b[2]*256),
    ("b[1:2:3] LE", lambda b: b[1] + b[2]*256 + b[3]*65536),
    ("b[1:2:3] BE", lambda b: b[3] + b[2]*256 + b[1]*65536),
    ("b[0:1:2] LE", lambda b: b[0] + b[1]*256 + b[2]*65536),
    ("b[2:3:4] LE", lambda b: b[2] + b[3]*256 + b[4]*65536),
]

for name, func in combinations:
    vals = [func(m["bytes"]) for m in measurements]
    print(f"\n{name}:")
    for i, m in enumerate(measurements):
        print(f"  {m['label']}: {vals[i]}")
    
    # Sprawdź czy zmienia się z ciśnieniem
    if vals[0] != vals[1]:
        delta = vals[1] - vals[0]
        psi_delta = measurements[1]["psi"] - measurements[0]["psi"]
        ratio = delta / psi_delta if psi_delta != 0 else 0
        print(f"  ΔValue: {delta}, ΔPSI: {psi_delta}, Ratio: {ratio:.4f}")
        
        # Spróbuj różne skalowania
        print(f"  Możliwe skalowania:")
        for scale in [0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 0.01, 0.05, 0.072]:
            psi1 = vals[0] * scale
            psi2 = vals[1] * scale
            err1 = abs(psi1 - measurements[0]["psi"])
            err2 = abs(psi2 - measurements[1]["psi"])
            total_err = err1 + err2
            if total_err < 5.0:  # Tylko sensowne wyniki
                print(f"    scale={scale}: {psi1:.2f} PSI (err={err1:.2f}), {psi2:.2f} PSI (err={err2:.2f})")

print("\n" + "="*70)
print("HIPOTEZA: Ciśnienie w bajtach 1-2 (LE lub BE)?")
print("="*70)

# Jeśli żadna kombinacja nie pasuje, może struktura jest inna
print("\nJeśli powyższe nie pasuje, możliwe że:")
print("- Ciśnienie jest zakodowane z innym offsetem lub skalą")
print("- Dane zawierają checksumę lub flagi")
print("- Struktura zmienia się w zależności od statusu")
print("\nPotrzebny trzeci pomiar na inne ciśnienie (np. 10 lub 15 PSI)")
print("z TEGO SAMEGO sensora, żeby potwierdzić wzór.")
