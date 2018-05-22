#include "braids/digital_oscillator.h"

#include <algorithm>
#include <cstdio>

#include "stmlib/utils/dsp.h"
#include "stmlib/utils/random.h"

#include "braids/parameter_interpolation.h"
#include "braids/resources.h"
#include "braids/quantizer.h"

extern braids::Quantizer quantizer;

namespace braids {

using namespace stmlib;

const int kStackSize = 6;

const uint8_t amplitudes[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  214,  171,   128,   86,   43,   43,
  42,    85,   128,  170,  213,  255,
  171,   86,     0,   85,  170,  255,
  171,   86,     0,   85,  170,  255,
    0,  255,     0,  255,    0,  255, 
    0,    0,     0,  255,  255,    0, 
    0,  255,   255,  255,    0,    0,
  221,  127,    71,    8,  197,   19,
   98,  137,    55,   70,  122,  153,
   50,  203,    43,  198,
};

inline void DigitalOscillator::renderChordSine(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint32_t *phase_increment, 
  uint8_t noteCount) {
  
  uint32_t phase_0, phase_1, phase_2, phase_3, phase_4, phase_5;

  phase_0 = state_.stack.phase[0];
  phase_1 = state_.stack.phase[1];
  phase_2 = state_.stack.phase[2];
  phase_3 = state_.stack.phase[3];
  phase_4 = state_.stack.phase[4];
  phase_5 = state_.stack.phase[5];

  while (size) {
    int32_t sample = 0;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = Interpolate824(wav_sine, phase_0) >> 2;
    sample += Interpolate824(wav_sine, phase_1) >> 2;
    sample += Interpolate824(wav_sine, phase_2) >> 2;
    sample += Interpolate824(wav_sine, phase_3) >> 2;

    if (noteCount > 4) {
      sample += Interpolate824(wav_sine, phase_4) >> 2;
    }
    if (noteCount > 5) {
      sample += Interpolate824(wav_sine, phase_5) >> 2;
    }

    sample = (sample >> 2) + (sample >> 5);
    CLIP(sample)
    *buffer++ = sample;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = Interpolate824(wav_sine, phase_0) >> 2;
    sample += Interpolate824(wav_sine, phase_1) >> 2;
    sample += Interpolate824(wav_sine, phase_2) >> 2;
    sample += Interpolate824(wav_sine, phase_3) >> 2;

    if (noteCount > 4) {
      sample += Interpolate824(wav_sine, phase_4) >> 2;
    }
    if (noteCount > 5) {
      sample += Interpolate824(wav_sine, phase_5) >> 2;
    }

    sample = (sample >> 2) + (sample >> 5);
    CLIP(sample)

    *buffer++ = sample;

    size -= 2;
  }
  
  state_.stack.phase[0] = phase_0;
  state_.stack.phase[1] = phase_1;
  state_.stack.phase[2] = phase_2;
  state_.stack.phase[3] = phase_3;
  state_.stack.phase[4] = phase_4;
  state_.stack.phase[5] = phase_5;
}

inline void DigitalOscillator::renderChordSaw(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint32_t *phase_increment, 
  uint8_t noteCount) {

  uint32_t phase_0, phase_1, phase_2, phase_3, phase_4, phase_5;

  phase_0 = state_.stack.phase[0];
  phase_1 = state_.stack.phase[1];
  phase_2 = state_.stack.phase[2];
  phase_3 = state_.stack.phase[3];
  phase_4 = state_.stack.phase[4];
  phase_5 = state_.stack.phase[5];

  while (size) {
    int32_t sample = 0;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = (1 << 15) - (phase_0 >> 16);
    sample += (1 << 15) - (phase_1 >> 16);
    sample += (1 << 15) - (phase_2 >> 16);
    sample += (1 << 15) - (phase_3 >> 16);

    if (noteCount > 4) {
      sample += (1 << 15) - (phase_4 >> 16);
    }
    if (noteCount > 5) {
      sample += (1 << 15) - (phase_5 >> 16);
    }

    sample = (sample >> 2) + (sample >> 5);
    CLIP(sample)
    *buffer++ = sample;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = (1 << 15) - (phase_0 >> 16);
    sample += (1 << 15) - (phase_1 >> 16);
    sample += (1 << 15) - (phase_2 >> 16);
    sample += (1 << 15) - (phase_3 >> 16);

    if (noteCount > 4) {
      sample += (1 << 15) - (phase_4 >> 16);
    }
    if (noteCount > 5) {
      sample += (1 << 15) - (phase_5 >> 16);
    }

    sample = (sample >> 2) + (sample >> 5);
    CLIP(sample)
    *buffer++ = sample;

    size -= 2;
  }
  
  state_.stack.phase[0] = phase_0;
  state_.stack.phase[1] = phase_1;
  state_.stack.phase[2] = phase_2;
  state_.stack.phase[3] = phase_3;
  state_.stack.phase[4] = phase_4;
  state_.stack.phase[5] = phase_5;
}

#define CALC_TRIANGLE(x) (0x4000 + ((x & 0x80000000) ? ((1 << 15) - ((x >> 16) & 0xffff)) : ((x >> 16) - (1<<15))))

inline void DigitalOscillator::renderChordTriangle(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint32_t *phase_increment, 
  uint8_t noteCount) {
  uint32_t phase_0, phase_1, phase_2, phase_3, phase_4, phase_5;

  phase_0 = state_.stack.phase[0];
  phase_1 = state_.stack.phase[1];
  phase_2 = state_.stack.phase[2];
  phase_3 = state_.stack.phase[3];
  phase_4 = state_.stack.phase[4];
  phase_5 = state_.stack.phase[5];

  while (size) {
    int32_t sample = 0;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = CALC_TRIANGLE(phase_0);
    sample += CALC_TRIANGLE(phase_1);
    sample += CALC_TRIANGLE(phase_2);
    sample += CALC_TRIANGLE(phase_3);

    if (noteCount > 4) {
      sample += CALC_TRIANGLE(phase_4);
    }
    if (noteCount > 5) {
      sample += CALC_TRIANGLE(phase_5);
    }

    sample = (sample >> 2) + (sample >> 3);
    CLIP(sample)
    *buffer++ = sample;

    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = CALC_TRIANGLE(phase_0);
    sample += CALC_TRIANGLE(phase_1);
    sample += CALC_TRIANGLE(phase_2);
    sample += CALC_TRIANGLE(phase_3);

    if (noteCount > 4) {
      sample += CALC_TRIANGLE(phase_4);
    }
    if (noteCount > 5) {
      sample += CALC_TRIANGLE(phase_5);
    }

    sample = (sample >> 2) + (sample >> 3);
    CLIP(sample)
    *buffer++ = sample;

    size -= 2;
  }
  
  state_.stack.phase[0] = phase_0;
  state_.stack.phase[1] = phase_1;
  state_.stack.phase[2] = phase_2;
  state_.stack.phase[3] = phase_3;
  state_.stack.phase[4] = phase_4;
  state_.stack.phase[5] = phase_5;
}

#define CALC_SQUARE(x, width) ((x > width) ? 5400 : -5400)

inline void DigitalOscillator::renderChordSquare(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint32_t *phase_increment, 
  uint8_t noteCount) {

  uint32_t phase_0, phase_1, phase_2, phase_3, phase_4, phase_5;
  uint32_t pw = parameter_[0] << 16;

  phase_0 = state_.stack.phase[0];
  phase_1 = state_.stack.phase[1];
  phase_2 = state_.stack.phase[2];
  phase_3 = state_.stack.phase[3];
  phase_4 = state_.stack.phase[4];
  phase_5 = state_.stack.phase[5];

  while (size) {
    int32_t sample = 0;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];

    sample = CALC_SQUARE(phase_0, pw);
    sample += CALC_SQUARE(phase_1, pw);
    sample += CALC_SQUARE(phase_2, pw);
    sample += CALC_SQUARE(phase_3, pw);

    if (noteCount > 4) {
      sample += CALC_SQUARE(phase_4, pw);
    }
    if (noteCount > 5) {
      sample += CALC_SQUARE(phase_5, pw);
    }

    CLIP(sample)
    *buffer++ = sample;

    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];

