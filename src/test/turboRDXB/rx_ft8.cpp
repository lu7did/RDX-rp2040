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

/*------------------------[FIX]------------------------------*/
//queue<void> queued_signal;
//queue<sigBin> queued_signal;


uint16_t queueR=0;
uint16_t queueW=0;

sigBin queued_signal[QMAX];
sigBin fresh_signal = {0};
sigBin old_signal = {0};

struct semaphore ipc;
struct semaphore spc;
struct semaphore apc;
bool start_adc=false;
queue_t qdata;
queue_t sdata;

bool led2=LOW;
/*-----------------------------------------------------------*/
uint dma_chan;
dma_channel_config cfg;

void pushSignal() {
   for (int i=0;i<960;i++) {
       queued_signal[queueW][i]=fresh_signal[i];
   }
   queueW++;    
   if (queueW>(QMAX-1)) queueW=0;
}
void popSignal() {
  for (int i=0;i<960;i++) {
     old_signal[i]=queued_signal[queueR][i];
  }   
  queueR++;
  if (queueR>(QMAX-1)) queueR=0;
}
bool availSignal() {
  if (queueR!=queueW) {
      return true;
  }
  return false;
}
int sizeSignal() {
  if (queueW>=queueR) {
     return queueW-queueR;
  }
  return QMAX-queueR+queueW;
}
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

void process_adc() {

  /*----------------------------------------------
   * First, sync with the FT8 protocol time 
   * windows. Collection must start at secs 0,15,30 and
   * 45
   */

   //uint32_t adcOk=multicore_fifo_pop_blocking();
   uint32_t q=0;
   queue_remove_blocking(&qdata,&q);

   for (uint idx_block = 0; idx_block < num_adc_blocks; idx_block++) {
       collect_adc();
       dma_channel_wait_for_finish_blocking(dma_chan);
       while(!sem_try_acquire(&ipc));
       pushSignal();
       sem_release(&ipc);
       queue_add_blocking(&sdata,(uint32_t)0);
       
       //multicore_fifo_push_blocking((uint32_t)0);
   }
}
