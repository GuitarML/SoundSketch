# Mercury (Planet Series)

Mercury is an amp sim using Neural Amp Modeler (NAM). It includes 9 custom size neural models and a 4 band EQ (Bass, Mid, Treble, Presence).
It is intended to be used with a separate IR/CabSim, or an actual amp (no cab modelling on Mercury).

Neural Network inference for the amp/pedal models uses [RTNeural](https://github.com/jatinchowdhury18/RTNeural) with [RTNeural-NAM](https://github.com/jatinchowdhury18/RTNeural-NAM).

Note: Due to the processing power required for the neural models, Mercury is a mono effect, which on the Funbox pedal is copied left to right channel for output. 
If a stereo signal is going into the input, Mercury will only take the left channel for processing.

![app](https://github.com/GuitarML/Funbox/blob/main/software/images/mercury_infographic.jpg)

*Models included:
Low Gain:
- Matchless SC30
- Twin Reverb
- Dumble Low Gain (from kit build)

Medium Gain:
- Dumble Higher Gain (from kit build)
- Ethos Tube Pedal
- JCM800 Medium Gain

High Gain:
- Mesa iib
- Splawn
- PRS Archon

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Gain | Increases the input gain to the neural model up to 2.5x |
| Ctrl 2 | Level | Overall volume |
| Ctrl 3 | Presence | +10/-10 dB Presence control, center is 0. |
| Ctrl 4 | Bass | +10/-10 dB Bass control, center is 0. |
| Ctrl 5 | Mid | +10/-10 dB Mid control, center is 0.  |
| Ctrl 6 | Treble | +10/-10 dB Treble control, center is 0. |
| 3-Way Switch 1 | Amp/Model Group | 3 available groups, default: left: LowGain, center: MedGain, right: HighGain |
| 3-Way Switch 2 | NAM Model Select |  3 available NAM models for each group* |
| 3-Way Switch 3 | Unused |  |
| Dip Switch 1 | |  |
| Dip Switch 2 | |  |
| FS 1 | Bypass |  |
| FS 2 | Preset |  |
| LED 1 | Bypass Indicator |  |
| LED 2 | Preset Indicator |  ||
| Audio In 1 | Mono In (Left Channel) | Right channel ignored if using TRS  |
| Audio Out 1 | Mono Out  | Left channel copied to Right Channel if using TRS |

### MIDI Reference

| Control | MIDI CC | Value |
| --- | --- | --- |
| Knob 1 | 14 | 0- 127 |
| Knob 2 | 15 | 0- 127 |
| Knob 3 | 16 | 0- 127 |
| Knob 4 | 17 | 0- 127 |
| Knob 5 | 18 | 0- 127 |
| Knob 6 | 19 | 0- 127 |

## Build

Mercury runs in SRAM memory on the Daisy Seed. You must use the Bootloader to load the executable.

Mercury is intended to be used as a submodule in Funbox, and build paths expect to be used as such. The Mercury 
code was split out from the Funbox project to preserve the License used in reused/modified code from other projects. 
To build Mercury, it is recommended to clone the Funbox project and run "git submodule update --init --recursive" 
to get Mercury and all required dependencies. Otherwise, you can download the .bin executable to upload to the Daisy Seed 
from the Releases page.