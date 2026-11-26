# Interaction Logic

The current installation uses two TTP223 sensors as **binary touch inputs**. Its interaction is **time-based**, not pressure-responsive.

| Input | Interpretation | Actuation |
| --- | --- | --- |
| Left only (hold) | Participant withdraws / releases air | All bags: suction while held; max 5 s |
| Right only (hold) | Participant expands / introduces air | All bags: inflation while held; max 5 s |
| Both held for **2–5 s**, then both released | Switch from direct agency to a programmed sequence | Start five-stage, 180 s choreography |
| Both held for **6 s** | Request to stop | Software command to stop all pumps |
| One-finger release after a BOTH gesture | Continue tracking the BOTH interaction | No accidental single-hand restart |
| Touch longer than 5 s | Manual software timeout | Stop; requires release and another touch |

## Gesture timing

- Press debounce: **80 ms**.
- Release debounce: **150 ms**.
- The BOTH timer starts after **both channels have debounced**.
- The BOTH activation duration is measured up to the first finger's release, but the show starts **only when both have released**.
- A BOTH hold of 5–6 s does not trigger the show or timed stop; at 6 s the stop is issued.
- The operator can send `S` over the Master serial monitor (9600 baud). To avoid immediate manual restart after `S`, the firmware waits until all sensors are released before accepting another touch.

## Technical limitation

The code is not a force, pressure, proximity-distance or gesture-classification model. Outputs are binary on/off and debounced, controlling full PWM (`255`) in the current choreography. Movement depends on physical pump dynamics, airflow and bag compliance, which are **not measured**.

## Artistic interpretation

The installation uses touch as a metaphor for intention and a shift from direct to autonomous motion. "Womb-inspired" and "breathing" describe a sculptural interaction; they should not be presented as clinically validated uterine or fetal mechanics.
