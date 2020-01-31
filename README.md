# sequencegang
A Midi Sequencer for Linux / SDL

To use binary:

sudo apt install libsdl-gfx1.2-5

To compile:

sudo apt install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev librtmidi-dev librtaudio-dev libsqlite3-dev libsdl-image1.2-dev libfluidsynth-dev fluid-soundfont-gm

./make

To install:

sudo cp sequencegang /usr/local/bin/
rm sequencegang
mkdir ~/.sequencegang
cp -r db/* ~/.sequencegang/
