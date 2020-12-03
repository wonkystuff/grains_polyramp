/* dr1.a - firmware for a minimal drone-synth (or VCDO) for the WonkyStuff
 * 'core1' board.
 *
 * Copyright (C) 2017-2018  John A. Tuffen
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
#define NUM_ADCS (4)
#define RESET_ACTIVE (1)  // we're programming via Arduino, so the reset better be active!

// Base-timer is running at 16MHz
#define F_TIM (16000000L)

// Fixed value to start the ADC
// enable ADC, start conversion, prescaler = /64 gives us an ADC clock of 8MHz/64 (125kHz)
#define ADCSRAVAL ( _BV(ADEN) | _BV(ADSC) | _BV(ADPS2) | _BV(ADPS1)  | _BV(ADIE) )

// Remember(!) the input clock is 64MHz, therefore all rates
// are relative to that.
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

uint16_t          phase;      // The accumulated phase (distance through the wavetable)
uint16_t          pi;         // wavetable current phase increment (how much phase will increase per sample)

void setup()
{
//  PLLCSR |= _BV(PLLE);                // Enable 64 MHz PLL
//  delayMicroseconds(100);             // Stabilize
//  while (!(PLLCSR & _BV(PLOCK)));     // Wait for it...
//  PLLCSR |= _BV(PCKE);                // Timer1 source = PLL
//
  CLKPR = _BV(CLKPCE);
  CLKPR = 0;
  
  ///////////////////////////////////////////////
  // Set up Timer/Counter1 for 250kHz PWM output
  TCCR2A = 0;                  // stop the timer
  TCCR2B = 0;                  // stop the timer
  TCNT2 = 0;                   // zero the timer
  GTCCR = _BV(PSRASY);         // reset the prescaler
  TCCR2A = _BV(WGM20)  | _BV(WGM21) |  // fast PWM to OCRA
           _BV(COM2A1) | _BV(COM2A0);  // OCR2A set at match; cleared at start
  TCCR2B = _BV(CS20);                  // fast pwm part 2; no prescale on input clock
  //OSCOUTREG = 128;                     // start with 50% duty cycle on the PWM
  pinMode(11, OUTPUT);                 // PWM output pin (grains)

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

// There are no real time constraints here, this is an idle loop after
// all...
void loop()
{
    pi = pgm_read_word(&octaveLookup[analogRead(2)]);
}

// deal with oscillator
ISR(TIMER0_COMPA_vect)
{
  PORTB ^= 0x01;
  // increment the phase counter
  phase += pi;

  // By shifting the 16 bit number by 6, we are left with a number
  // in the range 0-1023 (0-0x3ff)
  uint16_t p = (phase) >> FRACBITS;

  // look up the output-value based on the current phase counter (truncated)

  // to save wavetable space, we play the wavetable forward (first half),
  // then backwards (and inverted)
  uint16_t ix = p < WTSIZE ? p : ((2*WTSIZE-1) - p);

  uint8_t s1 = pgm_read_byte(&sine[ix]);
  uint8_t s2 = pgm_read_byte(&sine[ix]);
  uint8_t s = s1 + s2;

  // invert the wave for the second half
  OSCOUTREG = p < WTSIZE ? -s : s;
}
