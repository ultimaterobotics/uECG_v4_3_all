uECG device bootloader code with radio uploading capability.

Bootloader waits for 1.5 seconds after powering on uECG - if button is pressed within this time, it enters wireless update mode for 15 seconds (if code update wasn't started in this time frame, update is cancelled and unit proceeds to normal boot).

Wireless update requires base station with this capability (old base firmware doesn't support it). Update is safe in the sense that it can't overwrite the bootloader: if for any reason attemp wasn't successful, it can be repeated as many times as needed.

Bootloader runs code located at address 0x4000 so firmware should be built with FLASH (rx) : ORIGIN = 0x04000 in .ld file (bootloader ignores attempts to write at addresses below 0x4000).
