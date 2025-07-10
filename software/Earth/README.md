# Earth (Planet Series)

Earth is a digital plate reverb and octave pedal using the Dattorro algorithm.

![app](https://github.com/GuitarML/Funbox/blob/main/software/images/earth_infographic.jpg)

The Reverb is a modified version of the Dattorro implementation in [VCV rack Plateau](https://github.com/ValleyAudio/ValleyRackFree/tree/main/src/Plateau). 
The polyoctave effect is from: [Terrarium-Poly-Octave](https://github.com/schult/terrarium-poly-octave).

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | PreDelay | Amount of predelay, up to around 700ms |
| Ctrl 2 | Mix  | Dry/Wet mix |
| Ctrl 3 | Decay  | Amount of decay |
| Ctrl 4 | Mod Depth | Intensity of Mod |
| Ctrl 5 | Mod Speed | Rate of Modulation  |
| Ctrl 6 | Filter | Left of center is high cut, right of center is low cut |
| 3-Way Switch 1 | Reverb Size | Left=Small, Center=Med, Right=Big |
| 3-Way Switch 2 | Octave  | Left=None, Center=Octave Up, Right=Octave Up and Down  |
| 3-Way Switch 3 | Footswitch Action | Left=Momentary Freeze, Center=Momentary Overdrive, Right=Momentary Octave | 
| FS 1 | Bypass/Engage |  |
| FS 2 | Footswitch Action | Perform action based on toggle switch 3 |
| LED 1 | Bypass Indicator |  |
| LED 2 | Preset Indicator |  |
| Audio In 1 | Mono In | Right channel ignored |
| Audio Out 1 | Stereo Out  | Stereo Out via TRS |
<br>
### Expression
1. Plug in passive expression pedal into the 1/8" jack on the top left side of pedal. (will need a 1/4" female to 1/8" male TRS adapter)<br>
2. Hold both footswitches until they both light up (more than 0.5 seconds, but less than 2 seconds), you are now in Set Expression mode.<br>
3. Move the expression pedal into the heel position (up) and move any number of knobs to where you want to heel limit to be (for example you could turn Volume down). The right LED should be brighter up to indicate the heel position is ready to set.*<br>
3. Move the expression pedal into the toe position (down) and move any number of knobs to where you want to toe limit to be (for example you could turn Volume up). The Left LED should be brighter to indicate the toe position is ready to set.*<br>
4. Hold both footswitches to exit Set Expression mode. This will activate expression for the moved knobs. The moved knobs will be inactive until Expression Mode is deactivated.<br>
5. Repeat step 2 to reset Expression knobs.<br>
6. Hold both footswitches for 2 seconds or more to clear any Expression action and give control back to the Funbox knobs.<br>
<br>
* Currently, the expression input requires the full range of the expression pedal, in order to detect Up/Down positions. You can sometimes trim the expression pedal to not use the full range, so adjust the trim as necessary.<br>
  Also, some expression pedals have a "Standard" or "Alternate/Other" mode. Funbox should work on the "Standard" mode.<br>

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

Build with ```make```.

Earth is intended to be used as a submodule in Funbox, and build paths expect to be used as such. The Earth 
code was split out from the Funbox project to preserve the License used in reused/modified code from other projects. 
To build Earth, it is recommended to clone the Funbox project and run "git submodule update --init --recursive" 
to get Earth and all required dependencies. Otherwise, you can download the .bin executable to upload to the Daisy Seed 
from the Releases page.