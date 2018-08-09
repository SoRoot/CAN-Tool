#!/usr/bin/bash

includes="-Isrc -Isrc/synergy_gen"

find . -name "*.h"


cfiles="src/*.c test/*.c"

gcc ${includes} ${cfiles} -o test.exe
