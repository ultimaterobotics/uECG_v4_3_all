uECG device bootloader code with radio uploading capability.

Bootloader waits for 1.5 seconds after powering on uECG - if button is pressed within this time, it enters wireless update mode for 15 seconds (if code update wasn't started in this time frame, update is cancelled and unit proceeds to normal boot).

Wireless update requires base station with this capability (old base firmware doesn't support it). Update is safe in the sense that it can't overwrite the bootloader: if for any reason attemp wasn't successful, it can be repeated as many times as needed.

Bootloader runs code located at address 0x4000 so firmware should be built with FLASH (rx) : ORIGIN = 0x04000 in .ld file (bootloader ignores attempts to write at addresses below 0x4000).

## bootloader notes

General part (a copy of notes from USB bootloader):

Creating a bootloader for nRF52x chips was our goal for a long while, yet for a long time it didn't work. Somehow we couldn't find any clear instructions how to make it - and it has 3 points of failure, so it's really complicated to make it by trial-and-error. Eventually it did work though, so I will outline those points:

jump to the right address. Jump code must be handled by assembly, and jump itself is a simple BX command, a 5-lines ASM function can conveniently wrap all that's needed in this regard (in our implementation you can find it under the name start_code2(uint32_t start_addr)). But! If you assume that if program starts at 0x4000 then jump should be performed to 0x4000 - you are wrong! After compiling binary code, program's start point is storen in 2nd 32-bit word from the start. So bytes 4-7 of the .bin file define address where jump should be performed.

before making the jump, all interrupts must be disabled and VTOR register updated - without this, interrupts won't work. Our current level of understanding isn't good enough to properly explain details, basically VTOR tells where to look for interrup handling functions, and by default it points at bootloader's interrupts. After this interrupts should be enabled again (but not called) - so the program that would be launched has access to them. This piece is handled in our code by function binary_exec(uint32_t address)

target app must be compiled with proper origin address. First bytes of the flash memory are occupied by bootloader, so target app should start at higher address - and all function calls should know that it's not starting at zero. To do this, in .ld file an offset must be specified: FLASH (rx) : ORIGIN = 0x04000 (if bootloader writes program code starting at 0x4000 and uses bytes 0x4004-0x4007 to determine jump point for starting the program.

Keeping these 3 points in mind you should be able to write your own bootloader without any trouble :)

### Radio specific part:

Radio packets arrive corrupted or completely missing way too often to write them directly on flash. In our implementation each packet has 3 checksums (total, odd, every 3rd byte) and sends confirmation after every packet (if base didn't receive confirmation, it re-sends packet, so we also need to handle duplicate packets). But when all of it is done properly, the process seems to be quite reliable - it passed all tests with moving unit out of range, keeping it at max range (so less than half packets can get through), cutting power in the middle - so it looks ok for now, even though a bit slow (unnecessarily slow, but rewriting it requires time we need to move other parts of the project - will do that later, as always :) )
