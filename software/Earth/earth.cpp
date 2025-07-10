// Earth Reverbscape
// (For Soundsketch Pedal)

#include "daisy_petal.h"
#include "daisysp.h"
#include "soundsketch.h"
#include "expressionHandler.h"

#include "Dattorro/Dattorro.hpp"

#include <q/support/literals.hpp>
#include <q/fx/biquad.hpp>
#include "Util/Multirate.h"
#include "Util/OctaveGenerator.h"
namespace q = cycfi::q;
using namespace q::literals;

using namespace daisy;
using namespace daisysp;
using namespace soundsketch; 

// True bypass
GPIO audioBypassTrigger;
GPIO audioMuteTrigger;
bool m_audioBypass;
bool m_audioMute;

bool muteOn = false;
float muteOffTransitionTimeInSeconds = 0.02f;
int muteOffTransitionTimeInSamples;
int samplesTilMuteOff;

bool bypassOn = false;
float bypassToggleTransitionTimeInSeconds = 0.01f;
int bypassToggleTransitionTimeInSamples;
int samplesTilBypassToggle;



// Declare a local daisy_petal for hardware access
DaisyPetal hw;
::daisy::Parameter damp, mix, decay, moddepth, modspeed, predelay, expression;
float pdamp, pmix, pdecay, pmoddepth, pmodspeed, ppredelay;
bool      bypass;
Led led1, led2;

float dryMix;
float wetMix;

// Expression
ExpressionHandler expHandler;
bool expression_pressed;

// Midi
bool midi_control[6]; //  just knobs for now

// Control Values
float knobValues[6];
int toggleValues[3];

float pknobValues[6]; // Used for Midi control logic

bool            pswitch1[2], pswitch2[2], pswitch3[2];
int             switch1[2], switch2[2], switch3[2];


Dattorro reverb(48000, 16, 4.0);   // samplerate, max_lfo_depth, max_timescale
int footswitch_mode = 0;
int effect_mode = 0;
bool fw1_held = false;
bool effect_on_momentary = false;
bool freeze = false;

static Decimator2 decimate;
static Interpolator interpolate;
static const auto sample_rate_temp = 48000; //hard code for now                          // NOTE: the sample_rate must be divisible by the resample_factor (48/6 = 8)
static OctaveGenerator octave(sample_rate_temp / resample_factor); // resample_factor is defined in Multirate.h and equals 6
static q::highshelf eq1(-11, 140_Hz, sample_rate_temp);
static q::lowshelf eq2(5, 160_Hz, sample_rate_temp);
float buff[6];
float buff_out[6];
int bin_counter = 0;

float current_predelay, current_moddepth, current_modspeed, current_ODswell, current_freezeDecay;

float setTimeScale, current_timeScale, setOD;

Overdrive overdrive;
Overdrive overdrive2;
bool odOn = false;

bool first_start;


void SetAudioBypass(bool enabled) {
    m_audioBypass = enabled;
    audioBypassTrigger.Write(!m_audioBypass);
}

void SetAudioMute(bool enabled) {
    m_audioMute = enabled;
    audioMuteTrigger.Write(m_audioMute);
}

void InitTrueBypass(Pin relayPin, Pin mutePin) {
    // Init the HW Audio Bypass
    audioBypassTrigger.Init(relayPin, GPIO::Mode::OUTPUT);
    SetAudioBypass(true);

    // Init the HW Audio Mute
    audioMuteTrigger.Init(mutePin, GPIO::Mode::OUTPUT);
    SetAudioMute(false);

}


bool knobMoved(float old_value, float new_value)
{
    float tolerance = 0.005;
    if (new_value > (old_value + tolerance) || new_value < (old_value - tolerance)) {
        return true;
    } else {
        return false;
    }
}

void updateSwitch1()  // 3 size settings, small, medium, large
{

    if (toggleValues[0] == 0) {
        setTimeScale = 1.0;
        
    } else if (toggleValues[0] == 2) {
        setTimeScale = 4.0;

    } else {
        setTimeScale = 2.0;

    }
    reverb.setTimeScale(setTimeScale);

}


