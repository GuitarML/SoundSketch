#include "guitar_pedal_soundsketch.h"

using namespace bkshepherd;

static const int s_switchParamCount = 2;
static const PreferredSwitchMetaData s_switchMetaData[s_switchParamCount] = {{sfType: SpecialFunctionType::Bypass, switchMapping: 0},
                                                                            {sfType: SpecialFunctionType::TapTempo, switchMapping: 1}}; 

GuitarPedalSoundSketch::GuitarPedalSoundSketch() : BaseHardwareModule()
{
    // Setup the Switch Meta Data for this hardware
    m_switchMetaDataParamCount = s_switchParamCount;
    m_switchMetaData = s_switchMetaData;
}

GuitarPedalSoundSketch::~GuitarPedalSoundSketch()
{

}

void GuitarPedalSoundSketch::Init(bool boost)
{
    BaseHardwareModule::Init(boost);

    m_supportsStereo = true;

    Pin knobPins[] = {seed::D15, seed::D16, seed::D17, seed::D18, seed::D19, seed::D20, seed::D21}; // seed::D21, the 7th knobPin is for Expression pedal input
    InitKnobs(7, knobPins);

    // Index reference: {0:footswitch1, 1:footswitch2, 2:Switch1Up, 3:Switch1Down, 4:Switch1Up, 5:Switch2Down, 6:Switch3Up, 7:Switch3Down}
    Pin switchPins[] = {seed::D6, seed::D5, seed::D24, seed::D25, seed::D26, seed::D27, seed::D10, seed::D11};
    
    InitSwitches(8, switchPins);

    Pin ledPins[] = {seed::D23, seed::D22};
    InitLeds(2, ledPins);

    InitMidi(seed::D30, seed::D29);
}
