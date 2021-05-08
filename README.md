# Ekaggata
A small virtual computer


## What is this

Ekaggata is an attempt to create a virtual computer that works sort of like an embedded system, where hardware IO is accessible through registers on the machine. 

I am also building up a little ISA to learn more about computer architecture. The various opcodes can be seen in ```opcodes.x```.

The ```*.x``` files use a concept called [X macros](https://en.wikipedia.org/wiki/X_Macro). It is a terrifying hack, but also very convenient for storing related data together in C.

## How to run
Ekagatta depends on SDL2 for graphics display and Python for some file translation. The build file only runs on Linux.

So to compile and execute, simply run
```./compile``` 

This will run the python script ```assemble.py``` to translate ```test.ls``` (our "assembly" file) into a format the C pre-processor can understand and write instructions with. It is quite hacky, but it means we can use the C compiler to check for simple mistakes, like spelling instruction names wrong.

## Shortcomings
Things that are missing but I plan on adding:
  * Labels
  * Floats
  * Audio IO
  * Keyboard IO
  * Networking IO
  * An actual assembler to get rid of Python as a dependency
  * Definitions of memory mappings
  * And probably more!
