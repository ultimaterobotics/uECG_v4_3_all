# Main unit firmware

# uECG build and firmware upload process

We work in Ubuntu 18.04, although this should work with most Linux versions. We have no idea how to do this on Windows, sorry :)

## Prerequisites
   [Nordic SDK version 14.1](https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v14.x.x/nRF5_SDK_14.1.0_1dda907.zip) (both older and newer versions won’t work without config modification, although functionally it should be compatible with any version at all - we use only very basic SDK functions, no soft device)
   [Arm-none-eabi compiler version 8-2018-q4-major or higher](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
   [OpenOCD](https://sourceforge.net/projects/openocd/files/openocd/0.10.0/) version 0.10.0 or higher 
   STLink programmer (any clone should work just fine)

## Build process configuration
1. In Nodric SDK you need to write path to arm-none-eabi compiler tools, it is located in file components/toolchain/gcc/Makefile.posix
Its content should be following for given compiler version:
GNU_INSTALL_ROOT := /home/<your user folder>/<path to uncompressed compiler folder>/gcc-arm-none-eabi-8-2018-q4-major/bin/
GNU_VERSION := 8.2.1
GNU_PREFIX := arm-none-eabi
(if you use a different version, adjust folder name and version code accordingly)

2. After that, inside project folder, you need to adjust SDK path in makefile. Open project Makefile and put correct path in this line:
SDK_ROOT := /home/<your user folder>/<path to Nordic SDK>/nRF5_SDK_14.1.0_1dda907
(adjust SDK folder name accordingly if your version is different)

That’s it for build config. Now you need to run
`make`
in project folder and it should produce compiled .hex file.

## Upload process configuration
Download openocd. Unpack it to some folder (for this document we assume it is located in ~/openocd) and in the terminal, enter the following:

```sudo apt-get install make libtool pkg-config autoconf automake texinfo libusb-1.0-0-dev
cd ~/openocd
./configure --enable-stlink
make
sudo make install
```

## Uploading firmware
Connect the device to the programmer. Pins on uECG have text labels, check pinout of your particular STLink programmer for proper connections.

In the project folder (it’s important!) execute openocd command:

```cd ~/<your-project-folder>
openocd -f interface/stlink.cfg -f target/nrf52.cfg
```

If everything’s ok, command will not return to the shell, so open a new terminal window and enter:

`telnet 127.0.0.1 4444`

This will establish connection to OpenOCD via telnet. In telnet prompt type:

```halt
nrf5 mass_erase
reset
halt
flash write_image erase _build/nrf52832_xxaa.hex  (put relative path and name of .hex file)
reset
```

And that should be it!