void updateSwitch2() 
{

    if (toggleValues[1] == 0) {
        effect_mode = 0;
    } else if (toggleValues[1] == 2) {
        effect_mode = 2;
    } else {
        effect_mode = 1;
    }

}


void updateSwitch3() 
{
    if (toggleValues[2] == 0) {
        footswitch_mode = 0;
    } else if (toggleValues[2] == 2) {
        footswitch_mode = 2;
    } else {
        footswitch_mode = 1;
    }
}


void UpdateButtons()
{
    // (De-)Activate bypass and toggle LED when left footswitch is let go
    if(hw.switches[Soundsketch::FOOTSWITCH_2].FallingEdge())
    {
        if (!expression_pressed && (samplesTilMuteOff < 1)) { // This keeps the pedal from switching bypass when entering/leaving Set Expression mode
            bypass = !bypass;
            led2.Set(bypass ? 0.0f : 1.0f);

            // Mute audio output to remove pop when relay bypass is switched
            // and start countdowns for triggering the relay bypass, and then unmuting audio
            SetAudioMute(true);
            samplesTilMuteOff = muteOffTransitionTimeInSamples;
            samplesTilBypassToggle = bypassToggleTransitionTimeInSamples;
        }
        expression_pressed = false;
    }


    // Toggle Expression mode by holding down both footswitches for half a second
    if(hw.switches[Soundsketch::FOOTSWITCH_2].TimeHeldMs() >= 500 && hw.switches[Soundsketch::FOOTSWITCH_1].TimeHeldMs() >= 500 && !expression_pressed ) {
        expHandler.ToggleExpressionSetMode();

        if (expHandler.isExpressionSetMode()) {
            led1.Set(expHandler.returnLed1Brightness());  // Dim LEDs in expression set mode
            led2.Set(expHandler.returnLed2Brightness());  // Dim LEDs in expression set mode

        } else {
            led2.Set(bypass ? 0.0f : 1.0f); 
            led1.Set(0.0f);  
        }
        expression_pressed = true; // Keeps it from switching over and over while held

    }

    // Clear Expression settings by holding down both footswitches for 2 seconds
    if(hw.switches[Soundsketch::FOOTSWITCH_2].TimeHeldMs() >= 2000 && hw.switches[Soundsketch::FOOTSWITCH_1].TimeHeldMs() >= 2000) {
        expHandler.Reset();
        led2.Set(bypass ? 0.0f : 1.0f); 
        led1.Set(0.0f); 

    }


    // Footswitch momentary action
    if(hw.switches[Soundsketch::FOOTSWITCH_1].RisingEdge() && !expression_pressed && !fw1_held)
    {
        fw1_held = true;
        if (footswitch_mode == 0) {
            freeze = true;
        } else if (footswitch_mode == 1) {
            setOD = 0.6; // TODO Experiment with good max setting here
            odOn = true;
        } else {
            effect_on_momentary = true;
        }

    }

    if(hw.switches[Soundsketch::FOOTSWITCH_1].FallingEdge() && !expression_pressed && fw1_held)
    {
        fw1_held = false;
        freeze = false;
        setOD = 0.4; // TODO Experiment with good minimal setting here
        effect_on_momentary = false;
    }

    led1.Set(fw1_held ? 1.0f : 0.0f); 


}


