#include "daisy_petal.h"
#include "daisysp.h"
#include "soundsketch.h"
//#include <RTNeural/RTNeural.h>
#include <q/fx/biquad.hpp>
//#include "wavenet/wavenet_model.hpp"
#include "expressionHandler.h"

#define LAYER_ARRAY_BUFFER_PADDING 0
//#define WAVENET_MAX_NUM_FRAMES 1  // 64 is default
#include "NeuralAudio/WaveNet.h"

// Model Weights (edit this file to add model weights trained with Colab script)
#include "model_data_nam.h"


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
Parameter gain, level, presence, bass, mid, treble, expression;
bool            bypass;
int             modelIndex = 0;
int             m_currentModelindex = -1;
bool            pswitch1[2], pswitch2[2], pswitch3[2];
int             switch1[2], switch2[2], switch3[2];
float           nnLevelAdjust;

bool            eqOn = true;

float knobValues[6]; // Moved to global
int toggleValues[3];

// Midi
bool midi_control[6]; //  just knobs for now
float pknobValues[6]; // Used for Midi control logic

float dryMix, wetMix;
bool silence_output = false;
float setPopReduce, popReduce;

Led led1, led2;


// Expression
ExpressionHandler expHandler;
bool expression_pressed;



constexpr uint8_t NUM_FILTERS_NAM = 4;

const float minGain = -10.f;
const float maxGain = 10.f;

const float centerFrequencyNam[NUM_FILTERS_NAM] = {180.f, 1200.f, 4000.f, 8000.f}; // Experiment with these freqs and q values
const float q_nam[NUM_FILTERS_NAM] = {0.7f, 0.6f, 0.5f, 0.5f};

cycfi::q::peaking filter_nam[NUM_FILTERS_NAM] = {{0, centerFrequencyNam[0], 48000, q_nam[0]}, {0, centerFrequencyNam[1], 48000, q_nam[1]}, {0, centerFrequencyNam[2], 48000, q_nam[2]}, {0, centerFrequencyNam[3], 48000, q_nam[3]}};


using namespace NeuralAudio;

// NeuralAudio WaveNet Nano /////////////////////////////////////////////////
//NeuralAudio::WaveNetModelT<
//    NeuralAudio::WaveNetLayerArrayT<1, 1, headSize, numChannels, 3, ILiteDilations1, false>,
//    NeuralAudio::WaveNetLayerArrayT<numChannels, 1, 1, headSize, 3, ILiteDilations2, true>>

using ILiteDilations1 = NeuralAudio::Dilations<1, 2, 4, 8, 16, 32, 64>;
using ILiteDilations2 = NeuralAudio::Dilations<128, 256, 512, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512>;

NeuralAudio::WaveNetModelT<
    NeuralAudio::WaveNetLayerArrayT<1, 1, 2, 4, 3, ILiteDilations1, false>,
    NeuralAudio::WaveNetLayerArrayT<4, 1, 1, 2, 3, ILiteDilations2, true>> neural_audio_wavenet;
/////////////////////////////////////////////////////////////////////////////


//pico
//using ILiteDilations1 = NeuralAudio::Dilations<1, 2, 4, 8, 16, 32, 64>;
//using ILiteDilations2 = NeuralAudio::Dilations<128, 256, 512, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512>;

//NeuralAudio::WaveNetModelT<
//    NeuralAudio::WaveNetLayerArrayT<1, 1, 2, 2, 3, ILiteDilations1, false>,
//    NeuralAudio::WaveNetLayerArrayT<2, 1, 1, 2, 3, ILiteDilations2, true>> neural_audio_wavenet;



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

void SelectModel()
{

    if (m_currentModelindex != modelIndex) {
        //rtneural_wavenet.load_weights (model_collection_nam[modelIndex].weights);

        neural_audio_wavenet.SetWeights(model_collection_nam[modelIndex].weights); // TODO only choosing one model here

        //static constexpr size_t N = 1; // number of samples sent through model at once
        //rtneural_wavenet.prepare (N); // This is needed, including this allowed the led to come on before freezing
        //rtneural_wavenet.prewarm();  // Note: looks like this just sends some 0's through the model
        m_currentModelindex = modelIndex;
    }

}

