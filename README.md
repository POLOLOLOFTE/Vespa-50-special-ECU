# Vespa-50-special-ECU
A rpm counter for the vespa 50 special.... it work with an esp32

## How it works:
soo... this will be a bit hard to explain, because not everybody owns a Vespa 50 Special form 1977.

but i will do what i can:

there is and input from the points in the engine and then that input will be converted in more usable sorce of energy around 200V in AC so the Octofotocoapiator can accually register the signal

> [Note:] the signal from the points is very unregoladed and it has got a bad backfire (magnetally) so it will burts the led inide the Octofotocopiator

after getting the signal right it goes from a gpio in a ESP32 (it's great even an arduino it's just slower)