void UpdateSwitches()
{
    // 3-way Switch 1
    bool changed1 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch1[i]].Pressed() != pswitch1[i]) {
            pswitch1[i] = hw.switches[switch1[i]].Pressed();
            changed1 = true;
        }
    }
    if (changed1 || first_start) { // update_switches is for turning off preset
        if (pswitch1[0] == true) {
            toggleValues[0] = 0;
        } else if (pswitch1[1] == true) {
            toggleValues[0] = 2;
        } else {
            toggleValues[0] = 1;
        }
        updateSwitch1();
    }
    


    // 3-way Switch 2
    bool changed2 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch2[i]].Pressed() != pswitch2[i]) {
            pswitch2[i] = hw.switches[switch2[i]].Pressed();
            changed2 = true;
        }
    }
    if (changed2|| first_start) {
        if (pswitch2[0] == true) {
            toggleValues[1] = 0;
        } else if (pswitch2[1] == true) {
            toggleValues[1] = 2;
        } else {
            toggleValues[1] = 1;
        }
        updateSwitch2();

    }

    // 3-way Switch 3
    bool changed3 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i]) {
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
            changed3 = true;
        }
    }
    if (changed3 || first_start) {
        if (pswitch3[0] == true) {
            toggleValues[2] = 0;
        } else if (pswitch3[1] == true) {
            toggleValues[2] = 2;
        } else {
            toggleValues[2] = 1;
        }
        updateSwitch3();
    }



    first_start = false;
}
    
    


