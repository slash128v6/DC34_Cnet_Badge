# DC34 CompuNet Badge - Malware Scan Tool 3000


<img width="1545" height="1999" alt="Join CompuNet   Critical Start at Defcon!" src="https://github.com/user-attachments/assets/d39828cd-9f8d-4c1f-91cb-b384a767c6a0" />



> [!TIP]
> **THIS SPACE FOR LEASE - CALL 702-263-9744 **

# Section 1: Hardware Assembly

1.	Verify inventory:

| Qty	| Description |
|------|-------------|
|1	|PCB |
|1	|ESP32 Dev Module |
|1	|Round LCD Display |
|1	|RFID Reader |
|1	|Micro SD Card Reader |
|1	|Micro SD Card |
|1	|Slide Switch |
|1	|Schottky Diode |
|1	|RGB LED |
|1	|6 Pin SAO Female Header |
|1	|Battery Holder |
|4	|AAA Batteries |
|1	|Foam Tape |
|1  |3d Printed Stand |
|5  |3d Printed "Slides" |
|5  |RFID Sticker Tags |

![](images/kit.jpg)

2.	If you haven't, please download the repo for documentation and code:

[https://github.com/slash128v6/DC34_Cnet_Badge](https://github.com/slash128v6/DC34_Cnet_Badge)


3.	Bend the legs on the Schottky diode and solder on the back side of the PCB. Note the polarity band. Trim the leads on the front of the PCB.

![](images/Picture1.jpg)

![](images/Picture2.jpg)

4.	Solder the slide switch on the back of the PCB

5.	Solder the short side of the pin header to the Micro SD Card Reader as shown:

6.	Solder the Micro SD Card Reader to the back of the PCB as shown:

7.	Solder the short side of the pin header to the RFID Reader as shown:

8.  Solder the RFID Reader to the back of the PCB as shown:

9.  Solder the ESP32 Dev Module to the back of the PCB noting the USB port is at the bottom:

10.	Solder the 6 Pin SAO Header to the front of the PCB with the bump facing upward as shown:

11.	Solder the Round LCD Display to the front of the PCB:

12.	Solder the RGB LED “stage light” through the front of the PCB with the shorter leg into the square hole and bend towards the microscope stage as shown and trim the excess leads from the back to avoid clothing snags:

13. Use the double-sided foam tape to attach the 4x AAA battery holder to the rear of the PCB and solder the leads red to (+) and black to (-):

14.	Attach the RFID Tag Stickers in the cavity of the 3D Printed Slides:

15.	Copy the SD Card Contents from the GitHub repository folder to the Micro SD Card (ignore the System Volume Information folder):

16. Insert the Micro SD Card into the Micro SD Card Reader with the label facing the front of the badge:

17.	Locate the ESP32 Driver installer in the DC34_Cnet_Badge\Software\ESP32 Driver\CP210x_VCP_Windows\CP210x_VCP_Windows folder. Right-click the CP210xVCPInstaller_x64.exe driver installer and “Run as administrator”:

18. Open the DC34_Cnet_Badge.ino file in the DC34_Cnet_Badge\Software\DC34_Cnet_Badge folder and open in Arduino IDE:

19.	Add the ESP32 Board URL in Arduino Preferences:

https://espressif.github.io/arduino-esp32/package_esp32_index.json

20.	Install “esp32 by Espressif Systems” in Boards Manager:

21. 21)	Install the following libraries in Library Manager
a.	Adafruit GFX Library by Adafruit
b.	Adafruit GC9A01A by Adafruit
c.	AnimatedGIF by Larry Bank
d.	MFRC522 by GitHubCommunity
e.	SD by Arduino, Sparkfun
f.	Adafruit SPIFlash by Adafruit

22. Set the switch on the back of the badge to “USB”. Connect a USB-C cable to the ESP32 module on the back of the badge to your computer. In Arduino IDE select “ESP32 Dev Module – esp32” on the detected COM port:

23. Click the Upload button to flash the badge:

24. If all went well then you should see the badge logo on the Round LCD Display:

25. “Scan” the slides (or other RFID tags, hotel key cards, have fun!) on the sample area on the front of the badge and watch the display. There are several more images than the 5 slides that were included so share and trade with your friends to unlock all the images!

26. Attach a lanyard if you want to wear it:

27. Or slide the badge into the stand if you want to display it:

# SAO

1. 1)	Your SAO badge kit should contain the following components:

| Qty	| Description |
|------|-------------|
|1	|PCB |
|1	|6 Pin SAO Male Header |
|4	|RGB LEDs|

2.	Solder the 6 Pin SAO Male Header to the back of the PCB with the notch matching the outline on the PCB:

3. 	Solder the RGB LEDs through the front of the PCB with the short leads in the square holes and trim the leads on the back:

4. 	Connect to the 6 Pin SAO Female Header on the main badge and the LEDs should light up!


