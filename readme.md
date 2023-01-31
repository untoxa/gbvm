# Simple VM for the script-driven Game Boy or SMS/GG/MSX games

You need GBDK-2020 v.4.1 and higher to compile this repo

Features:
- human readable assembler-like scripts
- rom banks support
- multi-threading

Notes:
- threads share the same memory
- contexts stack grows ahead

Instead of 100 words:

![scheme](/scheme.png)

Your game objects in RAM may overlay VM memory, for example, like this:

![scheme](/scheme2.png)

Example script running on the Sega Game Gear:

![example](/example.png)
