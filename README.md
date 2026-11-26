# UTERUS — Interactive Pneumatic Installation

**Open-source Arduino firmware for a touch-responsive, four-airbag art installation.**

UTERUS explores breath, bodily presence, and rhythmic transformation through a physical, womb-inspired sculptural system. Two capacitive touch sensors let visitors modulate inflation and deflation directly, while a programmed **180-second, five-stage choreography** coordinates four air bags.

> **Research and artistic prototype — not a medical device.** Pump timing is programmed; air pressure, airflow, deformation and bag volume are **not measured**. A software timer or touch gesture is **not** a certified pressure limiter or emergency stop. Read [Safety and limitations](docs/SAFETY.md) before energizing any pump.

## Overview

| Subsystem | Hardware | Role |
| --- | --- | --- |
| Master controller | Arduino Uno (5 V) | Touch input, choreography, Bags 1–2 |
| Slave controller | Arduino Uno (5 V) | Bags 3–4, I²C listener at `0x08` |
| Touch interface | 2 × TTP223 (active HIGH) | Left / right / both hand detection |
| Motor drivers | 4 × L298N modules | Two pumps per air bag |
| Pneumatics | 4 air bags + 8 pumps | Dedicated inflation and suction pumps |
| User interface | 2 isolated conductive touch plates | Gesture input via the TTP223 sensing electrodes |

The design is an **artistic reference to gestational space and breathing**; it does not simulate a validated physiological process.

## Repository structure

```text
UTERUS_GitHub_Ready/
├── firmware/
│   ├── UTERUS_Master/UTERUS_Master.ino   # Master V17.4
│   └── UTERUS_Slave/UTERUS_Slave.ino     # Slave V17.2
├── examples/
│   ├── Airbag_Test_Master/               # Command one pump at a time
│   ├── Airbag_Test_Slave/                # Standalone Slave diagnostic
│   └── TTP223_Touch_Test/                # LEFT / RIGHT / BOTH / NONE
├── docs/
│   ├── HARDWARE.md
│   ├── INTERACTION.md
│   ├── PROTOCOL.md
│   ├── CHOREOGRAPHY.md
│   └── SAFETY.md
├── tests/                               # Host behavior tests
├── .github/workflows/                    # CI: mock tests + AVR compile
├── LICENSE                              # MIT (code and original docs)
├── CONTRIBUTING.md
└── README.md
```

Each `.ino` file resides in a directory with **exactly the same name as the sketch** so it can be opened directly in Arduino IDE.

## Quick start

**Prerequisites:** two Arduino Uno boards, Arduino IDE, one USB connection per board or equivalent appropriate power, and the components listed above. The production sketches use the standard `Wire` library bundled with the Arduino AVR core; no third-party Arduino libraries are required.

1. **Disconnect the external pump/motor power supply.** Keep pressure relief available.
2. Wire the controllers: **Master A4 ↔ Slave A4**, **Master A5 ↔ Slave A5**, **Master GND ↔ Slave GND**. Use a common ground with the motor drivers. **Do not join the two USB-powered Arduino 5 V pins.**
3. Connect touch outputs to the **Master**: LEFT `OUT → D10`, RIGHT `OUT → D11`. Power compatible TTP223 boards from a regulated **5 V** rail, sharing GND with the Master.
4. Refer to [Hardware and pin assignments](docs/HARDWARE.md) for the L298N and eight pump connections. **Verify the actual motor supply voltage and current rating; do not assume 12 V.**
5. Open **`firmware/UTERUS_Slave/UTERUS_Slave.ino`**, select the Slave Uno and its serial port, and upload first.
6. Open **`firmware/UTERUS_Master/UTERUS_Master.ino`**, select the Master Uno and its serial port, and upload second.
7. Open the **Master Serial Monitor at 9600 baud**. Confirm `SLAVE FOUND: 0x08` and `UTERUS MASTER V17.4 READY`. Otherwise troubleshoot power and A4/A5/GND before connecting pump power.
8. **With pump power still disconnected**, verify sensor states and gesture commands. Then test one pump at a time under supervision before enabling the complete choreography.