    sample = CALC_SQUARE(phase_0, pw);
    sample += CALC_SQUARE(phase_1, pw);
    sample += CALC_SQUARE(phase_2, pw);
    sample += CALC_SQUARE(phase_3, pw);

    if (noteCount > 4) {
      sample += CALC_SQUARE(phase_4, pw);
    }
    if (noteCount > 5) {
      sample += CALC_SQUARE(phase_5, pw);
    }

    CLIP(sample)
    *buffer++ = sample;

    size -= 2;
  }
  
  state_.stack.phase[0] = phase_0;
  state_.stack.phase[1] = phase_1;
  state_.stack.phase[2] = phase_2;
  state_.stack.phase[3] = phase_3;
  state_.stack.phase[4] = phase_4;
  state_.stack.phase[5] = phase_5;
}

const uint8_t mini_wave_line[] = {
  157, 161, 171, 188, 189, 191, 192, 193, 196, 198, 201, 234, 232,
  229, 226, 224, 1, 2, 3, 4, 5, 8, 12, 32, 36, 42, 47, 252, 254, 141, 139,
  135, 174
};

#define SEMI * 128

const uint16_t chords[17][3] = {
  { 2, 4, 6 },
  { 16, 32, 48 },
  { 2 SEMI, 7 SEMI, 12 SEMI },
  { 3 SEMI, 7 SEMI, 10 SEMI },
  { 3 SEMI, 7 SEMI, 12 SEMI },
  { 3 SEMI, 7 SEMI, 14 SEMI },
  { 3 SEMI, 7 SEMI, 17 SEMI },
  { 7 SEMI, 12 SEMI, 19 SEMI },
  { 7 SEMI, 3 + 12 SEMI, 5 + 19 SEMI },
  { 4 SEMI, 7 SEMI, 17 SEMI },
  { 4 SEMI, 7 SEMI, 14 SEMI },
  { 4 SEMI, 7 SEMI, 12 SEMI },
  { 4 SEMI, 7 SEMI, 11 SEMI },
  { 5 SEMI, 7 SEMI, 12 SEMI },
  { 4, 7 SEMI, 12 SEMI },
  { 4, 4 + 12 SEMI, 12 SEMI },
  { 4, 4 + 12 SEMI, 12 SEMI },
};

