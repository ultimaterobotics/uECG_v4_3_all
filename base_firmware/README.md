Base unit firmware, built for bootloader mode, won't work without it. If you want to make a no-bootloader version, edit ultimate_base_nrf52.ld file and replace line
  FLASH (rx) : ORIGIN = 0x04000, LENGTH = 0x80000
with
  FLASH (rx) : ORIGIN = 0x00000, LENGTH = 0x80000

This base version supports uploading uECG firmware via radio, when properly interfaced through USB (not documented yet, coming soon)
