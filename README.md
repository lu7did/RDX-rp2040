# RDX rp2040 Digital Transceiver (RDX_rp2040)

# Overview to Version 2.0 (Alpha)

A brief story of the project start with the excelent ADX Transceiver from Barb (WB2CBA) which can be found at

* Github site [link](http://www.github.com/WB2CBA/ADX).
* ADX transceiver blog [link](https://antrak.org.tr/blog/adx-arduino-digital-transceiver)

The ADX transceiver is powered by an Arduino Nano (ADX) or Arduino Uno (ADX_UNO) boards using both the 
ATMEL ATMEGA382p processor.

In order to leverage the capabilities of the transceiver with a powerful processor such as the Raspberry Pi Pico
which uses the rp2040 architecture this project was started.

Then a map between the Arduino board I/O and the rp2040 I/O was made showing some differences that need to be addressed
requiring additional circuitry, hardware modifications and firmware support.

Once the hardware platform was defined the firmware was ported using the ADX_UnO_V1.3 firmware as a baseline, the
porting didn't introduce any new feature or function, just the minimum number of changes to the code to accomodate
the architecture differences between both platforms. The firmware version emerging from that initial effort
can be found as the [ADX-rp2040](https://github.com/lu7did/ADX-rp2040/tree/master/src/ADX-rp2040) firmware version in this
site.		

Continuing with the roadmap of the project an experimental firmware able to operate as an autonomous transceiver by decoding and
generating FT8 signals **without** the usage of an external program such as WSJT-X was created, documentation of the
features on this project called **RDX**, standing for **Raspberry Pico Digital Transceiver**, are documented in this branch of the site.

```
*New in release 2.0 *

* Initial alpha release, for experimentation purposes only

build 20
* Automatic FT8 operation.

build 30
* TFT LCD 480x320 support.

build 40
* Autocalibration mode has been added check the appropriate section on how to enable and operate.
* NTP protocol based time synchronization.

build 50
* Web browser console (access to File System in Flash memory).

build 52
* Improvements in the FT8 protocol handling cycle.
* GUI development.

build 60
* ADIF logbook generation.
* USB ADIF logbook export.
* Serial configuration terminal.

build 70
* Multicore operation (core0 and core1)

build 72
* initial support for Si473x chipset
```

# Hardware

Same hardware than the supported by the ADX-rp2040 firmware,  with the following additions.


*	Audio amplifier (see modifications).
*	TFT LCD IL9488 480x320 board (see wiring). 
*	Si473x receiver chipset


# Firmware

## Binary distribution

Although not the preferred nor recommended mode of updating the firmware some specific levels might have a binary version (.uf2) available. It can be located at the
 ./bin directory.
The files are named using the following convention:
```
RDX-rp2040.ino.rpipico.uf2  for the Raspberry pico version
RDX-rp2040.ino.rpipicow.uf2 for the Raspberry pico Wireless version

The compilation occurs with specific feature and function configuration definitions, typically the documented in the .ino source file
if a different configuration is needed a full compilation needs to be performed

```
Please note the ./src directory might have binary files but would usually be intermediate development versions not recommended to flash as firmware, when using a 
binary distribution always pick it from the binary directory as it would be a more stable version.

### Procedure for Windows 10

In order to update the firmware the following procedure must be executed:

*	Connect the Raspberry Pico board with the PC USB port while pressing the **BOOTSEL** button.
*	Release the button once connected, a File Explorer window will appear.
*	Drag and drop the desired .uf2 file into that window. The file will be copied and shortly thereafter the Explorer window will be closed.
*	If no error message are shown the firmware has been updated.


## Build environment

Same build environment than the one used by the ADX-rp2040 firmware plus the additional conditions stated below.

### Arduino Pico Core

This firmware requires the [Arduino Pico Core](https://github.com/earlephilhower) created by Earle F. Philhower, III in order to setup a build chain
based on the Arduino IDE.
Check for installation and configuration instructions at this [link](https://www.upesy.com/blogs/tutorials/install-raspberry-pi-pico-on-arduino-ide-software).

### Arduino IDE board configuration

The following configuration applies for the Arduino IDE when using a Raspberry Pi Pico Wireless

![Alt Text](./docs/arduino_ide.png "Arduino IDE board configuration")


For the standard Raspberry Pi Pico (non Wireless) use the same parameters but selecting the Raspberry Pico board instead.


### Pre-requisites and libraries

Same build environment than the one used by the ADX-rp2040 firmware the latest version of the [Arduino pico core](https://github.com/earlephilhower/arduino-pico)
by Earle F. Philhower III  plus the following libraries:


*	[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) Library by Bodmer
*	[TFT_eWidget](https://github.com/Bodmer/TFT_eWidget) Library by Bodmer 
*	[MDNS_Generic](https://github.com/khoih-prog/MDNS_Generic) Library by Khoi Hoang.

```
Warning
The TFT_eSPI library requires the configuration of the TFT board in the **User_Setup.h* file, as the library support a 
large number of possible boards, each one with different modes the update of this file might be intimidating and 
error prone at first. A file already configured for the IL9488 board can be found in the **./ADX-rp2040/src/misc**
directory.
```


Code excerpts gathered from manyfold sources to recognize here, large pieces of code were extracted from former projects

* [ADX-rp2040](https://github.com/lu7did/ADX-rp2040).
* [PixiePi](https://github.com/lu7did/PixiePi).
* [Pixino](https://github.com/lu7did/Pixino).
* [OrangeThunder](https://github.com/lu7did/OrangeThunder).

## Code structure

### Overall FT8 decoding logic

The following UML graphic shows at high level the FT8 decoding cycle

![Alt Text](./docs/RDX-rp2040-FT8.png "FT8 decoding cycle")


### FT8 protocol finite state machine

The following UML graphic shows at high level the FT8 finite state machine controlling the behaviour of the 
transceiver during the FT8 QSO cycle.

![Alt Text](./docs/RDX-rp2040-FSM.png "FT8 protocol finite state machine")

## Multicore operation

In order to increase the performance of the transceiver the different sub-systems can be split between the two available CPU
cores provided by the rp2040 architecture (available on build 70 and up).
The sub-system high level logic can be seen in the following UML diagram:

![Alt Text](./docs/RDX-multicore.png "rp2040 Multicore high level logic")

Even if enabled by default being an experimental feature it can be switched off returning to the previous, single core,
operationg by commenting out the configuration line **#define MULTICORE 1**.

## Configuration Web server

Configuration parameters can also be changed using this facility, once activated it can be accessed using **http://rdx.local:8000"** or **"http://{ip_address}:8000.
![Alt Text](./docs/rdx_web.png "Web based configuration server")
Parameters can be modified and the update is made by pressing the button **update**, however the changes won't be made permanent until the **save** button is 
pressed and the changes made effective by writting them in EEPROM.
A facility to activate the transmitter can be used by pressing the **Tx+** button while pressing **Tx-** will turn the transmitter off.


## Configuration terminal

Most options which needs to be configured on a particular station can be modified at build time by correcting (or enabling, or disabling) the appropriate parameter
before compiling and flashing the firmware. Usually modifications needs to be done in the **RDX-rp2040.h** file.
However there are ocassions where the possibility to perform modifications is limited, either because the firmware was flashed from a pre-compiled version (.uf2 file)
or because the build environment isn't available or otherwise practical to be executed.
A number of parameters can be changed at run time using a built-in configuration terminal, this facility is activated if when the firmware is starting the
DOWN and TX buttons are found as pressed or by selecting the "HS" icon on the GUI, when executed a red banner shows the availability of the terminal at the GUI panel.
Either way of starting the terminal can be accessed using a serial port, with a suitable serial client such as Putty or minicom configured for the serial port used
by the USB Serial of the Raspberry Pico board.
This facility can be handy to change the operation parameters in response to an environment change, i.e. change the callsign, the grid locator, the credentials for 
the WiFi access point or other values.
When started the following banner is shown:

```
RDX 2.0 build(63) command interpreter
[16:51:59] >
```

A list of the available commands can be obtained by:

```
[16:51:59] >?
help list load save reset ? call grid adif ssid psk log msg host writelog autosend tx tcpport http ft8try ft8tx tz quit
[16:52:39] >
```
And a reduced help of the meaning of each command can be obtained by:
```
[16:52:39] >help
(help) - help for all commands
(list) - list EEPROM content
(load) - load EEPROM content
(save) - save EEPROM content
(reset) - reset EEPROM content to default
(?) - list all commands
(call) - station callsign
(grid) - station grid locator
(adif) - logbook name
(ssid) - WiFi AP SSID
(psk) - WiFi AP password
(log) - USB exported logbook
(msg) - FT8 ADIF message
(host) - host name
(writelog) - enable ADIF log write
(autosend) - enable FT8 QSO auto
(tx) - turn TX on/off
(tcpport) - Telnet Port
(http) - FS HTTP Port
(ft8try) - FT8 Max tries
(ft8tx) - FT8 Max tx
(tz) - TZ offset from UTC
(quit) - quit terminal mode
[16:53:36] >
```
Each command when executed without arguments will show the current value of the associated variable, if a valid argument is given the variable is replaced by the argument.

Some commands aren't related to variables but provided to execute directives such as:
```
load	Load the content of the EEPROM
save	Save current values of variables into EEPROM
reset	Reset EEPROM to default values
list	List contents of the EEPROM
tx	transmitter status
?	list of commands
help	Help on commands
quit	terminate session

```
Upon termination the board needs to be restarted for all changes to be made effective.



## Automatic calibration (autocalibration)

Starting on version 2.0 build(23) and higher a new capability to perform an automatic calibration of the Si5351 VFO has been added.

### Enabling

The firmware allows the automatic calibration of the Si5351 dds using the following procedures.

### Operation

When started the firmware will look during the setup stage if the **DOWN** pushbutton is pressed, if so all the on-board LEDs will
be lit with the exception of the TX LED indicating a waiting pattern, the autocalibration procedure will start as soon as the push
 button is released.

If the board is powered off before the push button is released the previous calibration stored in EEPROM (flash memory) will be reset
to zero.

The calibration can be monitored either by the LED pattern exhibited or thru the USB serial port (Arduino IDE Serial Monitor), once
the calibration is completed the results will be written in EEPROM (flash memory) as in the manual calibration in order to be used
on sucessive starting cycles. While the calibration is being performed the TX LED will blink once per second, the rest of the
board LEDs will mark how large is currently the difference in the calibration mode:

```
         WSPR,JS8,FT4,FT8 lit       error > 75 Hz
         WSPR,JS8,FT4     lit       error > 50 Hz
         WSPR,JS8         lit       error > 25 Hz
         WSPR             lit       error > 10 Hz
         All LED off                error < 10 Hz  (final convergence might take few seconds more)
```

When monitoring the calibration thru the USB Serial monitor the messages will look like:
```
Autocalibration procedure started
Current cal_factor=0
Current cal_factor=0, reset
Si5351 clock setup f 1000000 MHz
n(12) cal(1000000) Hz dds(1000071) Hz err (71) Hz factor(0)
n(12) cal(1000000) Hz dds(1000074) Hz err (74) Hz factor(500)
.............[many messages here]................
n(11) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(71500)
n(10) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(71500)
n(9) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(71500)
n(8) cal(1000000) Hz dds(1000002) Hz err (2) Hz factor(71500)
n(8) cal(1000000) Hz dds(1000000) Hz err (0) Hz factor(72000)
n(7) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(6) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(5) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(4) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(3) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(2) cal(1000000) Hz dds(1000001) Hz err (1) Hz factor(72000)
n(1) cal(1000000) Hz dds(1000000) Hz err (0) Hz factor(72000)
Calibration procedure completed cal_factor=72000
Turn power-off the ADX board to start

```

Upon finalization a message will be sent thru the serial monitor and the TX led will stop to  blink, the board power needs to be cycled
to restart the operation.

While the autocalibration is performed the progress is also indicated at the TFT LCD GUI.

*	The meter (upper right) will show progress in Hz difference.
*	The text scroll will exhibit progress messages.
*	The footer will be showing the label "AutoCal".

```
                                     *** Warning ***

Calibration total time might vary depending on the unique factory characteristics of the Si5351 chipset
being used. Upon finalization the power needs to be recycled for the board to restart.
```

## Time synchronization

To operate using FT8 the transmission and reception must be synchronized in time among all operator with a tolerance of less than 2 seconds. The rp2040 lacks a 
real time clock (RTC), it keeps track of the time quite precisely but starting from zero at the boot moment, which in turn might happen at any arbitrary time,
therefore rendering the board unusable for FT8 decoding and emitting pursposes.

There are several strategies to address this problem:

*	Using an external RTC board that can be synchronized with an external clock source.
*	Using a GPS receiver to synchronize the time.
*	Using the NTP protocol over the Internet to synchronize with a time server.
*	Some manual way to synchronize the time.

At this point the NTP protocol and manual synchronization has been adopted as viable strategies, however nothing in the hardware limit the future adoption of
other means such as GPS or RTC based synchronization.

### Manual time synchronization

The manual synchronization is the simplest and quickest to implement, it can be implemented on a minimum rp2040 configuration without any TCP/IP
connectivity, the later having a rp2040_W model as a pre-requisite.

With a manual synchronization Upon startup the rp2040 board starts it's internal clock is set arbitrarly to zero. 
However, if the **UP** button is found pressed while performing the initial firmware setup the processing is held (all LEDs blinking
signals that situation). The button can be held pressed until the top of the minute and when released the internal clock is set to 00:00:00 and therefore
left synchronized.

To operate FT8 the actual time isn't needed, other administrative pursposes such as a log might require that, but the protocol itself needs to identify within a 1 sec
precision the seconds 0,15,30 and 45 of each minute; once synchronized the internal clock is precise enough to do that.

The manual synchronization is volatile and therefore needs to be performed everytime the board is powered, but it can be done with any celular phone or other precise time
source (synchronized with a time server) where the second 00 of each minute can be precisely spot.

```
Warning

Although the internal clock is synchronized few microseconds after the release of the UP button the actual synchronization is an eye-hand coordination
that could take some hundred milliseconds up to over a second; in some cases the synchronization isn't good enough, and that can be seen as a difficulty
to properly decode signals or have a reduced sensitiviy to small signals. In that case the best cure is to repeat the synchronization.
However a simple method is to use a clock which actually is digital but has an analog format, when the seconds handle crosses the "1" of the "12" mark the button
must be released, this will account for some differences in the reaction time to do that and thus enhance the synchronization process. 
```

### NTP based synchronization

When provided with WiFi AP access credentials the firmware would attempt to connect to the Internet and synchronize the internal
clock automatically during the initial start up without any action from the operator.This requires a rp2040_w model though.
The Wifi access credentials can either be set within the code by defining at the RDX-rp2040.h file 
```
#define WIFI_SSID                  "Your WiFi SSID"
#define WIFI_PSK                   "0123456789"

```
Or including the same directive on a file called ap.h located in the same directory than the code when built.

### Time Zone

Time zone can be set by modifying the **#define TIMEZONE x** statement which is the amount of hours to be added or substracted to the UTC time provided by the
system clock. Without it the hour will be displayed as UTC.

The system clock once calibrated to be synchronized at the second 0/15/30 or 45 of the minute has no effect on the FT8 decoding.


## ADIF Logbook 

If the **#define ADIF 1** statements are included then a flash memory filesystem is configured and every QSO performed by the transceiver, either in
manual or automatic mode, is logged using the ADIF format into a file called **"/rdx.adif"**
In order for the flash memory based file system to be enabled the Tools/Flash Size IDE parameter must be set to **"2M Sketch 1948KB FS 64KB"**, this will create
a 64K flash memory based storage area managed using a simple file system. The capacity of the storage is very limited but enough to store about 100-sh FT8 contacts on it.
To recover, edit or erase the file you can use the Web Browser File System facility (FSBROWSER needs to be activated for that) or a USB Export (DATALOGGERUSB needs to 
be activated for that).

## Web File System Browser 

A facility called File System Browser can be activated at compile time thru the **#define FSBROWSER 1** in the RDX-rp2040.h file.
When activated the browser can be activated by simultaneously pressing **UP** and **DOWN** during the initizalization of the board, when activated a red spash screen
will notice that.
The web based file browser can be activated either by
```
http://{board IP address}/edit
http://rdx.local/edit
```
When accessed the GUI shown in the Web browser is as indicated by the following picture

![Alt Text](./docs/rdx_fsb.png "File System Browser")
Files can be edited, deleted, downloaded or moved using this facility.

Using this facility the files, can be more than one, could be edited, deleted or downloaded.

In order to de-activate the web based file browser the board needs to be re-initialized by cycling the power of it.

### TCP based terminal configuration tool

When the Web File System Browser is activated a TCP based terminal configuration tool is also available at port 9000 or otherwise configurated.
The commands and general "look-n-file" is the same than the one obtained with the serial based configuration tool.

## mDNS support
When TCP/IP is available, only the boards with rp2040-W are, and the function makes the TCP/IP connectivity to be ready the board can be reached by resolving
the symbolic name **rdx.local**
```
Warning
The mDNS resolution requires the client machine used for the access and the board to be in the same physical LAN.
```

## USB Export of logbook

A facility called USB export can be activated at compile time thru the **#define DATALOGGERUSB 1** directive in the RDX-rp2040.h file.
When available is can be activated by the USB export icon, when enabled a single file is exported thru the USB. The file can be browsed, edited, deleted or copied
to other place. The share finishes when the USB export icon is tapped again.
This facility is based on a feature of the Arduino pico core called **SingleFileDrive** where a single file is mapped, in this case the file mapped is the
**rdx.txt** used to store the ADIF logbook, the export name (the name used to be displayed in the host PC) will be **rdx-logbook.txt**.

### Windows 10

When activated thru the GUI a File Explorer window will be opened showing the PICODISK drive (drive letter will be assigned depending on the current drive configuration of the
machine); this USB drive will contain a single file named **rdx_toolbook.txt** containing the ADIF logbook. This file can be copied, moved, edited or erased like any file.

![Alt Text](./docs/RDX-USB.png "USB ADIF Logbook")


## GUI

The disposition of the LCD is just to make the hardware development more amenable, but it should be placed on top of the transceiver in some form of "sandwich" configuration.

![Alt Text](./docs/RDX-rp2040-GUI.jpg "RDX GUI Prototype")

The main areas of the GUI are:

*	Icons.
	Icons are meant to be used to activate or de-activate a given function such as WiFi, TCP/IP terminal, OTA firmware update, mount/extract a SD card, create an ADIF log and others.
	When the function isn't active it's shown as crossed (as they are most at this time).
	*	Active icons
		Active icons are shown as reversed between active and inactive state, in most functions where the FT8 decoding is
		stop while operating the function a red banner will appear over the waterfall showing the condition being activated.
		* **Time synchronization**. When tapped the firmware will attempt a time synchronization. No FT8 decoding activity will 
		  take place while performing the synchronization. Upon boot up this activity is performed automatically if the
                  WiFi AP credentials are provided. This function is enabled when the RP2040_W directive is enabled.
		  While performing the time synchronization a text message is placed in the waterfall area indicating that.
		* **Web based File System browser**. This mode is enabled by simultaneously pressing UP+DOWN at boot up, 
		  The tool can be accessed as *http://rdx.local/edit* . No FT8 decoding activity will take place while the web server is
		  active. Reboot the transceiver to resume normal operation. The RP2040_W and FSBROWSER directives are enabled.
		  While the browser is enabled a text message is placed in the waterfall area indicating that, no simultaneous FT8 operation
		  can take place while the browser is active.
		* **ADIF logging**. When tapped the firmware will generate an ADIF record for every QSO it is performed over the air,
		  the resulting file is named **/rdx.adif** and can be retrieved using the Web based File System browser. Beware that
 		  the file system has very limited space resources and therefore no large files can be handled (see below for further information).
		* **USB export** When tapped the firmware will enable a "single file USB data export" with the ADIF logger content,
		  the data can be edited, copied out or deleted. No logging will occur while the export is active. Tapping the icon alternatively
		  will enable and disable the export. Log file will be exported as **rdx_logbook.txt**.
		* **Configuration terminal (HS)** When tapped the serial configuration terminal is opened, access to the configuration menu and 
		  functions can be made using a terminal program configured for the USB serial port of the board. 
		* **Web based configuration tool** When tapped a connection with the configured WiFi AP is started and an internal web server initiated,
		  using it status information is shown and configuration parameters can be changed.
		* **FT8 QSO reset**. When tapped the firmware will reset the current QSO status back to idle, effectively cancelling it. Even if enabled
		  no log will be generated.
		* **MIC** (not used, future use)
		* **SPEAKER^** (not used, future use)
		* **Warning** This icon isn't activable and provides a visual clue that the tap of an icon has been registered and it's pending for
		  execution. The icon actions can only be processed at the end of the FT8 cycle. Once the icon activation is performed the icon
		  is placed in off condition.

```
Warning

When creating an ADIF file precise date and time are needed, therefore at build time this option is protected to be available
only when the RP2040_W, FSBROWSER and ADIF directives are defined.

```
```
Warning

Because of internal timing considerations the GUI can be activated at any time but the effects of the actions selected will be
operative at the end of each FT8 cycle.

```

* 	Meter.
	The meter is meant to display signal strenght (S-Units), power (in Watts), SWR or rx level. At this point only the S-meter is implemented to show a level proportional
	to the energy in the passband, it will be calibrated approximately to S-units.
* 	Display area.
	The display area shows several controls.
	*	**Buttons**.
		There are four buttons.
		*	**TX**
			When touched it will activate the transmission (similar to press the hardware TX button) and show as inverse, reversing it when touched again. 
			It will also inverse if the board is placed in transmission mode by the firmware or when the TX is activated by pressing the TX button.
		*	**CQ/Call**
			When touched will inverse and start sending CQ calls, eventually answering them and performing one full automated QSO until touched again (when the
			Manual/Auto control is in Manual). When selecting a particular CQ call from the text area it will be shown as "Call" while the QSO is attempted.
		*	**Manual/Auto**
			When in Manual the firmware will call CQ when pressing the CQ button or will answer a call if selected from the text display, in Auto mode it will
			call CQ periodically and attempt to answer the first CQ call heard.
		*	**Band**
			Shows the current band, the firmware support the 40,30,20 and 10 meters band, it will circulate amont them by pressing the button. Tha band change can 
			also be made by the standard ADX hardware procedure and changes made this way reflected in the value of the button. Also changes in the band performed
			by the cursors will be reflected.
	*	**Cursors**.
		The left cursor will decrease the current band and the right cursor increase it. Changes made will be reflected in the board LED and in the Band button.
	*	**Frequency display**.
		The frequency display will reflect the standard FT8 frequency of the selected band.
* 	Text area.
	This area will reflect several QSO lines using a color scheme to identify the type of it.
	*	Black on White. 3rd party QSO.
	*	Black on Yellow, CQ from our station or answering to another station.
	*	White on Red, QSO in progress.
	*	Black on Green, CQ from another station.
	When a CQ call from other station is selected by the pencil the transceiver is placed in "Call" mode and an attempt to perform a QSO is made.
* 	Waterfall.
	This area will show a waterfall representation of the passband updated every second.
* 	Footer.
	This area will show configuration information such as firmware level, callsign, grid locator, time and IP address. The time reflects the actual internal clock,
	either if it is synchronized by some means or not. A timezone correction is applied if defined. The IP address shows the assignment made by the local AP thru
	DHCP or "Disconnected" if not connected.



```
Warning

The band settings on the firmware needs to be made consistent by using the profer filter on the board and antenna as there is no way for the firmware
to validate neither the proper filter nor a reasonable SWR level when the TX is activated.
```


# Hardware

The hardware required by this transceiver derives directly from the ADX Transceiver (WB2CBA), the implementation can take basically two forms:

* Build a hand wired version of the circuit.
* Build an ADX transceiver and replace the Arduino Nano with the ADX2PDX daughter board created by Barb (WB2CBA), see below.

## ADX_rp2040 circuit

The circuit used is esentially the ADX transceiver with the minimum set of modifications to accomodate a Raspberry pico (rp2040 processor) instead of an
 Arduino Nano (ATMEGA328p processor).
The following diagram has been originally conceived by Dhiru (VU3CER) and put together by Barb (WB2CBA):

![Alt Text](./docs/PDX_V1.0_Schematic.jpg "PDX Schematic")
PDX_V1.0_Schematic.jpg

Check additionally mods required and TFT support requirements detailed below.

The receiver, Si5351 clock, RF driver and final stages are identical to the standard ADX Transceiver, whilst changes are made around the rp2040 processor to
accomodate the different signaling and voltages used.

### rp2040 pinout assignment

Same as the ADX-rp2040 project
![Alt Text](./docs/rp2040_pinout.jpg "rp2040 pinout")

### Power supply

Same as the ADX-rp2040 project

### Receiver

The receiver sub-system is identical than the ADX Transceiver.

### SWR protection

A Zener Diode (D10,1N4756) located where the board TP3 is defined would prevent a situation of high SWR to damage the finals.

### RF Power 

The RF power (driver and finals) is identical than the ADX Transceiver.

### Low Pass Filter

The Low Pass Filter (actually more than that) is needed to suppress unwanted spurious responses and also to achieve high efficiency class E operation.
The design is identical than the ADX Transceiver.


### ADX2PDX daughter board

Same as the ADX-rp2040 project


## RDX-rp2040 modifications

The following modifications applies to both the schematic of the RDX transceiver or the ADX2PDX daughterboard.
```

 * Build a small class A audio amplifier.
 * Connect the input of the amplifier to RXA.
 * Connect the output of the amplifier to GPIO26 (ADC0) pin 31 of the rp2040 board.

```
A suitable circuit can be seen in the following schematic

![Alt Text](./docs/RDX-rp2040-AF.png "Audio amplifier")

## Si473x Chipset support

An initial support for the Si473x chipset can be included by enabling the initializacion thru the **#include RX_SI4735  1** 

	*	During initialization a search in the I2C bus is made looking for a Si473x chip, if found the SSB patch is loaded and the chip is initialized
		with the current band parameters.
	*	On each band change the chip is reset and the new band parameters is loaded.

At this point there is no dynamic control of the Si473x chip parameters, only parameters changed thru build time. 
 
## TFT LCD display support

The firmware supports a TFT LCD IL9488 480x320 display where a GUI is presented allowing the operation of the transceiver, the LCD is optional
but it greatly enhances the autonomous FT8 operation allowing to see the activity on the channel and operate either to call CQ or answer to
an on-going call.

The actual wiring of the TFT board needs to connect the pinout to the Raspberry Pico (rp2040) processor as indicated in the following
diagram:

![Alt Text](./docs/RDX-rp2040-TFT.png "TFT LCD wiring")

```
* Warning *
Attempts to use TFT LCD display other than the IL9488 shows a performance degradation which conflicts with the ability to properly
decode FT8 signals. The problem has been traced back into a slow response to the pen position reading, which is performed every
few hundred millisecs. An special build directive "#define IL9488 1" informs the firmware it's running with the required display,
but if not informed (i.e. commented out or removed) the reading of the pen is avoided during the FT8 decoding cycle and performed
only during the few fractions of a second remaining after the decoding of a cycle. In this way the response will be much slower 
but still allow some manual functionality to be preserved, therefore it's a mitigation of an issue derived from not having the
specified hardware.
```

## ADX2PDX daughter board prototype fixes

Same as the ADX-rp2040 project

## ADX2PDX PCB

Same as the ADX-rp2040 project


# Testing

Only preliminar testing has been performed as it is just an alpha version of the firmware for preliminary evaluation purposes,
functions will be tested as the implementation evolves.

# Informal roadmap

This is the informal roadmap followed to prioritize and implement the future features of the project

## Pending

* Develop or adopt a PCB layout design.
* Support for Si4735 chipset based receiver
* Hardware interface to SD-Card/Export
* Organize and add functionality for icons (partial)

## low priority roadmap

* Support for smaller display (partial)
* CAT support (TS480).
* WSPR beacon.
* Support for QUAD multifilter board
* CW operation (basic, emergency)
* GPS support (time alignment & grid definition)
* Support for ATU reset
* SWR indicator & control (as HW support is introduced)
* Filter support (as HW support is introduced)

## rp2040-w specific

* OTA firmware update


## Done (as per V2.0 build 70)
* Support for smaller display (partial)
* File system (SD card based and USB based)
* Migrate signal processing to core1 (strategy to the DMA error).
* Performance issues (DMA error 0x04) with slower TFT boards
* TCP based terminal configuration tool.
* Explore overclock options (strategy to the DMA error).
* Web based configuration tool
* Organize and add functionality for icons (partial)
* Export/Import file feature
* Configuration terminal
* Basic transceiver operation (manual and auto mode).
* File system USB export
* USB based file system
* WiFi support
* File system (Flash based)
* ADIF generation
* NTP support and clock alignment
* mDNS implementation (rdx.local resolution)
* Web based ADIF export tool
* Organize and add functionality for icons
* Port automatic calibration from ADX-rp2040
* progress bar for RX/TX (green/red)
* display dialog multiband
* integrate meter (S-Meter and Power)
* document ft8 FSM (UML)
* include multiband support
* integrate scroll text 
* improve ft8 FSM (organize)
* Manual/Auto control
* CQ control
* TX control
