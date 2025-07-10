# libDaisy Modifications

Replace these files in libDaisy before building libDaisy. This is required to correctly map the controls for SoundSketch hardware.

<br><br>
INSTRUCTIONS:
```
# Replace libDaisy/src/daisy_pedal.h with SoundSketch/mod/daisy_pedal.h
# Replace libDaisy/src/daisy_pedal.cpp with SoundSketch/mod/daisy_pedal.cpp

make -C libDaisy
```

## bksheperd hardware module definitions

These are the hardware definitions for using the bksheperd guitar pedal framework on the SoundSketch. 
Add these two files to the following location in your [DaisySeedProjects](https://github.com/bkshepherd/DaisySeedProjects/tree/main) repo and update 
the guitar_pedal.cpp to use this hardware definition.

Add the guitar_pedal_soundsketch.h and guitar_pedal_soundsketch.cpp file to this location to use this extensive guitar effect framework on the SoundSketch:
https://github.com/bkshepherd/DaisySeedProjects/tree/main/Software/GuitarPedal/Hardware-Modules