inline void DigitalOscillator::renderChordWavetable(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint32_t *phase_increment, 
  uint8_t noteCount) {

  const uint8_t* wave_1 = wt_waves + mini_wave_line[parameter_[0] >> 10] * 129;
  const uint8_t* wave_2 = wt_waves + mini_wave_line[(parameter_[0] >> 10) + 1] * 129;
  uint16_t wave_xfade = parameter_[0] << 6;
  uint32_t phase_0, phase_1, phase_2, phase_3, phase_4, phase_5;

  phase_0 = state_.stack.phase[0];
  phase_1 = state_.stack.phase[1];
  phase_2 = state_.stack.phase[2];
  phase_3 = state_.stack.phase[3];
  phase_4 = state_.stack.phase[4];
  phase_5 = state_.stack.phase[5];

  while (size) {
    int32_t sample = 0;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = Crossfade(wave_1, wave_2, phase_0 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_1 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_2 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_3 >> 1, wave_xfade);
    if (noteCount > 4) {
      sample += Crossfade(wave_1, wave_2, phase_4 >> 1, wave_xfade);
    }
    if (noteCount > 5) {
      sample += Crossfade(wave_1, wave_2, phase_5 >> 1, wave_xfade);
    }

    sample = (sample >> 2) + (sample >> 6);
    CLIP(sample)
    *buffer++ = sample;
    
    phase_0 += phase_increment[0];
    phase_1 += phase_increment[1];
    phase_2 += phase_increment[2];
    phase_3 += phase_increment[3];
    phase_4 += phase_increment[4];
    phase_5 += phase_increment[5];

    sample = Crossfade(wave_1, wave_2, phase_0 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_1 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_2 >> 1, wave_xfade);
    sample += Crossfade(wave_1, wave_2, phase_3 >> 1, wave_xfade);
    if (noteCount > 4) {
      sample += Crossfade(wave_1, wave_2, phase_4 >> 1, wave_xfade);
    }
    if (noteCount > 5) {
      sample += Crossfade(wave_1, wave_2, phase_5 >> 1, wave_xfade);
    }

    sample = (sample >> 2) + (sample >> 6);
    CLIP(sample)
    *buffer++ = sample;

    size -= 2;
  }
  
  state_.stack.phase[0] = phase_0;
  state_.stack.phase[1] = phase_1;
  state_.stack.phase[2] = phase_2;
  state_.stack.phase[3] = phase_3;
  state_.stack.phase[4] = phase_4;
  state_.stack.phase[5] = phase_5;
}

extern const uint16_t chords[17][3];

