# Safety and Limitations

> **Experimental interactive artwork only. Not a medical device. Not suitable for use on or in the human body, and not a patient-connected system.**

This repository controls eight physical pumps and four inflatable air bags. Incorrect wiring, unexpected actuation, electrical faults, or overinflation can cause injury or property damage. The firmware has **not** been evaluated against functional-safety standards.

## Before powering the pumps

1. Check the pumps' rated **voltage and startup/stall current**, wiring gauge, driver thermal limits, and power supply protection. L298N modules can introduce substantial voltage drop and heat.
2. Fit a readily accessible **physical emergency power cutoff** independent of Arduino software and the touch sensors.
3. Incorporate an appropriate **pressure-relief or venting path** and ensure the air bags, tubes and connections tolerate the actual loads.
4. Use a dedicated pump supply; never feed eight pump motors from the Uno's `5V` pin or a breadboard.
5. Share logic reference ground without routing motor-return currents through breadboard rails or Arduino headers.
6. Verify all four bags' inflation and suction directions using **one-pump-at-a-time tests** and an open/low-pressure configuration.
7. Keep the installation supervised and within a safely controlled, isolated exhibition environment.

## Software protection is not a safety system

- The **5-second maximum manual actuation** is a time limit, not a measured pressure/volume limit.
- The **6-second dual-hand software stop** is deliberately slow and must **never** be treated as an emergency stop.
- The Slave's **400 ms watchdog** only responds to valid-frame loss; it does not monitor motor temperature, pressure, leaks, stuck relays, open circuits or supply faults.
- No pressure sensor, flow sensor or feedback regulator is present in the documented firmware.
- A 180-second show may include multiple full-power pump cycles and must not be considered safe without physical validation.
- TTP223 electrodes may trigger unexpectedly because of noise, moisture, grounding changes, large metal plates or power-up calibration.

## Academic / exhibition transparency

Graph these values as **programmed pump actuation**, not physiological breathing or measured pressure. Use "womb-inspired" as an **artistic metaphor**, not anatomical equivalence. Any third-party imagery or anatomical sources require independent citation, permission and ethics checks.

The code is provided **AS IS** under MIT; the license disclaimer is not a substitute for safe hardware engineering.
