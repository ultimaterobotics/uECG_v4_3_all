Base unit bootloader. Supports uploading code via USB - so once it's uploaded via STLink, further updates can be performed more easily. Runs code located at address 0x4000 - so requires firmware to be built with  FLASH (rx) : ORIGIN = 0x04000 in .ld file.

Bootloader waits for firmware update request for 2.5 seconds, then starts the code. Can't auto-reset the base on request (like Arduino) due to missing connection on the base station PCB - base was designed long before bootloader appeared in our plans :)


# bootloader notes

Creating a bootloader for nRF52x chips was our goal for a long while, yet for a long time it didn't work. Somehow we couldn't find any clear instructions how to make it - and it has 3 points of failure, so it's really complicated to make it by trial-and-error.
Eventually it did work though, so I will outline those points:

1. jump to the right address. Jump code must be handled by assembly, and jump itself is a simple BX command, a 5-lines ASM function can conveniently wrap all that's needed in this regard (in our implementation you can find it under the name start_code2(uint32_t start_addr)). But! If you assume that if program starts at 0x4000 then jump should be performed to 0x4000 - you are wrong! After compiling binary code, program's start point is storen in 2nd 32-bit word from the start. So bytes 4-7 of the .bin file define address where jump should be performed.

2. before making the jump, all interrupts must be disabled and VTOR register updated - without this, interrupts won't work. Our current level of understanding isn't good enough to properly explain details, basically VTOR tells where to look for interrup handling functions, and by default it points at bootloader's interrupts. After this interrupts should be enabled again (but not called) - so the program that would be launched has access to them. This piece is handled in our code by function binary_exec(uint32_t address)

3. target app must be compiled with proper origin address. First bytes of the flash memory are occupied by bootloader, so target app should start at higher address - and all function calls should know that it's not starting at zero. To do this, in .ld file an offset must be specified:   FLASH (rx) : ORIGIN = 0x04000 (if bootloader writes program code starting at 0x4000 and uses bytes 0x4004-0x4007 to determine jump point for starting the program.

Keeping these 3 points in mind you should be able to write your own bootloader without any trouble :)
