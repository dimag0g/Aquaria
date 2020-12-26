This is a port of Aquaria game to Nintendo Switch. It's a homebrew app, so you need to be running custom firmware for it to work.


[1] BUILDING
------------

If you're targeting Nintendo Switch, make sure you're using the `switch` branch. The `master` branch contains changes which are not specific to Switch and could be used on other platforms.

You will need to install the [devkitPro](https://devkitpro.org/) toolchain. Make sure to include the following packages:

    libnx switch-sdl2_image switch-sdl2_mixer switch-mesa

You'll need to build and install two additional libraries, [mojoAL-switch](https://github.com/TomBebb/mojoAL-switch) and [GLAD](https://glad.dav1d.de/).
For GLAD, I have used OpenGL 2.1 compatibility profile. Versions of those libraries which I have used are saved in `Aquaria\ExternalLibs`.

Then follow the usual steps to build Aquaria.

If you don't intend to do any coding and just want to play, grab the latest [release](https://github.com/dimag0g/Aquaria/releases) instead.


[2] SETUP
---------

You will need a copy of the official game to run this port. Demo version will not work.

Aquaria root directory is hardcoded as `/switch/aquaria`. Put all the game files there, then copy over the files from `files` directory, as well as `aquaria.nro`.
Replace any existing files when prompted. Don't remove any existing files, unless you're sure those are not needed (e.g. `Aquaria.exe`).

In the end you should have the following directory tree on your SD card:

    switch
     |
     +-- ...
     +-- aquaria  <--- this is the game directory
         |
         +--- .Aquaria
         +--- data
         +--- gfx
         +--- mus
         +--- scripts
         +--- sfx
         +--- vox
         +--- ...
         +--- aquaria.nro

⚠ Be careful if you do this on a Mac with finder because it replaces whole folders instead of just merging the new files in.

⚠ Make sure you don't forget the `.Aquaria` folder which may be hidden by your file manager.

Lastly, if you want to play the included default mods,
copy `game_scripts/_mods` over your local `_mods` directory.


[3] RUNNING
-----------

This build of Aquaria was tested on 10.2.0|AMS 0.14.4|S (FAT32). exFAT is not recommended.
Alternative controllers (keyboards, mouses, etc.) might work but weren't tested.

Default keys (most of which can be changed in the game options) are:
- A/R - Primary Action (sing/shoot)
- B/L - Secondary Action (swim boost)
- X - Song form
- Y - Energy form
- ⊖ - Open map
- ⊕ - Menu/Escape
- D-PAD / L-stick - navigate
- D-PAD ▴ - Cook food
- D-PAD ▾ - Remove food from wok pan
- L-stick push - Roll
- R-stick + push - Look around

[4] TODO
--------

Here's a list of known bugs / missing features, approximately in order of priority. Don't bother reporting those as issues.

- incomplete controls support(no ZR/ZL, no touchscreen)
- no support for full HD in docked mode
- no mods downloading support
- no locale support
- game hangs when skipping cutscenes - skippin cutscenes disabled
- glitches in world map - background made solid, transitions are still buggy
- occasional crash on exit
- no demo version support
