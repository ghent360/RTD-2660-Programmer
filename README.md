Realtek RTD2660/2662 programmer
===============================

##Programmer for LCD controllers based on the RTD2660 or RTD2662 chips.

To build the programmer you would need a basic FX2LP device. One from amazon or ebay based on CY7C68013A would do. Install the FX2LP SDK from Cypress and flash swd.iic from the FX2LP folder on the FX2LP device.

Connct:
```
    FX2LP       VGA Port
    Device      Target
    ------      ---------------
    PA0    ---> Pin 15 DDC SCL
    PA1    ---> Pin 12 DDC SDA
    GND    ---> GND (choose any of the white ports, not the VGA connector itself)
```
You would need to power the target board from a 12V or 5V power supply.

Use the PC software to flash a .bin file to the target or to read the content of the existing flash. Saving a copy of the existing software is reccomended.

Building the software:

There is a windows PC project for visual studio 2010. Multiplatform support is pending.

The device software needs the Keil PK51 toolchain. There is a compiled binary i2c.iic you can upload to the FX2LP device as well.