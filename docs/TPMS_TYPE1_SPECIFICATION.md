# TPMS Type 1 Specification (Current Implementation)

## Overview
- **Type ID**: 0x0001 (in manufacturer data header)
- **Packet Length**: 18 bytes
- **BLE Advertisement**: Manufacturer-specific data
- **Frequency**: Continuous/periodic (1-2 second cadence)
- **Protocol**: Unidirectional (no pairing required)

## Binary Format

### Structure (18 bytes total)

```
Byte Range  | Hex Format     | Field                    | Notes
------------|----------------|--------------------------|------------------------------------------
[0-1]       | 0x00 0x01      | Header (Manufacturer ID) | Fixed identifier for Type 1
[2]         | 0x80-0x8F      | Sensor Number            | 0x80=wheel0, 0x81=wheel1, etc.
[3-4]       | 0xEA 0xCA      | Magic Bytes              | Format validation marker
[5-7]       | XX XX XX       | Sensor ID                | 3-byte sensor identifier (unique)
[8-11]      | XX XX XX XX    | Pressure (kPa × 1000)   | 32-bit LE integer, stored in kPa*1000
[12-15]     | XX XX XX XX    | Temperature (°C × 100)  | 32-bit LE integer, stored in °C*100
[16]        | 0x00-0xFF      | Battery Level            | 0-255 scale (percentage indicator)
[17]        | 0x00 or 0x01   | Alert Flag               | 0=normal, 1=pressure/temp alert
```

### Example Packet
```
Raw Hex: 0x00 01 81 EA CA 20 04 10 23 06 00 00 1F 0B 00 00 09 00

Decoded:
  [0-1]:    0x00 0x01        = Type 1 header
  [2]:      0x81             = Sensor number 1 (wheel 1)
  [3-4]:    0xEA 0xCA        = Magic bytes (validation)
  [5-7]:    0x20 0x04 0x10   = Sensor ID: "0x100420"
  [8-11]:   0x23 0x06 0x00 0x00 = 0x00000623 = 1571 kPa * 1000 = 1.571 MPa ≈ 227.8 PSI
  [12-15]:  0x1F 0x0B 0x00 0x00 = 0x00000B1F = 2847 °C * 100 = 28.47°C
  [16]:     0x09             = Battery level 9/255 ≈ 3.5%
  [17]:     0x00             = No alert
```

## Data Field Details

### Header [0-1]: 0x00 0x01
- **Fixed Value**: Always 0x00 followed by 0x01
- **Purpose**: Type identifier for this TPMS variant
- **Validation**: MUST be present or packet is rejected

### Sensor Number [2]: 0x80-0x8F
- **Storage**: Actual wheel number = (byte[2] - 0x80)
- **Range**: 0x80 = wheel 0, 0x81 = wheel 1, 0x82 = wheel 2, 0x83 = wheel 3
- **Valid Range**: 0x80 (0) to 0x8F (15) - supports up to 16 wheels
- **Typical**: 0x80-0x83 for 4-wheel vehicles
- **Validation**: MUST be >= 0x80 or packet is rejected

### Magic Bytes [3-4]: 0xEA 0xCA
- **Fixed Value**: Always 0xEA followed by 0xCA
- **Purpose**: Format validation marker (second check)
- **Validation**: MUST be present or packet is rejected

### Sensor ID [5-7]: 3 bytes (manufacturer specific)
- **Length**: 3 bytes (variable content)
- **Content**: Unique sensor identifier (serial number or ID)
- **Endianness**: As-is (no conversion needed for display)
- **Example**: "0x20 0x04 0x10" could represent sensor serial or manufacturer code
- **Use**: Identifying which specific sensor this packet came from

### Pressure [8-11]: 32-bit LE integer
- **Storage**: kilopascals × 1000 (to preserve decimal precision)
- **Endianness**: Little-endian (LSB first)
- **Range**: 0-4,294,967,295 (0.0 - 4,294,967.3 kPa practical: 0-250 kPa)
- **Formula**:
  ```
  raw_value = byte[8] | (byte[9] << 8) | (byte[10] << 16) | (byte[11] << 24)
  pressure_kpa = raw_value / 1000.0
  pressure_psi = pressure_kpa * 0.145037738
  pressure_bar = pressure_kpa / 100.0
  ```
