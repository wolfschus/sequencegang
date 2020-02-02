# sequencegang
A Midi Sequencer for Linux / SDL

1987 I have started to develop a midi sequencer on the C64. After this I ported to the Amiga. After I was infected by the Raspberry Pi Virus some years ago, I started this project to develop a midi sequencer for raspberian.


To use binary:

sudo apt install libsdl-gfx1.2-5

To compile:

sudo apt install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev librtmidi-dev librtaudio-dev libsqlite3-dev libsdl-image1.2-dev libfluidsynth-dev fluid-soundfont-gm

make

To install:

make setup
sudo make install

