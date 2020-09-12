Base unit bootloader. Supports uploading code via USB - so once it's uploaded via STLink, further updates can be performed more easily. Runs code located at address 0x4000 - so requires firmware to be built with  FLASH (rx) : ORIGIN = 0x04000 in .ld file.

Bootloader waits for firmware update request for 2.5 seconds, then starts the code. Can't auto-reset the base on request (like Arduino) due to missing connection on the base station PCB - base was designed long before bootloader appeared in our plans :)
