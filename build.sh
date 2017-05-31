#!/usr/bin/env sh
basedir="./linux/x64/"
mkdir -p $basedir
mkdir -p "$basedir/bin"
mkdir -p "$basedir/bin/release"
mkdir -p "$basedir/bin/debug"
mkdir -p "$basedir/obj"
mkdir -p "$basedir/obj/release"
mkdir -p "$basedir/obj/debug"
cp "./palette.pal" "$basedir/bin/release/palette.pal"
cp "./palette.pal" "$basedir/bin/debug/palette.pal"
make "$@" > "./make.log"
