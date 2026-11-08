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
