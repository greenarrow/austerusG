# austerusG

**austerusG** is a collection of programs for controlling 3D printers like
*RepRap* and the *Ultimaker* on Linux in a simple and efficient manner.

## Why another host?

*   Many previous host programs have issues with latency that lead to printer
pauses and ultimately physical defects on prints. Languages like Java and
Python are inherently unpredictable due to the use of garbage collectors etc.

*   **austerusG** has been designed to stream gcode to printers with the lowest
latency and smallest overhead possible to ensure the highest quality of prints.
This allows the use of very low power hardware such as the *Raspberry Pi*.

## Introduction

    $ make
    # make install
    $ austerus-panel -s /dev/ttyACM0 -b 230400
    $ austerus-send -s /dev/ttyACM0 -b 230400 -a 6 test.gcode

## Design

**austerusG** is written entirely in C and consists of several small purposeful
programs.

When *austerus-send* is executed it in turn starts an *austerus-core* process
that it talks to through a pipe.

    |file| =====> |austerus-send| =====> |austerus-core| =======> |printer|
            read                   pipe                   serial

### austerus-core

This is the most important part of **austerusG**. The *core* is kept as simple
as possible to ensure the lowest possible latency.

The *core* simply reads lines in from *stdin* and writes them out as fast as
the printer accepts them.

Critically there are two places where the program blocks:

*   Read from *stdin*
    *   The use of the pipe ensures this never happens while there is still
data to send.
*   Read from *serial* while *unacknowledged lines* > *ack-count*
    *   Where *unacknowledged lines* is the number of lines that have been sent
to the printer that acknowledgements have not been recieved for.
    *   Where *limit* is the number of lines the printer firmware can queue.

The code paths between blocking points and transmission of the next line are
very short so it would be unusual for the line queue on the firmware to not
be full.

### austerus-send

Simple program for printing gcode files while displaying progress.

### austerus-panel

Simple *Ncurses* based control panel for 3D printers.

