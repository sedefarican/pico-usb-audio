# Pico USB Audio + LED Activity

A project that turns your Raspberry Pi Pico 2 into a **USB audio device (speaker)** using the TinyUSB library. It also lights up an LED on GPIO2 whenever audio data is received, providing visual feedback for audio activity.

> Goal: Make your Raspberry Pi Pico 2 act as a **USB speaker** when connected to a computer and blink an LED with audio activity.

## Hardware Requirements

- Raspberry Pi Pico 2
- USB-C cable (data supported)
- 1x LED (preferably with resistor)
- GPIO2 connection
- (Optional) Audio output circuit or speaker


## Software Requirements

- [pico-sdk](https://github.com/raspberrypi/pico-sdk)
- [pico-extras](https://github.com/raspberrypi/pico-extras)
- [tinyusb](https://github.com/hathach/tinyusb)
- CMake ≥ 3.13
- Ninja Build
- Windows (or Linux/macOS)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm)

