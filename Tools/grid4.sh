#!/bin/bash
# grid4.sh <dir> <out.jpg> <a> <b> <c> <d>: four captures on one sheet (for looking at runs quickly)
d=$1; o=$2; shift 2
ffmpeg -v error -y -i "$d/$1.png" -i "$d/$2.png" -i "$d/$3.png" -i "$d/$4.png" -filter_complex "[0]scale=960:540[a];[1]scale=960:540[b];[2]scale=960:540[c];[3]scale=960:540[d];[a][b][c][d]xstack=inputs=4:layout=0_0|960_0|0_540|960_540" "$o"
