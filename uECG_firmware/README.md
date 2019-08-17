# Main unit firmware

Firmware is intended to be compiled under Linux with Nordic SDK v 14.1.0 and arm-none-eabi-gcc compiler. Although nothing platform-specific is used in the code, so if you know how to configure the build process for your system - you should have no problems adapting it.

No softdevice is used. Interrupts and computations put significant load on the MCU, so adapting it to use with softdevice might be problematic.

