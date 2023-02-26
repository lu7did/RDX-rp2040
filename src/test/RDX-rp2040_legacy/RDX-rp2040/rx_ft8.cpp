#include <Arduino.h>
#include "RDX-rp2040.h"
#include "constants.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
   rx_ft8
   Setup the ADC and start conversions
   Code excerpts from
   originally from ft8_lib by Karlis Goba (YL3JG)
   excerpts taken from pi_ft8_xcvr by Godwin Duan (AA1GD) 2021
   excerpts taken from Orange_Thunder by Pedro Colla (LU7DZ) 2018

   Adaptation to ADX-rp2040 project by Pedro Colla (LU7DZ) 2022

  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include "rx_ft8.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "decode_ft8.h"
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*  Variables                                                                               *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

const int CAPTURE_DEPTH = block_size;
int16_t fresh_signal[block_size] = {0};
int16_t old_signal[block_size] = {0};
uint dma_chan;
dma_channel_config cfg;

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*  Setup and operate ADC                                                                   *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
/*---------------------------
   setup_adc()
   initial setup of the ADC conversion and DMA transfer
   This has been modeled as part of the quicksilver algorithm
*/
void setup_adc() {

  /*------------------------------------
     setup the ADC port
     ADC0=GPIO26, ADC_CHANNEL=0, Hi-Z, No pulls, disable the pin as digital input
  */
  /*---------------
     initializes the ADC ports
  */
  adc_gpio_init(ADC0 + ADC_CHANNEL);
  adc_init();
  adc_select_input(ADC_CHANNEL);
  adc_fifo_setup(
    true,    // Write each completed conversion to the sample FIFO
    true,    // Enable DMA data request (DREQ)
    1,       // DREQ (and IRQ) asserted when at least 1 sample present
    false,   // We won't see the ERR bit because of 8 bit reads; disable.
    false     // Shift each sample to 8 bits when pushing to FIFO
  );

  /*------------------
     Set the divider which defines the number of samples
  */

  adc_set_clkdiv(CLOCK_DIV);

  /*------------------
     Let things settle
  */
  sleep_ms(200);

  /*------------------
     Initialize now the DMA channel
     Start by selecting an unused DMA channel and get the default configuration for it.
     Then establish that samples will be 8 bytes and placed incrementally in the write buffer
     Finally, sync the rate of transfer to the availability of ADC samples
  */

  dma_chan = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dma_chan);
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);

}

/*-----------------
   collect_adc()
   triggered to start an ADC meassurement
   As part of the quicksilver algorithm the processing isn't longer
   held here until the ADC sample ends, it rather continues and the
   remaining time is used by the caller 
*/
void collect_adc() {

  adc_fifo_drain();
  adc_irq_set_enabled(false);
  adc_run(false);
      
  dma_channel_configure(dma_chan, &cfg,
      fresh_signal,    // dst
      &adc_hw->fifo,  // src
      CAPTURE_DEPTH,          // transfer count
      true            // start immediately
      );
  adc_irq_set_enabled(true);
  adc_run(true);
}
