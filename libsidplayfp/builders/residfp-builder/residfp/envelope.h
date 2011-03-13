//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef VICE__ENVELOPE_H__
#define VICE__ENVELOPE_H__

#include "residfp-config.h"

extern float env_dac[256];

// ----------------------------------------------------------------------------
// A 15 bit counter is used to implement the envelope rates, in effect
// dividing the clock to the envelope counter by the currently selected rate
// period.
// In addition, another counter is used to implement the exponential envelope
// decay, in effect further dividing the clock to the envelope counter.
// The period of this counter is set to 1, 2, 4, 8, 16, 30 at the envelope
// counter values 255, 93, 54, 26, 14, 6, respectively.
// ----------------------------------------------------------------------------
class EnvelopeGeneratorFP
{
public:
  EnvelopeGeneratorFP();

  enum State { ATTACK, DECAY_SUSTAIN, RELEASE };

  RESID_INLINE void clock();
  void reset();
  void mute(bool enable);

  void writeCONTROL_REG(reg8);
  void writeATTACK_DECAY(reg8);
  void writeSUSTAIN_RELEASE(reg8);
  reg8 readENV();

  RESID_INLINE float output();

protected:
  void set_nonlinearity(const chip_model model, const float nl);
  void update_rate_period(reg16 period);

  int rate_counter;
  int rate_period;
  reg8 exponential_counter;
  reg8 exponential_counter_period;
  reg8 envelope_counter;
  float envelope_counter_dac;
  bool hold_zero;
  bool muted;

  reg4 attack;
  reg4 decay;
  reg4 sustain;
  reg4 release;

  reg8 gate;

  State state;

  // Lookup table to convert from attack, decay, or release value to rate
  // counter period.
  static const reg16 rate_counter_period[];

  // The 16 selectable sustain levels.
  static const reg8 sustain_level[];

friend class SIDFP;
};

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGeneratorFP::clock()
{
  if (++ rate_counter != rate_period)
    return;

  rate_counter = 0;

  // The first envelope step in the attack state also resets the exponential
  // counter. This has been verified by sampling ENV3.
  //
  if (state == ATTACK || ++exponential_counter == exponential_counter_period)
  {
    exponential_counter = 0;

    // Check whether the envelope counter is frozen at zero.
    if (hold_zero) {
      return;
    }

    switch (state) {
    case ATTACK:
      // The envelope counter can flip from 0xff to 0x00 by changing state to
      // release, then to attack. The envelope counter is then frozen at
      // zero; to unlock this situation the state must be changed to release,
      // then to attack. This has been verified by sampling ENV3.
      //
      ++envelope_counter &= 0xff;
      if (envelope_counter == 0xff) {
        state = DECAY_SUSTAIN;
        update_rate_period(rate_counter_period[decay]);
      }
      break;
    case DECAY_SUSTAIN:
      if (envelope_counter != sustain_level[sustain]) {
        --envelope_counter;
      }
      break;
    case RELEASE:
      // The envelope counter can flip from 0x00 to 0xff by changing state to
      // attack, then to release. The envelope counter will then continue
      // counting down in the release state.
      // This has been verified by sampling ENV3.
      // NB! The operation below requires two's complement integer.
      //
      --envelope_counter &= 0xff;
      break;
    }
    
    // Check for change of exponential counter period.
    switch (envelope_counter) {
    case 0xff:
      exponential_counter_period = 1;
      break;
    case 0x5d:
      exponential_counter_period = 2;
      break;
    case 0x36:
      exponential_counter_period = 4;
      break;
    case 0x1a:
      exponential_counter_period = 8;
      break;
    case 0x0e:
      exponential_counter_period = 16;
      break;
    case 0x06:
      exponential_counter_period = 30;
      break;
    case 0x00:
      exponential_counter_period = 1;

      // When the envelope counter is changed to zero, it is frozen at zero.
      // This has been verified by sampling ENV3.
      hold_zero = true;
      break;
    }

    envelope_counter_dac = muted ? 0.f : env_dac[envelope_counter];
  }
}

// ----------------------------------------------------------------------------
// Read the envelope generator output.
// ----------------------------------------------------------------------------
RESID_INLINE
float EnvelopeGeneratorFP::output()
{
  return envelope_counter_dac;
}

#endif // not VICE__ENVELOPE_H__
