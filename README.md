# Ekaggata
A small virtual machne with its work assembly language


## What is this

Ekaggata is an attempt to create a virtual machine that works sort of like an embedded system, where hardware IO is accessible through registers on the machine. 
I am also building up a little ISA to learn more about computer architecture. The various opcodes can be seen in "opcodes.x".

This project is quite hacky. I use [X macros](https://en.wikipedia.org/wiki/X_Macro) for storing various internal data, like register names and opcode information.


Ekagatta depends on SDL2 for graphics display, and the build file only runs on Linux.

```./compile``` to execute


## Some more info
Please do not mind how the .ls (litte assembly) is compiled, it will be fixed later. For now, a python scripts wraps each line in an X Macro, and this is imported into the ```asm.c``` file, where the values of line are put into a struct and placed into memory...

There are still loads of things to figure out :)
