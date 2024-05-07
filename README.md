# AuRA 1.3 Rotary Assist Film Developing Machine

## READ THIS
This project is the next itteration of the AuRA Film Developing Machine.
Original machine was introduced on the Kickstarter back in 2021.
https://www.kickstarter.com/projects/vfalendysh/aura-rotary-assist-film-developing-machine

PLEASE NOTE: Everything provided AS IS, without any warranties, for non-commercial private use only. 
I'm trying my best to keep this project alive for the benefit of the Film Photography community.

## Manuals
### Operations Manual
https://docs.google.com/document/d/11cLEZ-TlUOUsFuLOAkJhnijDMngBIQ9VhE78JiG_ncQ/edit?usp=sharing

## Questions?
Please ask here: https://www.facebook.com/groups/195994281887859

## Changes between 1.2 and 1.3
### Firmware
- Max dev. time increased to 90 minutes.
- Changed tanks captions in "Settings" menu.
- Enabled demo mode by default.

### Mechanical
- Main ball bearings are moved on the outside of the constaining bodies and now fixed on both sides.
- No need for retaining rings (no lathe work required) - replaced with shaft collars.
- Increased main case size a little bit for better access.
- Boards are now easily removable if needed - they are placed on the platform and fixed with 2 screws only.
- No USB port on a side (may add latter uppon request).

## Bill Of Materials
Most of the componets can be purchase on Amazon or AliExpress. 

https://docs.google.com/spreadsheets/d/1RUYtAMMg844CDN_oNOtQVQ9Nui_DjjbX51vlC8axlfM/edit?usp=sharing

Please let me know if something is missing from the BOM or you have sourcing questions.

## 3d Printer Settings
All parts should be printed with the following settings:
- 0.4mm nozzle
- 4 perimeters
- 0.2mm layer height
- 5 top/bottom shells
- 60% geroid infill
- PETG plastic
- Colors you like
  
Please make sure that your printer is calibrated and prints with the hight dimmensional accuracy.
If rollers do not fit - try to print one by one resizing it by 0.2% increment.

## Firmware
- Put folders / files from the "Firmware/libs" folder into your local Arduino/libraries folder.
- Choose "Arduino Nano" board in Arduino IDE. Sometimes you may need to choose "old bootloader" verstion of the board.
- Open "firmware/aura13.ino" file.
- Compile / upload to the board.

If you have binary size issues - make sure that U8G2 library has size optimizations turned on. 
Check for more here: https://github.com/olikraus/u8g2/wiki/u8g2optimization

## Electronics
Wire / assembly diagram - WIP.


