
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <si5351.h>
#include "hardware/watchdog.h"
#include "Wire.h"
#include <EEPROM.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include <WiFi.h>
#include <Time.h>
#include <stdbool.h>
uint16_t val;
uint16_t c=0;
char hi[128];
void setup() {

  Serial.begin(115200);
  while(!Serial);
  Serial.println("Partisan program started");
        /* set up IRQ handler */
  irq_set_exclusive_handler(ADC_IRQ_FIFO, irs_adc);
  irq_set_enabled(ADC_IRQ_FIFO, true);

        /* fifo */
  adc_fifo_setup(true, false, 8, true, false);
  
  /* run conversions */
  adc_irq_set_enabled(true);
  adc_run(true);
  Serial.println("Setup completed");
}
void loop() {  
    sleep_ms(1000);
    sprintf(hi,"Counter=%d\n",c);
    Serial.print(hi);
}

static void irs_adc(void) {
  
  uint16_t val;
  uint8_t level;
  c++;
  
  level = adc_fifo_get_level();
  //sprintf(hi,"adc: input: %u, empty: %d, level: %u, input: %u\n",adc_get_selected_input(),adc_fifo_is_empty(),level,adc_get_selected_input());
  //Serial.print(hi);
  while (level-- > 0) {
    val = adc_fifo_get();
    if (val & 0x1000) {
      //sprintf(hi,"\t%u: %u (error)\n", level, val);
      //Serial.print(hi);
    } else {
      //sprintf(hi,"\t%u: %u\n", level, val);
      //Serial.print(hi);
    }
  }
}
