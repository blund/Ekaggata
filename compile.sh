#!/bin/sh

mkdir -p build

CC="gcc"
FLAGS="-o build/ekaggata"
STEP=""
RUN="./build/ekaggata"
MESSAGES=""

while getopts "c:ors" arg; do
    case $arg in
        c)
            CC=${OPTARG}
            [[ ! $CC =~ clang|gcc|tcc ]]
            ;;
        o)
            FLAGS="${FLAGS} -O2"
            ;;
        s)  STEP="-DDEBUG"
            ;;
        *)
            echo "Fikk et ulovlig argument: ${OPTARG}!"
            ;;
    esac
done
shift $((OPTIND-1))

echo
echo "Kompilerer med ${CC}"
echo

FILES="asm.c renderer.c"
LINKER_FLAGS=" -I/usr/include/SDL2 -lSDL2"


python assemble.py && $CC $FILES $FLAGS $STEP $LINKER_FLAGS && $RUN
