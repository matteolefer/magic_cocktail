# STM32 Firmware

Firmware for an STM32F4 microcontroller that listens on UART for pump commands coming from the Raspberry Pi.

## Hardware Assumptions

* MCU: STM32F4 series (tested on Nucleo-F401RE).
* UART interface: USART2 mapped to the ST-LINK virtual COM port (PA2 = TX, PA3 = RX).
* Pumps connected to GPIOA pins:
  * PA1 → Pump 1
  * PA4 → Pump 2
  * PA5 → Pump 3
  * PA9 → Status LED (optional visual feedback)

Adapt pin mappings in `Core/Inc/main.h` if your board differs.

## Build Instructions (STM32CubeIDE)

1. Create a new STM32CubeIDE project for your target MCU.
2. Replace the generated `Core/Inc/main.h` and `Core/Src/main.c` files with the ones from this repository.
3. Ensure the HAL drivers for GPIO and USART are enabled.
4. Build and flash the project to the board.

## UART Protocol

The Raspberry Pi sends ASCII commands terminated by `\n`.

```
<pump_id>:<duration_ms>\n
```

Examples:

* `1:4000` → run pump 1 for 4 seconds.
* `2:3000` → run pump 2 for 3 seconds.

The firmware acknowledges each successful command with `OK` and errors with `ERR:<reason>`.

## Safety Notes

* Pumps should be powered via appropriate MOSFET or relay drivers. The GPIO pins must **not** drive the pumps directly.
* Always test with water before using real ingredients.
