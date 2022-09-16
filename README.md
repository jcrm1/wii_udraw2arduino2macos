# wii_udraw2arduino2macos
Use the Wii uDraw GameTablet as a digitizer/graphics tablet on your macOS device!  

Emulates the basics of the WACOM IV serial protocol on an Arduino.  

I used a cheap adapter meant for Wii Nunchuks to connect my board to GND, 5V, SDA, and SCL of the tablet.  

Requires [TabletMagic](https://github.com/thinkyhead/TabletMagic)

Arduino sketch was modified from [mwvent/wii_udraw2arduino2linux](https://github.com/mwvent/wii_udraw2arduino2linux)  

Last, note that although this sketch was written for use on a Teensy++ 2.0, it should work on all Arduino boards that support serial over USB and I2C.