void processSmoothedParameters() {

    // Predelay
    fonepole(current_predelay, ppredelay, .0002f);
    reverb.setPreDelay(current_predelay);

    // Mod Depth
    fonepole(current_moddepth, pmoddepth, .0002f);
    reverb.setTankModDepth(current_moddepth * 8); // was * 16

    // Mod Rate
    fonepole(current_modspeed, pmodspeed, .0002f);
    reverb.setTankModSpeed(0.3 + current_modspeed * 15); // was * 100

    // Size 
    //fonepole(current_timeScale, setTimeScale, .0002f);  // decided not to smooth this, but leaving commented out if I change my mind
    //reverb.setTimeScale(current_timeScale);

    // Swell overdrive footswitch
    if (odOn) {
        fonepole(current_ODswell, setOD, .000015f);  // Gradually swell OD up and back down after releasing footswitch
        overdrive.SetDrive(current_ODswell);
        overdrive2.SetDrive(current_ODswell);
        if (current_ODswell < 0.41 && !fw1_held) {
            odOn = false; // Turn od off after releasing footswitch and drive drops back down
        }
    }

    // Freeze smoothing from current decay to 1.0 and back
    if (freeze) {
        fonepole(current_freezeDecay, 1.0, .0002f); 
    } else {
        fonepole(current_freezeDecay, pdecay, .0002f); 
    }
    reverb.setDecay(current_freezeDecay);


}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    led1.Update();
    led2.Update();

    UpdateButtons();
    UpdateSwitches();


    // Knob and Expression Processing ////////////////////

    // float knobValues[6]; // moved to global
    float newExpressionValues[6];


    // Knob 1
    if (!midi_control[0])   // If not under midi control, use knob ADC
        pknobValues[0] = knobValues[0] = predelay.Process();
    else if (knobMoved(pknobValues[0], predelay.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[0] = false;

    // Knob 2
    if (!midi_control[1])   // If not under midi control, use knob ADC
        pknobValues[1] = knobValues[1] = mix.Process();
    else if (knobMoved(pknobValues[1], mix.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[1] = false;

    // Knob 3
    if (!midi_control[2])   // If not under midi control, use knob ADC
        pknobValues[2] = knobValues[2] = decay.Process();
    else if (knobMoved(pknobValues[2], decay.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[2] = false;

    // Knob 4
    if (!midi_control[3])   // If not under midi control, use knob ADC
        pknobValues[3] = knobValues[3] = moddepth.Process();
    else if (knobMoved(pknobValues[3], moddepth.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[3] = false;


    // Knob 5
    if (!midi_control[4])   // If not under midi control, use knob ADC
        pknobValues[4] = knobValues[4] = modspeed.Process();
    else if (knobMoved(pknobValues[4], modspeed.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[4] = false;


    // Knob 6
    if (!midi_control[5])   // If not under midi control, use knob ADC
        pknobValues[5] = knobValues[5] = damp.Process();
    else if (knobMoved(pknobValues[5], damp.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[5] = false;


    float vexpression = expression.Process(); // 0 is heel (up), 1 is toe (down)
    expHandler.Process(vexpression, knobValues, newExpressionValues);


    // If in expression set mode, set LEDS accordingly
    if (expHandler.isExpressionSetMode()) {
        led1.Set(expHandler.returnLed1Brightness());
        led2.Set(expHandler.returnLed2Brightness());
    }
  

    float vpredelay = newExpressionValues[0];
    float vmix = newExpressionValues[1];
    float vdecay = newExpressionValues[2];
    float vmoddepth = newExpressionValues[3];
    float vmodspeed = newExpressionValues[4];
    float vdamp = newExpressionValues[5];


    if (pmix != vmix) {
        if (knobMoved(pmix, vmix)) {
            //    A cheap mostly energy constant crossfade from SignalSmith Blog
            float x2 = 1.0 - vmix;
            float A = vmix*x2;
            float B = A * (1.0 + 1.4186 * A);
            float C = B + vmix;
            float D = B + x2;

            wetMix = C * C;
            dryMix = D * D;
            pmix = vmix;
        }
    }


    // Set Reverb Parameters ///////////////
    if (knobMoved(ppredelay, vpredelay)) {

        //reverb.setPreDelay(vpredelay);
        ppredelay = vpredelay;
    }


    if (knobMoved(pdecay, vdecay)) {
        //reverb.setDecay(vdecay);
        pdecay = vdecay;
    }

    if (knobMoved(pmoddepth, vmoddepth)) {
        //reverb.setTankModDepth(vmoddepth * 6); // was * 16
        pmoddepth = vmoddepth;
    }

    if (knobMoved(pmodspeed, vmodspeed)) {
        //reverb.setTankModSpeed(0.3 + vmodspeed * 15); // was * 100
        pmodspeed = vmodspeed;
    }

    if (knobMoved(pdamp, vdamp)) {
        // Can also use tank cut functions, try with input cuts and see what sounds better
        if (vdamp < 0.5) {
            float reverbDampHigh = vdamp * 2.0;
            reverb.setInputFilterHighCutoffPitch(7. * reverbDampHigh + 3); // 3 to 10
            //reverb.setTankFilterHighCutFrequency(7. * reverbDampHigh + 3); // 3 to 10
        } else {
            float reverbDampLow = (vdamp - 0.5) * 2.0;
            reverb.setInputFilterLowCutoffPitch(9. * reverbDampLow);  // 0 to 9
            //reverb.setTankFilterLowCutFrequency(9. * reverbDampLow);  // 0 to 9
        }
        pdamp = vdamp;

    }

    // Dipswitch 1 disables input diffusion (turn switch to on position to disable input diffusion)
    //reverb.enableInputDiffusion(!dipValues[0]);

    float inputL;
    float inputR;


    for (size_t i = 0; i < size; i++)
    {

        // Handle true bypass relay
        if (samplesTilMuteOff > 0) {
            samplesTilMuteOff -= 1;
            if (samplesTilMuteOff < 1) {
                SetAudioMute(false);
            }
        }

        if (samplesTilBypassToggle > 0) {
            samplesTilBypassToggle -= 1;
            if (samplesTilBypassToggle < 1) {
                SetAudioBypass(bypass);
            }
        }

        if(!bypass) {

            processSmoothedParameters();

            inputL = inputR = in[0][i];
 
            // NOTE: Octave before reverb sounds better (personal preference), and doing octave after reverb would require another polyoctave for second channel anyway
            buff[bin_counter] = inputL;
            // do calculation every 6 samples
            if (bin_counter > 4) {

                std::span<const float, resample_factor> in_chunk(&(buff[0]), resample_factor);  // std::span is c++ 20 feature
                    
                const auto sample = decimate(in_chunk); 

                float octave_mix = 0.0;
                octave.update(sample);

                if (effect_mode != 0)
                    octave_mix += octave.up1() * 2.0;
                if (effect_mode == 2) {
                    octave_mix += octave.down1() * 2.0;
                    octave_mix += octave.down2() * 2.0;
                }

                auto out_chunk = interpolate(octave_mix);
                for (size_t j = 0; j < out_chunk.size(); ++j)
                {
                    float mix = eq2(eq1(out_chunk[j]));

                    const auto dry_signal = buff[j];
                  
                    float dryLevel = 0.5;
                    if (effect_mode == 2) // Dont add in dry mix if dip3 switch is on, but always add if in effect mode 2 (momentary octave)
                        mix += dryLevel * buff[j];
                    if (effect_mode != 0)
                        buff_out[j] = mix;
                    else 
                        buff_out[j] = 0.0;
                }

            }

            // Increments the buffer index from 0 to 5 (workaround to adapt code)
            bin_counter += 1;
            if (bin_counter > 5)
                bin_counter = 0;


            float reverb_in = inputL;

            if (effect_mode != 0 ) { // Up oct or down oct
                if ((footswitch_mode == 2 && effect_on_momentary) || (footswitch_mode != 2)) {
                    //reverb_in = inputL + upOct;
                    reverb_in = buff_out[bin_counter]; // This adds 6 samples of latency to the octave sound
                }

            }

            // Calculate Reverb
            reverb.process(reverb_in, reverb_in);

            // Momentary Overdrive Swell
            float reverbLeftOut = reverb.getLeftOutput();  
            float reverbRightOut = reverb.getRightOutput();
            float effectLeftOut = 0.0;
            float effectRightOut = 0.0;

            if (odOn) {
                // Really cool sound when the low octave is overdriven, like epic sci fi blade runner
                effectLeftOut = overdrive.Process(reverbLeftOut*0.25) *  (1.0 - (current_ODswell * current_ODswell * 2.8 - 0.1296)); // reduce volume as od drive goes up (otherwise way too loud)
                effectRightOut = overdrive2.Process(reverbRightOut*0.25) *  (1.0 - (current_ODswell * current_ODswell * 2.8 - 0.1296));
              
            } else {
                effectLeftOut = reverb.getLeftOutput();
                effectRightOut = reverb.getRightOutput();
            }

            float leftOutput = inputL * dryMix + effectLeftOut * wetMix * 0.4;  // 0.4 is for overall volume reduction on reverb
            float rightOutput = inputR * dryMix + effectRightOut * wetMix* 0.4;

            out[0][i] = leftOutput;
            out[1][i] = rightOutput;


        } else { // else bypassed

            inputL = in[0][i];
            inputR = in[0][i];

            out[0][i] = inputL;
            out[1][i] = inputR;

        }
    }
}


// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
 
            //led2.Set(1.0); // TODO Simple test to see if midi note is detected
            //led2.Update();
            NoteOnEvent p = m.AsNoteOn();
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                // Do stuff with the midi Note/Velocity info here
                //osc.SetFreq(mtof(p.note));
                //osc.SetAmp((p.velocity / 127.0f));
            }
        }
        break;
        case ControlChange:
        {

            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 14:
                    midi_control[0] = true;
                    knobValues[0] = ((float)p.value / 127.0f);
                    break;
                case 15:
                    midi_control[1] = true;
                    knobValues[1] = ((float)p.value / 127.0f);
                    break;
                case 16:
                    midi_control[2] = true;
                    knobValues[2] = ((float)p.value / 127.0f);
                    break;
                case 17:
                    midi_control[3] = true;
                    knobValues[3] = ((float)p.value / 127.0f);
                    break;
                case 18:
                    midi_control[4] = true;
                    knobValues[4] = ((float)p.value / 127.0f);
                    break;
                case 19:
                    midi_control[5] = true;
                    knobValues[5] = ((float)p.value / 127.0f);
                    break;


                default: break;
            }
            break;
        }
        default: break;
    }
}



int main(void)
{
    float samplerate;

    hw.Init(true);
    //hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
    hw.SetAudioBlockSize(48);
    samplerate = hw.AudioSampleRate();

    float inputDampLow = 0.;
    float inputDampHigh = 0.;
    float reverbDampLow = 0.;
    float reverbDampHigh = 0.;
    float diffusion = 1.;

    reverb.setSampleRate(samplerate);

    reverb.setTimeScale(4.0);
    reverb.setPreDelay(0.0);

    reverb.setInputFilterLowCutoffPitch(10. * inputDampLow);
    reverb.setInputFilterHighCutoffPitch(10. - (10. * inputDampHigh));
    reverb.enableInputDiffusion(true);
    reverb.setDecay(0.877465);
    reverb.setTankDiffusion(diffusion * 0.7);
    reverb.setTankFilterLowCutFrequency(10. * reverbDampLow);
    reverb.setTankFilterHighCutFrequency(10. - (10. * reverbDampHigh));
    reverb.setTankModSpeed(1.0);
    reverb.setTankModDepth(0.5);
    reverb.setTankModShape(0.5); // <-- currently not controllable, maybe use dipswitch for different shape
    reverb.clear();

    // Initialize buffers for polyoctave to 0
    for (int j = 0; j < 6; ++j) {
        buff[j] = 0.0;
        buff_out[j] = 0.0;
    }

    overdrive.Init();
    overdrive.SetDrive(0.4);
    overdrive2.Init();
    overdrive2.SetDrive(0.4);

    predelay.Init(hw.knob[Soundsketch::KNOB_1], 0.0f, 1.0f, ::daisy::Parameter::LINEAR); 
    mix.Init(hw.knob[Soundsketch::KNOB_2], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    decay.Init(hw.knob[Soundsketch::KNOB_3], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    moddepth.Init(hw.knob[Soundsketch::KNOB_4], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    modspeed.Init(hw.knob[Soundsketch::KNOB_5], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    damp.Init(hw.knob[Soundsketch::KNOB_6], 0.0f, 1.0f, ::daisy::Parameter::LINEAR);
    expression.Init(hw.expression, 0.0f, 1.0f, Parameter::LINEAR); 

    pdamp = 0.0;
    pmix = 0.0;
    pdecay = 0.0;
    pmoddepth = 0.0;
    pmodspeed = 0.0;
    ppredelay = 0.0;

    // For parameter smoothing
    current_predelay = current_moddepth = current_modspeed = current_freezeDecay = 0.0;
    current_ODswell= 0.4;
    setOD = 0.4;

    switch1[0]= Soundsketch::SWITCH_1_UP;
    switch1[1]= Soundsketch::SWITCH_1_DOWN;
    switch2[0]= Soundsketch::SWITCH_2_UP;
    switch2[1]= Soundsketch::SWITCH_2_DOWN;
    switch3[0]= Soundsketch::SWITCH_3_UP;
    switch3[1]= Soundsketch::SWITCH_3_DOWN;


    pswitch1[0]= false;
    pswitch1[1]= false;
    pswitch2[0]= false;
    pswitch2[1]= false;
    pswitch3[0]= false;
    pswitch3[1]= false;

    // True Bypass
    bypassToggleTransitionTimeInSamples = samplerate * bypassToggleTransitionTimeInSeconds;
    muteOffTransitionTimeInSamples = samplerate * muteOffTransitionTimeInSeconds;
    InitTrueBypass(seed::D1, seed::D12); // (relaypin=D1, mutepin=D12)

    samplesTilMuteOff = 0;
    samplesTilBypassToggle = 0;

    // Expression
    expHandler.Init(6);
    expression_pressed = false;

    // Midi
    for( int i = 0; i < 6; ++i ) 
        midi_control[i] = false;  // Is this needed? or does it default to false
    // index for midi_control: 0-5 knobs, 6 expression


    first_start = true;

    // Init the LEDs and set activate bypass
    led1.Init(hw.seed.GetPin(Soundsketch::LED_1),false);
    led1.Update();

    led2.Init(hw.seed.GetPin(Soundsketch::LED_2),false);
    led2.Update();
    bypass = true;

    hw.InitMidi();
    hw.midi.StartReceive();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {

        hw.midi.Listen();
        // Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }

	    System::Delay(100);
    }
}