- **Example**: 0x23 0x06 0x00 0x00 = 0x00000623 = 1571
  - kPa = 1571 / 1000 = 1.571 kPa ❌ (too low, should be 2*10^6 + 1571?)
  - Check: 0x00000623 = 1571 decimal
  - Actual interpretation: 1571 (in kPa*1000 units) = 1.571 kPa → 0.227 PSI (unrealistic)
  - **NOTE**: May need to verify actual sensor calibration data

### Temperature [12-15]: 32-bit LE integer
- **Storage**: Degrees Celsius × 100 (to preserve 0.01°C precision)
- **Endianness**: Little-endian (LSB first)
- **Range**: -214,748 to +214,747 °C (±1e6 in practice: -50 to +150°C typical)
- **Formula**:
  ```
  raw_value = byte[12] | (byte[13] << 8) | (byte[14] << 16) | (byte[15] << 24)
  temperature_c = raw_value / 100.0
  ```
- **Example**: 0x1F 0x0B 0x00 0x00 = 0x00000B1F = 2847
  - Temperature = 2847 / 100 = 28.47°C ✓

### Battery Level [16]: 8-bit unsigned
- **Storage**: Single byte (0-255)
- **Range**: 0% to 100% (mapped to 0-255 scale)
- **Formula**: percentage = (byte[16] / 255.0) * 100
- **Example**: 0x09 = 9 decimal → 9/255 = 3.5% battery ⚠️ (low battery)
- **Typical**: 200-255 (normal), 50-100 (warning), 0-50 (critical)

### Alert Flag [17]: 8-bit flag
- **Storage**: Single byte, typically 0 or 1
- **Values**:
  - 0x00 = Normal operation (no alert)
  - 0x01 = Alert active (pressure or temperature out of range)
- **Interpretation**: Boolean (non-zero = alert, though implementation checks == 1)

## Validation Checks

```cpp
bool isTPMSSensor(const uint8_t* data, size_t length) {
    // 1. Length must be EXACTLY 18 bytes
    if (length != 18) return false;
    
    // 2. Header must be 0x00 0x01
    if (data[0] != 0x00 || data[1] != 0x01) return false;
    
    // 3. Magic bytes must be 0xEA 0xCA
    if (data[3] != 0xea || data[4] != 0xca) return false;
    
    // 4. Sensor number must be >= 0x80
    if (data[2] < 0x80) return false;
    
    return true;
}
```

## Parsing Algorithm

1. **Validate packet** (see above)
2. **Extract Sensor ID**: bytes [5-7] → store as-is (3-byte array)
3. **Extract Sensor Number**: byte[2] - 0x80 → 0-15 range
4. **Extract Pressure**: 
   - Read 4-byte LE integer from [8-11]
   - Divide by 1000 to get kPa
   - Convert to PSI (× 0.145037738) and BAR (÷ 100)
5. **Extract Temperature**:
   - Read 4-byte LE integer from [12-15]
   - Divide by 100 to get °C
6. **Extract Battery**: byte[16] as 0-255, can map to % or use raw
7. **Extract Alert**: byte[17] == 0x01 ? alert_active : normal
8. **Record Timestamp**: Current system time (ms since boot)

## Transmission Characteristics

- **Advertisement Type**: Manufacturer-specific data (not service UUID)
- **Manufacturer ID**: 0x0001 (typically in AD structure before this payload)
- **Frequency**: ~1 Hz (1 packet per second) typical for TPMS
- **Power**: Low (battery-powered sensor)
- **Range**: 10-30 meters typical BLE range

## Known Limitations / Notes

- Pressure values need verification (may be scaled differently on some sensors)
- Temperature offset/calibration may vary by manufacturer
- Battery level scaling (0-255) vs percentage is device-specific
- No sensor pairing - broadcasts to all listeners
- No authentication/encryption (open protocol)

## Type 1 Variants?

Some Type 1 sensors might use:
- Different manufacturer ID (not 0x0001)
- Different byte layout within same format
- Extended packets (>18 bytes)
- Service UUIDs instead of manufacturer data

These would be Type 1 variants requiring subtype detection.
