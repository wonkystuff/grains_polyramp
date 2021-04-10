/* Minimal sine generator for testing AE Modular Grains. Very much based
 *  on the dr1.a wonkystuff firmware, so there may be some residual stuff
 *  hanging around from that…
 * 
 * Pitch is controlled by P1/IN 1
 * 
 * Copyright (C) 2017-2020  John A. Tuffen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Questions/queries can be directed to info@wonkystuff.net
 */

#include "calc.h"

// Base-timer is running at 16MHz
#define F_TIM (16000000L)

// Fixed value to start the ADC
// enable ADC, start conversion, prescaler = /64 gives us an ADC clock of 8MHz/64 (125kHz)
#define ADCSRAVAL ( _BV(ADEN) | _BV(ADSC) | _BV(ADPS2) | _BV(ADPS1)  | _BV(ADIE) )

// let the preprocessor calculate the various register values 'coz
// they don't change after compile time
#if ((F_TIM/(SRATE)) < 255)
#define T1_MATCH ((F_TIM/(SRATE))-1)
#define T1_PRESCALE _BV(CS00)  //prescaler clk/1 (i.e. 8MHz)
#else
#define T1_MATCH (((F_TIM/8L)/(SRATE))-1)
#define T1_PRESCALE _BV(CS01)  //prescaler clk/8 (i.e. 1MHz)
#endif


#define OSCOUTREG (OCR2A)

void setup()
{
  CLKPR = _BV(CLKPCE);
  CLKPR = 0;
  
  ///////////////////////////////////////////////
  // Set up Timer/Counter2 for PWM output
  TCCR2A = 0;                           // stop the timer
  TCCR2B = 0;                           // stop the timer
  TCNT2 = 0;                            // zero the timer
  GTCCR = _BV(PSRASY);                  // reset the prescaler
  TCCR2A = _BV(WGM20)  | _BV(WGM21) |   // fast PWM to OCRA
           _BV(COM2A1) | _BV(COM2A0);   // OCR2A set at match; cleared at start
  TCCR2B = _BV(CS20);                   // fast pwm part 2; no prescale on input clock
  //OSCOUTREG = 128;                    // start with 50% duty cycle on the PWM
  pinMode(11, OUTPUT);                  // PWM output pin (grains)

  ///////////////////////////////////////////////
  // Set up Timer/Counter0 for sample-rate ISR
  TCCR0B = 0;                 // stop the timer (no clock source)
  TCNT0 = 0;                  // zero the timer

  TCCR0A = _BV(WGM01);        // CTC Mode
  TCCR0B = T1_PRESCALE;
  OCR0A  = T1_MATCH;          // calculated match value
  TIMSK0 |= _BV(OCIE0A);

  pinMode(8, OUTPUT);         // marker
}

#define NUMVOICES 8

typedef struct {
  uint16_t phase;
  uint16_t phase_inc;
} accumulator_t;

volatile accumulator_t accum[NUMVOICES];

// There are no real time constraints here, this is an idle loop after
// all...
void loop()
{
  uint8_t offs = analogRead(0) >> 6;        // P3 - detune
   
  accum[0].phase_inc = pgm_read_word(&octaveLookup[analogRead(2)]);   // CV Input 1 (P1)
  accum[1].phase_inc = accum[0].phase_inc/2;
  accum[2].phase_inc = accum[0].phase_inc+offs;
  accum[3].phase_inc = accum[0].phase_inc-offs;

  // harmonic oscillators use 3 bits: 0-7
  // CV Input 2 (P2)
  accum[4].phase_inc = accum[0].phase_inc * ((analogRead(1) >> 7)+1) + offs;
  accum[5].phase_inc = accum[4].phase_inc/2;

  // CV Input 3
  accum[6].phase_inc = accum[0].phase_inc * ((analogRead(3) >> 7)+1) - offs;
  accum[7].phase_inc = accum[6].phase_inc/2;
}

// deal with oscillator
ISR(TIMER0_COMPA_vect)
{
  uint16_t oldPhase = accum[0].phase;
  uint8_t outVal = 0;

  int i;
  for(i=0; i< NUMVOICES;i++)
  {
    outVal += accum[i].phase >> 11;
    accum[i].phase += accum[i].phase_inc;   // move the oscillator's accumulator on
  }

  OSCOUTREG = outVal;

  // If the new value of phase is smaller than it
  // was, then we have completed a ramp-cycle, so
  // change the state of output 2 - this gives us
  // a square wave an octave down from output 1.
  if (accum[0].phase < oldPhase)
  {
    PORTB ^= 0x01;  // Sub oscillator square on the D output
  }
}
