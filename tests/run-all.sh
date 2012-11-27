#!/bin/bash

for SET in "verge"; do
    OLDDIR=$PWD
    cd $SET
    ./run.sh tests/*
    cd $OLDDIR
done

