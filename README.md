# Magic Cocktail Machine

A complete Raspberry Pi + STM32 project to automate three peristaltic pumps for cocktail preparation.

## Architecture Overview

```
Browser → Flask Web UI (Raspberry Pi) → UART (USB) → STM32 HAL Firmware → Pump Drivers → Pumps
```

* **Raspberry Pi** hosts a Flask application (`raspberry_pi/app.py`). Users pick a cocktail recipe from any device and the Pi translates recipe proportions into timed pump commands.
* **STM32** executes the commands received over UART (`stm32/Core/Src/main.c`), driving GPIOs connected to the pump power stages.

## Communication Protocol

* Transport: USB CDC (virtual COM port) exposed by the STM32 as `/dev/ttyACM0` (default).
* Command format: ASCII `<pump_id>:<duration_ms>\n`.
* Pumps identifiers: `1`, `2`, `3` (mapped to PA1, PA4, PA5 respectively).
* The STM32 replies with `OK` or `ERR:<reason>` for each command.

Example flow for a Mojito (40% / 30% / 30% over 9 s total):

1. Flask computes pour durations: `pump1 → 3.6 s`, `pump2 → 2.7 s`, `pump3 → 2.7 s`.
2. Flask sends sequential commands: `1:3600`, `2:2700`, `3:2700`.
3. STM32 activates pumps accordingly via HAL GPIO control.

## Repository Layout

```
raspberry_pi/    Flask application, templates and usage guide
stm32/           HAL-based firmware for UART controlled pumps
```

## Getting Started

### Raspberry Pi

Follow `raspberry_pi/README.md` to install dependencies (`Flask`, `pyserial`) and start the web server.

### STM32 Firmware

Use the files under `stm32/Core` inside an STM32CubeIDE project targeting your board. Detailed steps are in `stm32/README.md`.

## Extending the System

* Add new cocktails by editing the `RECIPES` dictionary in `raspberry_pi/app.py`.
* Adjust total pour time (`TOTAL_POUR_DURATION_S`) or pump mappings (`PUMP_IDS`) as needed.
* Expand the UART protocol to include pump priming, pause/resume or feedback messages.

## Safety Considerations

* Use dedicated MOSFET or relay drivers to interface pumps with the STM32 GPIO outputs.
* Provide backflow valves and tubing rated for your liquids.
* Clean the system regularly to avoid contamination.
