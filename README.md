# AuRA-1.3
## AuRA 1.3 File Development Machine

### READ THIS
This project is the next itteration of the AuRA Film Developing Machine.
Original machine was introduced on the Kickstarter back in 2021.
https://www.kickstarter.com/projects/vfalendysh/aura-rotary-assist-film-developing-machine

PLEASE NOTE: Everything provided AS IS, without any warranties, for non-commercial private use only. 
I'm trying my best to keep this project alive for the benefit of the Film Photography community.

## Bill Of Materials
https://docs.google.com/spreadsheets/d/1RUYtAMMg844CDN_oNOtQVQ9Nui_DjjbX51vlC8axlfM/edit?usp=sharing

## 3d Printer Settings
All parts should be printed with the following settings:
- 0.4mm nozzle
- 4 perimeters
- 60% geroid infill
- PETG plastic
- Colors you like
Please make sure that your printer is calibrated and prints with the hight dimmensional accuracy.
If rollers do not fit - try to print one by one resizing it by 0.2% increment.

## Firmware
There are few minor changes compared to v1.2: max dev. time now is 90 minutes, changed tanks captions, enabled demo mode by default.

Put folders / files from the "Firmware/libs" folder into your local Arduino/libraries folder.
Choose "Arduino Nano" board in Arduino IDE. Sometimes you may need to choose "old bootloader" verstion of the board.
Open "firmware/aura13.ino" file.
Compile / upload to the board.

If you have binary size issues - make sure that U8G2 library has size optimizations turned on. 
Check for more here: https://github.com/olikraus/u8g2/wiki/u8g2optimization


