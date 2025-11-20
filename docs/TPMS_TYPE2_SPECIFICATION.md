# TPMS Type 2 Specification (TC.TPMS Implementation)

## Overview
- **Type ID**: 0xA828 (BLE service UUID)
- **Packet Length**: 11 bytes (manufacturer data)
- **BLE Advertisement**: Service UUID 0xA828 + manufacturer data
- **Frequency**: Continuous/periodic (1-2 second cadence)
- **Protocol**: Unidirectional (no pairing required)
- **Manufacturer**: TC.TPMS (based on MAC address prefix 37:39)

## Binary Format

### Structure (11 bytes total)

```
Byte Range  | Hex Format     | Field                    | Notes
------------|----------------|--------------------------|------------------------------------------
[0]         | 0x00-0xFF      | Status Byte              | Bit 1 = alarm flag (0x02 = alert)
[1]         | 0x14-0x1E      | Battery Voltage          | ×0.1V (20=2.0V, 30=3.0V)
[2]         | 0x00-0xFF      | Temperature              | Direct °C (no scaling)
[3]         | XX             | Pressure MSB             | Upper byte of 16-bit LE pressure
[4]         | XX             | Pressure LSB             | Lower byte of 16-bit LE pressure
[5-10]      | XX XX XX XX XX XX | Sensor ID (MAC)     | Full 6-byte MAC address
```

### Example Packet
```
Raw Hex: 0x00 0x1C 0x17 0x01 0xB3 0x37 0x39 0x02 0x00 0xD7 0x6A

Decoded:
  [0]:      0x00             = Status: normal (no alarm)
  [1]:      0x1C             = Battery: 28 × 0.1 = 2.8V
  [2]:      0x17             = Temperature: 23°C
  [3-4]:    0x01 0xB3        = Pressure: 0x01B3 = 435 decimal
  [5-10]:   0x37 0x39 0x02 0x00 0xD7 0x6A = MAC: 37:39:02:00:D7:6A
```

## Data Field Details

### Status Byte [0]: 8-bit status flags
- **Bit 1 (0x02)**: Alarm flag
  - 0 = Normal operation
  - 1 = Alert active (pressure/temperature out of range)
- **Other bits**: Reserved/unknown
- **Formula**: `alarm = (byte[0] & 0x02) != 0`
- **Example**: 0x00 = normal, 0x02 = alarm active

### Battery Voltage [1]: 8-bit scaled voltage
- **Storage**: Voltage × 10 (to preserve 0.1V precision)
- **Range**: 0x14-0x1E (2.0V - 3.0V typical)
- **Formula**:
  ```cpp
  voltage_v = byte[1] * 0.1f
  percentage = min((voltage_v - 2.0f) * 100.0f, 100.0f)
  ```
- **Example**: 0x1C = 28 decimal → 2.8V → (2.8-2.0)×100 = 80%
- **Typical Range**: 2.0V (0%) to 3.0V (100%)

### Temperature [2]: 8-bit direct Celsius
- **Storage**: Direct Celsius value (no scaling)
- **Range**: 0-255°C (practical: -50 to +150°C)
- **Formula**: `temperature_c = (float)byte[2]`
- **Example**: 0x17 = 23 decimal → 23°C
- **Resolution**: 1°C (integer precision)

### Pressure [3-4]: 16-bit little-endian
- **Storage**: Raw 16-bit value (little-endian)
- **Endianness**: LSB first (byte[4] = LSB, byte[3] = MSB)
- **Formula**:
  ```cpp
  raw_value = byte[4] + (byte[3] << 8)  // 16-bit LE
  pressure_psi = 0.10223139f * raw_value - 14.61232950f
  ```
- **Example**: 0x01 0xB3 = 0x01B3 = 435 decimal
  - PSI = 0.10223139 × 435 - 14.61232950 ≈ 44.5 - 14.6 = 29.9 PSI
- **Calibration**: Derived from 4-point linear regression
- **Accuracy**: ±0.14 PSI (verified calibration)

### Sensor ID [5-10]: 6-byte MAC address
- **Content**: Complete BLE MAC address of the sensor
- **Format**: Standard MAC address format (XX:XX:XX:XX:XX:XX)
- **Structure**:
  ```
  [5-6]: 0x37:0x39     = Manufacturer prefix (TC.TPMS)
  [7]:   0x01-0x04     = Wheel number (1=FL, 2=FR, 3=RL, 4=RR)
  [8-10]: XX:XX:XX    = Unique sensor ID (3 bytes)
  ```
- **Example**: 0x37 0x39 0x02 0x00 0xD7 0x6A = "37:39:02:00:D7:6A"
  - Manufacturer: TC.TPMS
  - Wheel: 2 (front right)
  - Sensor ID: 00:D7:6A

## MAC Address Structure

