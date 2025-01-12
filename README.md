# Using HSPI and VSPI on ESP32: Interfacing RC522 and NRF24L01 with ESP8266 Communication

## 1. Introduction
I think this is a very niche problem, but I encountered it during my final year B.Tech project, which is based on V2X communication. I need to interface both the NRF24L01 module and the RC522 module to an ESP32. However, the problem is that both modules use the SPI protocol for communication. The ESP32 has two hardware SPI buses: HSPI and VSPI.

By default, most libraries use the VSPI bus. This is the case with the MFRC522 and RF24Network libraries as well. You can see the issue, right? If we interface both modules together, conflicts arise as they both try to use the same VSPI bus by default for communication.

One potential solution is assigning different CS pins to the two modules and activating them only when necessary (_ignore this: pakshe athil oru thrill illa_). However, for my use case, I need both modules to be active at all times. While it might be possible to use only the VSPI bus to achieve this, I’m unsure how to do it.

Here’s the plan: I’ll interface the NRF24L01 and the RC522 RFID module with the ESP32. The NRF24L01 will use the default VSPI bus, while the RC522 will use the second SPI bus, HSPI. To make this work, we need to modify the MFRC522 library files to use the HSPI bus instead of VSPI.

For the receiver, I’ll use an ESP8266 (you can use another ESP32 if you prefer, but I only have an ESP8266 available). This ESP8266 will be interfaced with the NRF24L01 module. Whenever a tag is placed on the RC522 reader, the ESP32 will query the ESP8266 (assuming it represents a vehicle) about its intended direction and speed. The ESP8266 will respond with the relevant data. The ESP32 will then calculate a priority and communicate it back to the ESP8266.

I’ll also demonstrate how to work with both the ESP32 and ESP8266 simultaneously using PlatformIO. This includes compiling and uploading code to both devices at the same time and monitoring multiple serial ports simultaneously.

For this project, I’ll be using PlatformIO with Visual Studio Code instead of the Arduino IDE.

Let’s get started!

## 2. Hardware Connections

### ESP32 with NRF24L01
https://github.com/adarsh-h-007/ESP32-HSPI-VSPI-Demo/blob/master/Pictures/01%20NRF24L01%20Pinout.png

