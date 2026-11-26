# Master–Slave I²C Protocol

The production firmware coordinates **Master V17.4** (Bags 1 and 2) and **Slave V17.2** (Bags 3 and 4) using the standard Arduino `Wire` library.

- **I²C Slave address:** `0x08` (7-bit)
- **Bus:** Uno A4 (`SDA`) and A5 (`SCL`), common GND
- **Nominal transmit cadence:** every 50 ms, plus an immediate STOP packet when requested
- **Packet length:** exactly 7 bytes

| Byte index | Value / meaning |
| --- | --- |
| `0` | `0xA5` frame header |
| `1` | Mode: `0=STOP`, `1=FILL`, `2=DRAIN`, `3=GROW`, `4=HOLD` |
| `2` | Bag 3 inflation PWM (`0..255`) |
| `3` | Bag 3 suction PWM (`0..255`) |
| `4` | Bag 4 inflation PWM (`0..255`) |
| `5` | Bag 4 suction PWM (`0..255`) |
| `6` | XOR checksum of bytes `0..5` |

### Frame example — STOP

```text
A5 00 00 00 00 00 A5
```

### Receiver behavior

1. Reject unexpected packet length, header, invalid mode or checksum.
2. Copy only valid packets from the receive callback into the normal loop.
3. A `STOP` mode forces all Slave outputs off regardless of the PWM fields.
4. An interlock zeroes both channels for a bag if simultaneous inflation and suction are requested.
5. If active and no **valid processed packet** arrives for more than **400 ms**, the Slave disables all pump outputs.

## Known limitations

- XOR catches some transmission errors but is **not** cryptographic, collision-proof or a safety-rated communications protocol.
- `Wire` transactions are not proof of motor activity, pressure, reliable pump delivery or absence of electrical faults.
- The Master logs a missing Slave at startup but can still drive Bags 1 and 2. Check `SLAVE FOUND: 0x08` **before energizing the full installation**.
- A physical emergency power cutoff is required independently of I²C and firmware.
