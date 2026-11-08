# Vespa-50-special-ECU
A rpm counter for the vespa 50 special.... it work with an esp32

## How it works:
soo... this will be a bit hard to explain, because not everybody owns a Vespa 50 Special form 1977.

but i will do what i can:

there is and input from the points in the engine and then that input will be converted in more usable sorce of energy around 200V in AC so the Octofotocoapiator can accually register the signal

> [Note:] the signal from the points is very unregoladed and it has got a bad backfire (magnetally) so it will burts the led inide the Octofotocopiator

after getting the signal right it goes from a gpio in a ESP32 (it's great even an arduino it's just slower)

and then ino get displayed on a oled display

## Assembly
> [Note:] This is designer for the Vespa 50 special but it can applyed to other veicles (mostly old monopeds)

i didn't designed a pcb for this becuse i will use a breadboard (more easy to work)

so you assemble the logic part and you have to put it in a place that the points wire is easily accessible and you got the bars not so far 

then you have to wire up everything and install the button and display (with their mount) on the bars (you should use a bit of force to install it because it vibrate a lot)

and if you start it up is should go

## Input and Output

you need to connect the ESP 32 to a 5V source and then you haveto connect the hole system to the points for it to works

and then you could connect the ESP32 to your phone and you can send packages via bluethoot for the Gps (I use Serial Bluethoot Terminal)

> [^Experimental^] my code is not that great so don't worry if it dosn't work at all

## Controls

i have made (or i think is it) a preatty easy interface: for swiching interface kmh-rpm you have to press raplidy the button, and then if you want to see how many rpm it can get you have to press the button for at lest 800ms so 0.8 seconds

you can personalize your code 

## Gallery
<img width="571" height="768" alt="Screenshot 2026-11-07 213111" src="https://github.com/user-attachments/assets/8022bb42-8e5e-49b2-aae8-aa276b85060f" />
<img width="1107" height="695" alt="Screenshot 2026-11-03 165344" src="https://github.com/user-attachments/assets/05d3ccbc-9843-4b13-b07d-058ff9905793" />
<img width="169" height="209" alt="Screenshot 2026-11-03 164758" src="https://github.com/user-attachments/assets/34c7a7b1-f465-4e9a-b33a-d4092ea428e3" />
<img width="918" height="870" alt="Screenshot 2026-11-08 102136" src="https://github.com/user-attachments/assets/206e76f3-320c-4f9c-b943-5d03b5ffe715" />

## Things that you need:

| # | Component | Qty | Why it is used | Amazon link | Approx. price (USD) | Notes |
|---|-----------|-----|----------------|-------------|---------------------|-------|
| 1 | ESP32 dev board (ESP-WROOM-32, 38-pin) | 1 | Main microcontroller: counts ignition pulses to compute RPM, drives the display and receives the speed over Bluetooth Classic. | [HiLetgo ESP-WROOM-32 ESP32 dev board](https://www.amazon.com/ESP-WROOM-32-Development-Dual-Mode-Microcontroller-Integrated/dp/B0FRD82Q54) | ~$10 | Must be the **original ESP32** (Bluetooth Classic). ESP32-S2/S3/C3/C6 will **not** work. Choose the **1-piece** option on the listing. |
| 2 | 1.3" OLED display, 128x64, I2C (SH1106) | 1 | Shows the RPM / km/h, the bar graph and the max value. | [1.3" I2C OLED SH1106 128x64 (1 pc, with jumper wires)](https://us.amazon.com/Display-Breadboard-Multicolored-Compatible-Raspberry/dp/B0HFSCMGWD) | ~$8 | 3.3V compatible. Check the pin order printed on the module (GND/VCC/SCL/SDA) before powering. The code uses the SH1106 driver (U8g2). |
| 3 | AC optocoupler isolation module (1 channel, 220V AC detect) | 1 | Galvanically isolates the high-voltage ignition signal and turns it into a clean 3.3V logic pulse for the ESP32 (output goes low while the input is active). | [2-pack 1-bit AC optocoupler isolation module](https://www.amazon.com/Optocoupler-Isolation-Module-Voltage-Adaptive/dp/B0G642W1RJ) | ~$7 | Sold as a 220V AC detector; check the real maximum input voltage of the board you receive (this build uses a module rated for about 300V). 2-pack gives a spare. |
| 4 | 15 kΩ resistor, 2 W, metal film | 2 | Series current-limiting resistors on the high-voltage input side of the optocoupler module. | [Set of 15 pcs 15K ohm 2W 1% metal film resistors](https://www.amazon.com/Pieces-Tolerance-Metal-Film-Resistor/dp/B00VQSNAUM) | ~$8 | Only 2 are needed; the rest are spares. 2 W rating is needed for the high-voltage side. |
| 5 | 100 nF (0.1 µF) capacitor, 400 V film | 1 | Capacitor on the high-voltage side, rated for the ignition pulses. | [10 pcs 104 400V 0.1uF polypropylene film capacitors](https://www.amazon.com/10pcs-0-1uF-Metallized-Polypropylene-Capacitor/dp/B0FHDYPS8F) | ~$6 | Must be rated **400 V or more**. The spares can also be used across the push button to reduce electrical noise. |
| 6 | 10 Ω resistor, 1/4 W | 1 | Low-value series resistor in the sensing path. | [Pack of 30 resistors, 10 Ω – 1 MΩ, 1/4W 1%](https://www.amazon.com/10-1M-Resistors-Metal-Resistor-Assortment/dp/B0CLV1YXH8) | ~$6 | The pack contains the 10 Ω value plus many others. |
| 7 | 16 mm metal push button, momentary, waterproof (stainless steel) | 1 | Short press: switch between RPM and km/h view. Long press (> 0.8 s): reset the max values. Wired between GPIO25 and GND (internal pull-up, no resistor needed). | [16mm IP65 waterproof metal momentary push button (pack of 2)](https://www.amazon.com/Momentary-Button-Waterproof-Stainless-Terminals/dp/B0F62S67RS) | ~$9 | Racing-style stainless steel button, normally open (no polarity), screw terminals. Needs a **16 mm mounting hole** (head about 17.8 mm), so make that hole in the 3D printed part. Pack of 2 gives a spare. |
| 8 | Breadboard + jumper wires kit | 1 | Prototyping and wiring everything without soldering. | [830 + 400 tie-point breadboards with jumper wires kit](https://www.amazon.com/ALLECIN-Breadboard-Jumper-Wires-Kit/dp/B0C1VDKPW2) | ~$9 | Includes two breadboards and about 200 jumper wires. |
| 9 | 3D printed parts | – | Enclosure and mounting parts for the display, the ESP32 and the button. | – (custom, printed at home or by a print service) | ~$10 | Estimated cost. |

**Estimated total: about $73** (before shipping and tax).

### Not included

- The Vespa
- An Android phone (Bluetooth Classic is not supported on iPhone)
- A power source for the ESP32: a USB power bank, or a 12V to 5V buck converter if the bike has a battery
- A USB cable with data lines for programming the ESP32
- Basic tools (soldering iron, wire cutters, etc.)

