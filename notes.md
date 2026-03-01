# Notes

```text
LoRa message received from Device 1, Type: 5, ID: 194, Dst: 0, PayloadLen: 31, RSSI: -20
    Raw payload: 6 0 0 0 0 1 1 0 0 0 F2 2 0 0 0 19 3 0 0 0 8 4 0 0 0 34 5 0 0 0 0
Value message from Device 1
    Number of values: 6
    Value 0: Entity ID=0, Value=1
    Value 1: Entity ID=1, Value=242
    Value 2: Entity ID=2, Value=25
    Value 3: Entity ID=3, Value=8
    Value 4: Entity ID=4, Value=52
    Value 5: Entity ID=5, Value=0
```

## Entity 0

```text
Discovery message from Device 1
    Entity ID: 0, Type: 2, Device Class: 6, Unit: 0, Format: 0x0

    Type: 2=Cover
    Device Class: 6=garage
    Unit: 0=None
    Format: 0x00=unsigned, 1 byte, 0 decimals
```

```text
Value payload
00 00 00 01
```

Value=1 => True => Open

## Entity 1

```text
Discovery message from Device 1
    Entity ID: 1, Type: 1, Device Class: 39, Unit: 1, Format: 0x15

    Type: 1=Sensor
    Device Class: 39=temperature
    Unit: 1=celcius => °C
    Format: 0x15=signed, 2 bytes, 1 decimal
```

```text
Value payload
00 00 00 F2
```

Value=0x00F2 => 242 => 24.2 °C