[Image Source](https://forum.arduino.cc/t/simple-nrf24l01-2-4ghz-transceiver-demo/405123)

|   NRF24L01    |   ESP32   |
|   --------    |   -----   |
|   1 GND       |   GND     |
|   2 Vcc       |   3V3     |
|   3 CE        |   D22     |
|   4 CSN       |   D21     |
|   5 SCK       |   D18     |
|   6 MOSI      |   D23     |
|   7 MISO      |   D19     |
|   8 IRQ       |   Not required    |

### ESP32 with RC522

|   RC522  |   ESP32   |
|   ----   |   -----   |
|   Vcc     |   3V3     |     
|   RST     |   D27     |
|   GND     |   GND     |
|   IRQ     |   Not required    |
|   MISO    |   D12     |
|   MOSI    |   D13     |
|   SCK     |   D14     |
|   SS      |   D26     |

### ESP8266 with NRF24L01

|   NRF24L01    |   ESP8266     |
|   ----        |   ----        |
|   1 GND       |   GND     |
|   2 Vcc       |   3V3     |
|   3 CE        |   D2 (GPIO 4)     |
|   4 CSN       |   D1  (GPIO 5)     |
|   5 SCK       |   D5 (GPIO 14)     |
|   6 MOSI      |   D7  (GPIO 13)     |
|   7 MISO      |   D6  (GPIO 12)     |
|   8 IRQ       |   Not required    |

Now that all the connections are set, let’s get started with PlatformIO. I am assuming that you’ve set up PlatformIO and Visual Studio Code.

## 3. Software Setup
### 3.1 Configuring PlatformIO to use both ESP32 and ESP8266 simultaneously:

Let’s start by making PlatformIO compatible with using two boards simultaneously. I stumbled upon an excellent guide for this: [Manage Two Arduinos with Ease Using PlatformIO](https://kevinxli.medium.com/manage-two-arduinos-with-ease-using-platformio-4f83ad4a8868). This is where I learned how to use two boards at the same time. Take a look if you want to know more details. I’ll be discussing how to set it up for ESP32 and ESP8266.

#### Step 1: Create a New Project
Create a new PlatformIO project. Give it a suitable name and select one of the two boards you plan to use (I’ll select `nodemcu-32s`). It doesn’t matter which one you select initially, as we’ll configure the project to support both boards soon. Choose the framework as Arduino.

#### Step 2: Set Up Separate Code Files
The `main.cpp` file, which contains the code, is stored in the `PlatformIO\<project_name>\src` folder. We’ll create two new `.cpp` files, one for the ESP32 and one for the ESP8266. 

1. Delete the `main.cpp` file in the `src` folder from the Explorer tab in Visual Studio Code.
2. Right-click on the `src` folder and select **New File**. Name the new file `ESP32.cpp`.
3. Create another file named `ESP8266.cpp` in the `src` folder.

It should now be clear that the code for each board will be written in its respective file. 

#### Step 3: Configure `platformio.ini`
To configure PlatformIO to use both `.cpp` files and upload them to their respective boards, open the `platformio.ini` file in your project folder. 

#### ESP32 Configuration
Inside `platformio.ini`, you’ll see something like this:
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
```
Add the following line to ensure PlatformIO only compiles the `ESP32.cpp` file and ignores the `ESP8266.cpp` file:
```
build_src_filter = 
	${env.src_filter}
	-<ESP8266.cpp>
```

#### ESP8266 Configuration
Now you need to find out the details of your second board. I don’t have a very good answer as to how to do that, but what I do is create a dummy project with that board, open the ‘platformio.ini’ file of that project, and copy it. It’s not ideal, but it works, lmao. 

In my case, the ESP8266 details in the `platformio.ini` file look like this:
```
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
```
Add this to your main project’s `platformio.ini` file. To ensure PlatformIO ignores the `ESP32.cpp` file for the ESP8266, add:
```
build_src_filter = 
	${env.src_filter}
	-<ESP32.cpp>
```
After all this your `platformio.ini` file should look soomething like this:
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP8266.cpp>

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP32.cpp>
```

#### Step 4: Set Baud Rate
Set the desired baud rate for both boards. I’ll use a baud rate of `115200`. Add this line under both `[env:nodemcu-32s]` and `[env:nodemcuv2]`:
`monitor_speed = 115200`


#### Step 5: Configure Upload and Monitor Ports
Now comes the tricky part. We need to find the ports used by our respective boards. If it’s just one board, we could simply click upload, and PlatformIO would find the port automatically. But now, since we have two boards, we need to specify which board uses which port. This is tricky because I’ve noticed that every time my laptop restarts, the port assignments change. So, you’ll have to figure this out using trial and error.

Connect both your boards to your PC. Now open PlatformIO and open a terminal. It’s usually located at the bottom and has the icon of a terminal. Click on it to open the terminal. In the terminal, type:
`pio device list`

You’ll see a bunch of devices connected to your ports. Among the ports, look for lines with `Description: Silicon Labs CP210x USB to UART Bridge` (this might vary depending on the board you’re using). You should see two of these. Note down both ports. 

Here comes the confusing part: figuring out which port is connected to which board. In my case, I have two COM ports, `COM7` and `COM10`, that have `Description: Silicon Labs CP210x USB to UART Bridge`.

Now open the `platformio.ini` file of your project. Here, under each board, we need to specify which port to upload to and which port to monitor. So, under `[env:nodemcu-32s]`, I’m going to specify:
```
upload_port = COM7 
monitor_port = COM7
```
And under `[env:nodemcuv2]`:
```
upload_port = COM10 
monitor_port = COM10
```
Remember to save the file after making any changes. 

The only way I know how to verify these port assignments is to just try and upload some code. If the code uploads fine, then voilà, everything’s good. But if I get an `upload error`, I’ll try switching the ports—i.e., replace `COM7` with `COM10` everywhere and vice-versa.

After this step your `pllatformio.ini` file should look something like this:
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP8266.cpp>
monitor_speed = 115200
upload_port = COM10
monitor_port = COM10

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP32.cpp>
monitor_speed = 115200
upload_port = COM7
monitor_port = COM7
```

#### Step 6: Install Libraries

Next, we need to install the necessary libraries for our project. Click on the PlatformIO icon on the right side, and under `PIO Home`, click on `Libraries`. Now, install the `MFRC522 by Miki Balboa` library by searching for it and clicking on `Add to Project`. Then, select your project and click on `Add`.

Now we need to install the libraries for the NRF24L01 module. Just like before, search for `RF24Network by nRF24` and add it to your project. PlatformIO will automatically install the `RF24Network` library and all its dependencies.

**Very Important:** The RF24Network also adds a library called `nrf_to_nrf`. I have no idea why, but this library always creates an error during code compilation. So, we need to remove it. To remove it, go to `PlatformIO\<project_name>\.pio\libdeps\board_name` and delete the `nrf_to_nrf` folder. You have to do this for both the boards.

Finally, your `platformio.ini` file should look something like this:
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP8266.cpp>
monitor_speed = 115200
lib_deps = 
	miguelbalboa/MFRC522@^1.4.11
	nrf24/RF24Network@^2.0.3
upload_port = COM10
monitor_port = COM10

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
build_src_filter = 
	${env.src_filter}
	-<ESP32.cpp>
monitor_speed = 115200
lib_deps = 
	miguelbalboa/MFRC522@^1.4.11
	nrf24/RF24Network@^2.0.3
upload_port = COM7
monitor_port = COM7

```
## 4. Modifying the MFRC522 Library to use the HSPI bus:
As I’ve mentioned before, most libraries assume that we’ll be using the default VSPI bus for SPI communication, and so does this library. We need to make a slight modification to the library as well as to our ‘ESP32.cpp’ code to make it use the HSPI bus. I learned about this change from a GitHub issue [comment](https://github.com/espressif/arduino-esp32/issues/1219#issuecomment-373496330).

So what we need to do is modify the ‘MFRC522.cpp’ file at: `PlatformIO\project_name\.pio\libdeps\nodemcu-32s\MFRC522\src`.

Open the ‘MFRC522.cpp’ file using Visual Studio Code. After all the code for header file inclusion (i.e., after all the `#include` lines), add:  
```cpp
SPIClass HSPIRFID(HSPI);
```
So now we’ve created an `SPIClass` instance called `HSPIRFID` that uses the HSPI bus. Next, we need to replace all the instances of `SPI.` with `HSPIRFID.`. To do that:

- Press `CTRL + H` to bring up the find-and-replace tool
- In the find box, type `SPI.` (including the dot).
- Make sure `Match case` (`Alt + C`) and `Match Whole Word` (`Alt + W`) are selected.
- In the replace box, type `HSPIRFID.` (including the dot).
- Then click on `Replace All` (`CTRL + ALT + ENTER`).

After replacing everything, we need to add:

```cpp
#include <SPI.h>
```
Under the `#include "MFRC522.h"` line. We add this after replacing the `SPI.` instances because otherwise it’ll replace the SPI in the header file as well, which we don’t want.

Now the library modification is complete, but we still need to modify our code. In our ESP32 code, we need to add:

```cpp
extern SPIClass HSPIRFID;
```
after including the header files (`#include` statements) to specify that HSPIRFID has been defined in another file. Otherwise, we’ll get an error.

If you’re really lazy to do all this, just download the `MFRC522.cpp` file from this GitHub repository and replace it with the file in your own project folder. The folder structure of this repository is the same as that of any PlatformIO project. If you do this, make sure to still include `extern SPIClass HSPIRFID;` in your code, or it’ll throw an error.

## 5. Project Code:
So that’s it for all the preparation. The next step is to write the code for our microcontrollers. I’ve written the code for both the ESP32 and ESP8266 in this repository; it’s in the `src` folder. You can copy and paste or download the files as you wish. Make necessary modifications as you see fit. Now I’ll explain what the code does in brief:

- The ESP32 monitors the RC522 reader for any tags.
- If a tag is detected, it scans its UID.
- The UID is associated with the ESP8266.
- The ESP32 queries the ESP8266 about its speed and intended direction through the NRF24L01 module (keep in mind this is a V2X project, hence the vehicle jargon).
- The ESP8266 decides some random speed and direction and sends it back to the ESP32 through the NRF24L01 module.
- The ESP32 computes a priority based on speed and sends it back to the ESP8266.

This is arbitrary code that I’ve written. You can modify it as you wish.

This is the structure of the message that is being sent and received:

```
+=====+====================+===========================+
| Byte| Field Name         | Description               |
+=====+====================+===========================+
|  0  | Message Type       | 1 = Query                 |
|     |                    | 2 = Response to Query     |
|     |                    | 3 = Priority Assignment   |
+-----+--------------------+---------------------------+
| 1-4 | RFID Unique ID     | Unique ID of RFID Tag     |
+-----+--------------------+---------------------------+
|  5  | Emergency Slowdown | Used in Type 1; stuffed   |
|     |                    | with 0xFF in others       |
+-----+--------------------+---------------------------+
|  6  | Priority Level     | Used in Type 3; stuffed   |
|     |                    | with 0xFF in others       |
+-----+--------------------+---------------------------+
|  7  | Vehicle Speed      | Relevant in Type 2 & 3;   |
|     |                    | stuffed in Type 1         |
+-----+--------------------+---------------------------+
| 8-9 | Vehicle Direction  | 0x01 0x00 = Left          |
|     |                    | 0x00 0x01 = Right         |
|     |                    | 0x01 0x01 = Straight      |
+=====+====================+===========================+
```

## 6. Serial Monitor Setup
This is the last step: opening both the serial monitors of ESP32 and ESP8266. It’s a really simple process, and I’m just including it for the sake of it.

1. Click on the PlatformIO icon on the right side.
2. On the rightward tab `Project Tasks`, click on the board name (here it’s `nodemcu-32s`).
3. Under `General`, click on `Monitor`.
4. Repeat the same for the next board (here `nodemcuv2`) as well.
Now you have both the serial monitors open. Drag and keep them on split screen or do as you wish.

## For The Lazy Peeps:
If you don’t want to go through all this or just want to test this quickly for some reason, I’ve got you covered. Just download this repository as a zip file and extract it. Open PlatformIO and click on `Open Project`, navigate to the extracted folder, and click open. Now you can compile and run everything. But your lazy ass will now encounter a problem, most probably; the COM ports won’t match. So suck it up, find your COM ports as mentioned under `Step 5: Configure Upload and Monitor Ports` in Section 3.1, and update the port assignments in your `platformio.ini` file.

# Footnote
Now that’s it, folks. I know this isn’t some novel revolutionary idea that I’ve come up with, nor is it cutting-edge, latest technology. All the information I’ve provided here could be found using a couple of Google searches, and I recommend doing that so that you can learn.

I hope someone like me, who’s looking to interface something as obscure as ESP32 with RC522 and NRF24L01 and then communicate with another ESP8266, finds this helpful. Or if you belong to the normal category of sane humans, I hope you learn something from this as well.

I take this moment to thank all the authors of all the resources that I’ve used. If I haven’t credited you, I’m sorry. Additionally, I would like to extend a big thank you to ChatGPT for assisting me with this project, helping format and correct the mistakes in this readme file, and making it more presentable.

Currently, I’m working on a V2X project with my team for my final year B.Tech project. I don’t know if we’ll be able to pull it off or even if we do, it’ll be impractical and overcomplicate trivial things. I sincerely hope not.

If you’ve made it this far, thanks for reading, and I hope whatever you’re doing works out well. Thank you. :)
