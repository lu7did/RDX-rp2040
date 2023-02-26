#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "decode_ft8.h"

#define CAPTURE_CHANNEL 0
#define DC_BIAS 2048
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 * Sampling parameters
 * ADC Clock=48 MHz
 * The sample acquisition time is 2 uSecs thus the maximum divisor is 96
 * therefore the maximum sampling rate is 500000 per second or 500 KSamples/sec
 * The value of CLOCK_DIV will define the actual sample rate in this program
 * a value of 0 equals using 96 as the divider because the restriction will come 
 * from the ADC conversion rate
 *    
 * 0     = 500,000 Hz
 * 960   = 50,000 Hz
 * 9600  = 5,000 Hz
 * 8000  = 6,000 Hz (sampling rate used to process FT8)
 * 
  */
#define ADC_CLOCK 48000000     //Clock rate of the independent ADC clock
#define CLOCK_DIV 8000         //Selected to sample at 6000 samples/sec (largest bandwidth admisible would be 3000 Hz)
#define FSAMP (ADC_CLOCK / CLOCK_DIV)
//#define NSAMP 960

#ifndef MULTICORE
extern int16_t fresh_signal[block_size];
extern int16_t old_signal[block_size];
#endif //not MULTICORE

#ifdef MULTICORE
extern sigBin fresh_signal;
extern sigBin old_signal;
#endif //MULTICORE

extern uint dma_chan;
extern dma_channel_config cfg;

void setup_adc();
void collect_adc();
void set_rx_freq(uint32_t);
void set_sideband(); //90 for usb, 270 for lsb
