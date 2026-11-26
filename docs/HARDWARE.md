# Hardware and Wiring

This document records the tested **control-pin mapping** for the current four-bag prototype. It is not an electrical engineering certification or a complete wiring diagram for an arbitrary L298N variant.

## System components

- 2 × Arduino Uno, designated **Master** and **Slave**
- 2 × TTP223 capacitive touch modules, default active-HIGH output
- 4 × L298N dual H-bridge motor driver boards
- 4 inflation pumps + 4 suction pumps, one pair for each air bag
- 4 independent pneumatic bags; fittings, pressure-release paths and suitable tubing
- 2 conductive touch plates and insulated sensing leads
- 1 low-current breadboard for touch power distribution, jumper wires
- External pump power supply sized to the **actual pump ratings**; physical emergency cutoff

## Controllers

| Master Uno | Slave Uno | Purpose |
| --- | --- | --- |
| `A4` (SDA) | `A4` (SDA) | I²C data |
| `A5` (SCL) | `A5` (SCL) | I²C clock |
| `GND` | `GND` | Common signal reference |

- Slave I²C address: `0x08`.
- Do **not** connect the 5 V pins of two separately USB-powered Unos directly together.
- Connect the motor-driver **logic grounds** to the relevant controller reference ground, with pump-current returns routed separately from the breadboard.
- The Uno's A4/A5 bus is used for controller-to-controller communication; the TTP223 sensors are **digital outputs**, not I²C devices.

## Touch modules — Master only

| Module | Module pin | Master destination |
| --- | --- | --- |
| LEFT TTP223 | `OUT` | `D10` |
| RIGHT TTP223 | `OUT` | `D11` |
| Both (if rated for 5 V) | `VCC` | Regulated `5V` touch-supply rail |
| Both | `GND` | Common ground rail |

Connect each external metal plate only to the **sensor electrode** for its own TTP223 board; do not connect the plate to `OUT`, `VCC` or `GND`. Keep conductive plates and foil separated. Use the module's specified supply voltage; default HIGH-on-touch behavior is assumed. Long wires/large plates can require recalibration and noise testing.

**MPR121 and the eight-channel voltage-level translator are not part of this version.**

## Motor driver pin mapping

Each bag uses **one L298N board**: one H-bridge channel drives its inflation pump; the other drives its suction pump. For the L298N board labels, the fill channel uses **ENA/IN1/IN2/OUT1/OUT2** and the suction channel uses **ENB/IN3/IN4/OUT3/OUT4**.

### Master — Bags 1 and 2

| Bag | Pump | EN (PWM) | First direction input | Second direction input |
| --- | --- | --- | --- | --- |
| **1** | Inflation | `D6` | `D12` | `A0` |
| 1 | Suction | `D9` | `A1` | `A2` |
| **2** | Inflation | `D3` | `D2` | `D4` |
| 2 | Suction | `D5` | `D7` | `D8` |

### Slave — Bags 3 and 4

| Bag | Pump | EN (PWM) | First direction input | Second direction input |
| --- | --- | --- | --- | --- |
| **3** | Inflation | `D3` | `D2` | `D4` |
| 3 | Suction | `D5` | `D7` | `D8` |
| **4** | Inflation | `D6` | `D12` | `A0` |
| 4 | Suction | `D9` | `A1` | `A2` |

The unused `D13` pin of the Slave is left unconnected. The Arduino analog pins `A0–A2` are used as **digital outputs**, not sensors, in the motor-driver mapping.

### Driver wiring notes

- Remove **ENA/ENB jumpers** as appropriate for your L298N variant if the Arduino EN pins provide PWM.
- L298N modules vary: check motor-voltage terminal markings, onboard 5V regulator, and any `5V-EN` jumper configuration against the exact board documentation.
- Supply **motor power externally** at the pump's rated voltage. The module's `+12V` terminal label on some boards does **not** mean every pump should run at 12V.
- Pumps can have high startup current; size wiring, fuses, supplies and drivers to the actual loads. The L298N can dissipate significant heat and voltage drop.
- Do not supply eight pumps through Arduino `5V`, a breadboard rail, or Arduino GND traces.
- Venting and physical emergency cutoff are necessary. A five-second timeout is not a verified pressure limit.

## Programming / power-up sequence

1. Cut motor supply power.
2. Upload `firmware/UTERUS_Slave/UTERUS_Slave.ino` to the **Slave** Uno.
3. Upload `firmware/UTERUS_Master/UTERUS_Master.ino` to the **Master** Uno.
4. Verify `SLAVE FOUND: 0x08` in the Master Serial Monitor (9600 baud).
5. Confirm sensor readings using `examples/TTP223_Touch_Test` if needed, then re-upload production Master firmware.
6. Test individual pumps at low pressure with separate test sketches and a working physical cutoff before connecting the full load.