inline void DigitalOscillator::renderChord(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint8_t* noteOffset, 
  uint8_t noteCount) {

  uint32_t fm = 0;

  if (strike_) {
    for (size_t i = 0; i < kStackSize; ++i) {
      state_.stack.phase[i] = Random::GetWord();
    }
    strike_ = false;
  }
  
  // Do not use an array here to allow these to be kept in arbitrary registers.
  uint32_t phase_increment[6];

  // indication we should use built in chords
  if (noteCount == 0) {
    noteCount = 4;
    uint16_t chord_integral = parameter_[1] >> 11;
    uint16_t chord_fractional = parameter_[1] << 5;
    if (chord_fractional < 30720) {
      chord_fractional = 0;
    } else if (chord_fractional >= 34816) {
      chord_fractional = 65535;
    } else {
      chord_fractional = (chord_fractional - 30720) * 16;
    }

    phase_increment[0] = phase_increment_;
    for (size_t i = 0; i < 3; ++i) {
      uint16_t detune_1 = chords[chord_integral][i];
      uint16_t detune_2 = chords[chord_integral + 1][i];
      uint16_t detune = detune_1 + ((detune_2 - detune_1) * chord_fractional >> 16);
      phase_increment[i+1] = ComputePhaseIncrement(pitch_ + detune);
    }
  } else {
    if (quantizer.enabled_) {
      uint16_t index = quantizer.index;
      fm = pitch_ - quantizer.codebook_[quantizer.index];

      phase_increment[0] = phase_increment_;
      for (size_t i = 1; i < noteCount; i++) {
        index = (index + noteOffset[i-1]) % 128;
        phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(quantizer.codebook_[index] + fm);
      }
    } else {
      phase_increment[0] = phase_increment_;
      for (size_t i = 1; i < noteCount; i++) {
        phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(pitch_ + (noteOffset[i-1]<<7));
      }
    }
  }

  if (shape_ == OSC_SHAPE_STACK_SAW || shape_ == OSC_SHAPE_CHORD_SAW) {
    renderChordSaw(sync, buffer, size, phase_increment, noteCount);
  } else if (shape_ == OSC_SHAPE_STACK_WAVETABLE || shape_ == OSC_SHAPE_CHORD_WAVETABLE) {
    renderChordWavetable(sync, buffer, size, phase_increment, noteCount);
  } else if (shape_ == OSC_SHAPE_STACK_TRIANGLE || shape_ == OSC_SHAPE_CHORD_TRIANGLE) {
    renderChordTriangle(sync, buffer, size, phase_increment, noteCount);
  } else if (shape_ == OSC_SHAPE_STACK_SQUARE || shape_ == OSC_SHAPE_CHORD_SQUARE) {
    renderChordSquare(sync, buffer, size, phase_increment, noteCount);
  } else {
    renderChordSine(sync, buffer, size, phase_increment, noteCount);
  }
}

void DigitalOscillator::RenderStack(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  uint8_t span = 1 + (parameter_[1] >> 11);
  uint8_t offsets[kStackSize];
  uint8_t acc = 0;
  uint8_t count = kStackSize-1;

  for (uint8_t i = 0; i < count; i++) {
    acc += span;
    offsets[i] = acc;
  }

  renderChord(sync, buffer, size, offsets, kStackSize);
}

// number of notes, followed by offsets
const uint8_t diatonic_chords[16][4] = {
  {1, 7, 0, 0}, // octave
  {2, 5, 7, 0}, // octave add6
  {1, 6, 0, 0}, // 7th
  {2, 5, 6, 0}, // 7th add6
  {2, 6, 8, 0}, // 9th
  {3, 6, 8, 10}, // 11th
  {3, 5, 7, 10}, // 11th add6
  {1, 8, 0, 0}, // add9
  {2, 6, 10, 0}, // 7th add11
  {2, 6, 12, 0}, // 7th add13
  {1, 7, 0, 0}, // oct sus4
  {1, 6, 0, 0}, // 7th sus4
  {2, 6, 8, 0}, // 9th sus4
  {3, 6, 8, 10}, // 11th sus4
  {2, 6, 8, 0}, // 9th sus2
};

void DigitalOscillator::RenderDiatonicChord(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  uint8_t extensionIndex = (parameter_[1] >> 12) & 0xf;
  // uint8_t inversion = (parameter_[1] >> 13) & 0x7;
  uint8_t offsets[6];
  uint8_t len = 0;

  if (quantizer.enabled_) {
    offsets[1] = 4;

    if (extensionIndex < 11) {
      offsets[0] = 2;
    } else if (extensionIndex < 15) {
      offsets[0] = 3;
    } else {
      offsets[0] = 1;
    }

    len = diatonic_chords[extensionIndex][0] + 3;

    for (size_t i = 0; i < len; i++) {
      offsets[i+2] = diatonic_chords[extensionIndex][i+1];
    }
  } else {
    // send len=0 as indication to use the phase offsets from the paraphonic chord array.
  }

  renderChord(sync, buffer, size, offsets, len);
}

}