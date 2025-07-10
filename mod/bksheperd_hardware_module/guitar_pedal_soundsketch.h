#pragma once
#ifndef GUITAR_PEDAL_SOUNDSKETCH_H
#define GUITAR_PEDAL_SOUNDSKETCH_H /**< & */

#include "base_hardware_module.h"

#ifdef __cplusplus

/** @file guitar_pedal_soundsketch.h */

using namespace daisy;

namespace bkshepherd {

/**
   @brief Helpers and hardware definitions for the SoundSketch pedal platform by GuitarML.
*/
class GuitarPedalSoundSketch : public BaseHardwareModule
{
  public:
    GuitarPedalSoundSketch();
    ~GuitarPedalSoundSketch();
    void Init(bool boost = false) override;
};
} // namespace bkshepherd
#endif
#endif