void updateSwitch1or2() 
{

    if (toggleValues[0] == 0) {         // low gain models
        if (toggleValues[1] == 0) {
            modelIndex = 0;
            nnLevelAdjust = 1.3;
        } else if (toggleValues[1] == 1) {
            modelIndex = 1;
            nnLevelAdjust = 1.6;
        } else {
            modelIndex = 2;
            nnLevelAdjust = 1.1;
        }
    } else if (toggleValues[0] == 1) {  // med gain models
        if (toggleValues[1] == 0) {
            modelIndex = 3;
            nnLevelAdjust = 1.0;
        } else if (toggleValues[1] == 1) {
            modelIndex = 4;
            nnLevelAdjust = 1.0;
        } else {
            modelIndex = 5;
            nnLevelAdjust = 0.7;
        }
    } else {                            // high gain models
        if (toggleValues[1] == 0) {
            modelIndex = 6;
            nnLevelAdjust = 0.9;
        } else if (toggleValues[1] == 1) {
            modelIndex = 7;
            nnLevelAdjust = 0.9;
        } else {
            modelIndex = 8;
            nnLevelAdjust = 0.9;
        }
    }
    silence_output = true;
    popReduce = 1.0;
    setPopReduce = 0.0;
    SelectModel();
}



void updateSwitch3() 
{
    if (toggleValues[2] == 0) {

    } else if (toggleValues[2] == 2) {

    } else {

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


    led1.Update();
    led2.Update();

}


void UpdateSwitches()
{
    // Detect any changes in switch positions

    // 3-way Switch 1
    bool changed1 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch1[i]].Pressed() != pswitch1[i]) {
            pswitch1[i] = hw.switches[switch1[i]].Pressed();
            changed1 = true;
        }
    }
    if (changed1) { // update_switches is for turning off preset
        if (pswitch1[0] == true) {
            toggleValues[0] = 0;
        } else if (pswitch1[1] == true) {
            toggleValues[0] = 2;
        } else {
            toggleValues[0] = 1;
        }
        updateSwitch1or2();
    }
    


    // 3-way Switch 2
    bool changed2 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch2[i]].Pressed() != pswitch2[i]) {
            pswitch2[i] = hw.switches[switch2[i]].Pressed();
            changed2 = true;
        }
    }
    if (changed2) {
        if (pswitch2[0] == true) {
            toggleValues[1] = 0;
        } else if (pswitch2[1] == true) {
            toggleValues[1] = 2;
        } else {
            toggleValues[1] = 1;
        }
        updateSwitch1or2();

    }

    // 3-way Switch 3
    bool changed3 = false;
    for(int i=0; i<2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i]) {
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
            changed3 = true;
        }
    }
    if (changed3) {
        if (pswitch3[0] == true) {
            toggleValues[2] = 0;
        } else if (pswitch3[1] == true) {
            toggleValues[2] = 2;
        } else {
            toggleValues[2] = 1;
        }
        updateSwitch3();
    }


}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    //hw.ProcessAllControls();
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    UpdateButtons();
    UpdateSwitches();

    // Knob and Expression Processing ////////////////////

    float newExpressionValues[6];


    // Knob 1
    if (!midi_control[0])   // If not under midi control, use knob ADC
        pknobValues[0] = knobValues[0] = gain.Process();
    else if (knobMoved(pknobValues[0], gain.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[0] = false;

    // Knob 2
    if (!midi_control[1])   // If not under midi control, use knob ADC
        pknobValues[1] = knobValues[1] = level.Process();
    else if (knobMoved(pknobValues[1], level.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[1] = false;

    // Knob 3
    if (!midi_control[2])   // If not under midi control, use knob ADC
        pknobValues[2] = knobValues[2] = presence.Process();
    else if (knobMoved(pknobValues[2], presence.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[2] = false;

    // Knob 4
    if (!midi_control[3])   // If not under midi control, use knob ADC
        pknobValues[3] = knobValues[3] = bass.Process();
    else if (knobMoved(pknobValues[3], bass.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[3] = false;


    // Knob 5
    if (!midi_control[4])   // If not under midi control, use knob ADC
        pknobValues[4] = knobValues[4] = mid.Process();
    else if (knobMoved(pknobValues[4], mid.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[4] = false;


    // Knob 6
    if (!midi_control[5])   // If not under midi control, use knob ADC
        pknobValues[5] = knobValues[5] = treble.Process();
    else if (knobMoved(pknobValues[5], treble.Process()))  // If midi controlled, watch for knob movement to end Midi control
        midi_control[5] = false;


    float vexpression = expression.Process(); // 0 is heel (up), 1 is toe (down)
    expHandler.Process(vexpression, knobValues, newExpressionValues);

    // If in expression set mode, set LEDS accordingly
    if (expHandler.isExpressionSetMode()) {
        led1.Set(expHandler.returnLed1Brightness());
        led2.Set(expHandler.returnLed2Brightness());
    }

    float vgain = newExpressionValues[0] * 2.0;
    float vlevel = newExpressionValues[1];
    float vpresence = newExpressionValues[2] * 20.0 - 10.0;  // Make eq control range from -10 to +10 dB
    float vbass = newExpressionValues[3] * 20.0 - 10.0;
    float vmid = newExpressionValues[4] * 20.0 - 10.0;
    float vtreble = newExpressionValues[5] * 20.0 - 10.0;

    // Order of effects is:
    //           Gain -> Neural Model -> Tone -> 
    //

    // Bass, Mid, Treble, Presence 
    filter_nam[0].config(vbass, centerFrequencyNam[0], 48000, q_nam[0]);
    filter_nam[1].config(vmid, centerFrequencyNam[1], 48000, q_nam[1]);
    filter_nam[2].config(vtreble, centerFrequencyNam[2], 48000, q_nam[2]);
    filter_nam[3].config(vpresence, centerFrequencyNam[3], 48000, q_nam[3]);

    float input[64];
    float output[64];

    for(size_t i = 0; i < size; i++)
    {
        // Process your signal here
        //float in_temp = in[0][i] * vgain;
        input[i] = in[0][i] * vgain;
    }

    neural_audio_wavenet.Process(input, output, 64);

    for(size_t i = 0; i < size; i++)
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

        if (!bypass) {

            // Apply 4 band EQ
            float ampOut = output[i];

            // NOTE: For some reason, including more than 2 different models (rather than 9 identical modals)
            //       Causes slowdown, taking out the EQ filters keeps it up to speed without dropouts for 
            //         (tested) at least 3 different models TODO be able to use the 4 band EQ with 9 different models

            for (uint8_t i = 0; i < NUM_FILTERS_NAM; i++) {
                ampOut = filter_nam[i](ampOut);
            }


            out[0][i] = out[1][i] = ampOut * vlevel; 

        } else {
            out[0][i] = out[1][i] = in[0][i];
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
    samplerate = hw.AudioSampleRate();

    setupWeightsNam();
    //hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ); // Currently needs to run at 32kHz and block size 256 to keep up with processing
    hw.SetAudioBlockSize(64);

    switch1[0]= Soundsketch::SWITCH_1_UP;
    switch1[1]= Soundsketch::SWITCH_1_DOWN;
    switch2[0]= Soundsketch::SWITCH_2_UP;
    switch2[1]= Soundsketch::SWITCH_2_DOWN;
    switch3[0]= Soundsketch::SWITCH_3_UP;
    switch3[1]= Soundsketch::SWITCH_3_DOWN;

    pswitch1[0]= true;
    pswitch1[1]= true;
    pswitch2[0]= true;
    pswitch2[1]= true;
    pswitch3[0]= true;
    pswitch3[1]= true;

    // True Bypass
    bypassToggleTransitionTimeInSamples = samplerate * bypassToggleTransitionTimeInSeconds;
    muteOffTransitionTimeInSamples = samplerate * muteOffTransitionTimeInSeconds;
    InitTrueBypass(seed::D1, seed::D12); // (relaypin=D1, mutepin=D12)

    samplesTilMuteOff = 0;
    samplesTilBypassToggle = 0;


    setupWeightsNam(); // in the model data nam .h file
    //updateSwitch1or2();
    SelectModel();
    setPopReduce = 1.0;
    popReduce = 1.0;

    filter_nam[0].config(0.0, centerFrequencyNam[0], samplerate, q_nam[0]);
    filter_nam[1].config(0.0, centerFrequencyNam[1], samplerate, q_nam[1]);
    filter_nam[2].config(0.0, centerFrequencyNam[2], samplerate, q_nam[2]);


    gain.Init(hw.knob[Soundsketch::KNOB_1], 0.1f, 2.5f, Parameter::LINEAR);
    level.Init(hw.knob[Soundsketch::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    presence.Init(hw.knob[Soundsketch::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    bass.Init(hw.knob[Soundsketch::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
    mid.Init(hw.knob[Soundsketch::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    treble.Init(hw.knob[Soundsketch::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR); 
    expression.Init(hw.expression, 0.0f, 1.0f, Parameter::LINEAR); // TODO Make sure this is the correct way to reference expression

    // Initialize the correct model
    modelIndex = 0;
    nnLevelAdjust = 1.0; // TODO Use level adjust to get model volumes even

    // Expression
    expHandler.Init(6);
    expression_pressed = false;

    // Midi
    for( int i = 0; i < 6; ++i ) 
        midi_control[i] = false;  // Is this needed? or does it default to false
    // index for midi_control: 0-5 knobs

    // Init the LEDs and set activate bypass
    led1.Init(hw.seed.GetPin(Soundsketch::LED_1),false);
    led1.Update();
    bypass = true;

    led2.Init(hw.seed.GetPin(Soundsketch::LED_2),false);
    led2.Update();

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