**Important:** Uploading an `examples/` sketch overwrites the firmware currently on that Arduino. Restore both production sketches afterward.

## Interaction controls

| Gesture / command | Behavior |
| --- | --- |
| **LEFT:** touch and hold | All four bags **deflate** while held, up to **5 seconds**; release to stop |
| **RIGHT:** touch and hold | All four bags **inflate** while held, up to **5 seconds**; release to stop |
| **BOTH:** hold **2–5 seconds**, then release both | Start the **180-second autonomous sequence** |
| **BOTH:** hold **6 seconds** | Issue a **software stop** to all pumps |
| Master serial `C` / `D` | Timed fill / drain test, up to 5 seconds |
| Master serial `R` / `G` / `S` | Start choreography / disable Stage II growth bias / software stop |

TTP223 detection uses an **80 ms press** and **150 ms release** debounce. Manual actuation stops after five seconds even if contact continues; a new press requires a full release. A hardware-level cutoff is required independently of the six-second gesture.

For gesture transitions and expected outputs, see [Interaction model](docs/INTERACTION.md).

## Programmed breathing choreography

| Stage | Interval | Motion design |
| --- | --- | --- |
| I — Fetal Breath Initiation | `0–5 s` | Bag 2 inflates continuously while the others begin offset cycles |
| II — Uterine Expansion | `5–30 s` | Alternating pump cycles with additional inflation bias on Bags 2 and 4 |
| III — Uterine Wall Response | `30–60 s` | More varied inflation and suction intervals |
| IV — Fetal Breath Resonance | `60–170 s` | Interleaved five-second rhythms |
| V — Return to Stillness | `170–180 s` | Shorter programmed activation intervals, followed by pump stop |

The programmed cycle period is **5 seconds**, with **100 ms between inflation and suction** for each bag. Full pin-level timings are documented in [Choreography](docs/CHOREOGRAPHY.md).

**Figures and research use:** Plot these signals as *programmed pump activation*, **not** measured pressure, measured volume or biomedical breathing data. For a paper, include a clear caption identifying them as **control outputs derived from firmware**.

## Serial / I²C protocol

The Master sends a seven-byte frame to the Slave at `0x08` approximately every **50 ms**:

```text
[0xA5, mode, bag3_fill, bag3_drain, bag4_fill, bag4_drain, XOR]
```

`mode`: `0=stop`, `1=fill`, `2=drain`, `3=grow`, `4=hold`.
The checksum is the XOR of bytes `0–5`. The Slave stops its pumps if no valid frame has been processed for **400 ms** while active. See [Protocol](docs/PROTOCOL.md).

## Testing and verification status

- The sketches were reorganized from the project's existing **Master V17.4 / Slave V17.2** and its independent diagnostic programs; their defined pin mapping, timings, and protocol have been retained.
- Host-side syntax and behavior checks can be executed as described in [`tests/README.md`](tests/README.md), without connecting pumps.
- Host tests are **not** a substitute for Arduino Uno compilation or real hardware validation. Pressure, electrical and mechanical safety remain **unverified** by software tests.

## License and third-party materials

This repository's **original source code and documentation** are distributed under the [MIT License](LICENSE). The copyright holder line is currently a **project-contributors placeholder**: please replace it with the correct author or rights-holder name before publication.

**Not included / not automatically licensed:** photographs, medical or cadaveric imagery, third-party datasets, stock images, generated artwork, institution branding, physical hardware designs, or externally sourced materials. Obtain and document separate permissions where necessary.

## Citation

If this firmware informs a publication, cite this repository with the **actual repository URL, authors, version/tag and access date** once it has been published. Do not cite a placeholder URL.

## Safety

This is experimental installation software. Use properly rated supplies, common reference grounds with separated high-current wiring, physical power cutoffs, pressure relief and supervised tests. See [Safety and limitations](docs/SAFETY.md) before operation.

## Publishing to GitHub

See [Publishing guide](docs/PUBLISHING.md) for the pre-release checklist, GitHub Actions checks, author/rights-holder attribution and account-security recommendations. **Never share GitHub passwords, personal access tokens or recovery codes.**