### Full MAC Format: 37:39:WW:XX:YY:ZZ
```
Position | Value          | Meaning
---------|----------------|---------------------------------
[0-1]    | 37:39         | Manufacturer ID (TC.TPMS)
[2]      | 01-04         | Wheel position
[3-5]    | XX:YY:ZZ      | Unique sensor identifier
```

### Wheel Number Mapping
```
Hex Value | Decimal | Wheel Position
----------|---------|---------------
0x01      | 1       | Front Left (FL)
0x02      | 2       | Front Right (FR)
0x03      | 3       | Rear Left (RL)
0x04      | 4       | Rear Right (RR)
```

## Validation Checks

```cpp
bool isTPMSSensor_Type2(const uint8_t* data, size_t length) {
    // Type 2 validation is minimal - just check length
    return (length == 11);
}
```

**Note**: Type 2 sensors are identified by the service UUID 0xA828 rather than manufacturer data validation. The 11-byte length check is the primary validation.

## Parsing Algorithm

1. **Validate packet length** (must be 11 bytes)
2. **Extract Status**: byte[0] → check bit 1 for alarm flag
3. **Extract Battery**:
   - Raw voltage = byte[1] × 0.1V
   - Percentage = min((voltage - 2.0) × 100, 100%)
4. **Extract Temperature**: byte[2] as direct °C
5. **Extract Pressure**:
   - Raw = byte[4] + (byte[3] << 8) (16-bit LE)
   - PSI = 0.10223139 × raw - 14.61232950
6. **Extract MAC Address**: bytes[5-10] as complete MAC
7. **Parse Wheel Number**: MAC byte[2] (1-4)
8. **Parse Sensor ID**: MAC bytes[3-5] (3 bytes)
9. **Record Timestamp**: Current system time (ms since boot)

## Transmission Characteristics

- **Advertisement Type**: BLE service advertisement
- **Service UUID**: 0xA828 (TPMS Type 2 identifier)
- **Manufacturer Data**: 11 bytes (status, battery, temp, pressure, MAC)
- **Frequency**: ~1 Hz (1 packet per second) typical
- **Power**: Low (battery-powered sensor)
- **Range**: 10-30 meters typical BLE range
- **Identification**: MAC address based (no separate sensor ID field)

## Calibration Data

### Pressure Formula Derivation
- **Method**: 4-point linear regression calibration
- **Points**: Measured PSI vs raw sensor values
- **Formula**: PSI = 0.10223139 × raw - 14.61232950
- **Accuracy**: ±0.14 PSI (verified against known pressures)
- **Range**: Tested 20-45 PSI (typical operating range)

### Battery Voltage Mapping
- **Linear Scale**: 2.0V = 0%, 3.0V = 100%
- **Formula**: percentage = (voltage - 2.0) × 100
- **Cap**: Maximum 100% (voltages > 3.0V still show 100%)
- **Resolution**: 0.1V input → 10% output steps

## Known Limitations / Notes

- **MAC-based ID**: Sensor identification relies on BLE MAC address
- **No explicit validation**: Minimal packet validation (length only)
- **Wheel number in MAC**: Position encoded in MAC byte[2]
- **Service UUID identification**: Uses 0xA828 service UUID for detection
- **Pressure calibration required**: Each sensor batch may need recalibration
- **Temperature integer precision**: 1°C resolution (no decimal places)
- **Battery estimation**: Percentage derived from voltage (not direct measurement)

## Comparison with Type 1

| Aspect              | Type 1                          | Type 2                          |
|---------------------|---------------------------------|---------------------------------|
| **Packet Length**  | 18 bytes                       | 11 bytes                       |
| **Identification** | Service UUID 0xA828            | Service UUID 0xA828            |
| **Sensor ID**      | 3-byte field in data           | MAC address bytes[3-5]         |
| **Wheel Number**   | Data byte[2] - 0x80            | MAC byte[2]                    |
| **Pressure**       | 32-bit kPa × 1000              | 16-bit calibrated PSI          |
| **Temperature**    | 32-bit °C × 100                | 8-bit direct °C                |
| **Battery**        | 8-bit raw (0-255)              | 8-bit voltage × 10             |
| **Alert**          | 8-bit flag                     | Status byte bit 1              |
| **Validation**     | Header + magic bytes           | Length only                    |

## Implementation Notes

- **ESP32 Compatibility**: Designed for ESP-IDF framework
- **Memory Management**: Objects allocated with `new`, caller must `delete`
- **Thread Safety**: Not thread-safe, use appropriate locking
- **BLE Stack**: Requires NimBLE for service UUID scanning
- **Timestamp**: Uses `esp_timer_get_time() / 1000` (ms since boot)</content>
<parameter name="filePath">E:\Programming\ProjektyMCU\EclipseESP32Hello\docs\TPMS_TYPE2_SPECIFICATION.md