#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "hardware/watchdog.h"
#include "Wire.h"
#include <EEPROM.h>
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include <WiFi.h>
#include <Time.h>
#include <stdbool.h>
#include <WiFiUdp.h>
#include <si5351.h>
#include "SPI.h"
#include <FS.h>
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>




char _str[128];
int16_t fresh_signal[1024];
uint dma_chan;
dma_channel_config cfg;
WiFiUDP udp;                           //UDP server
int cal_factor=100000;
Si5351 si5351;
TFT_eSPI tft = TFT_eSPI();


//*---------------
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}
//*---------------
int setup_wifi() {
  
  uint32_t t=time_us_32();
/*            VVVVVVVVVVVVVVVVVVVVVVV   VVVVVVVVVVV   <----- Replace by your WiFi AP credentials */
  WiFi.begin("Fibertel WiFi996 2.4GHz","00413322447");

  while (WiFi.status() != WL_CONNECTED) {
    if (time_us_32() - t > 10000000) {
      Serial.println("Failed to connect to WiFi");
      return WiFi.status();
    }
  }
  sprintf(_str,"Connected to WiFi IP(%s)\n",IpAddress2String(WiFi.localIP()).c_str());
  Serial.print(_str);
  delay(500);
  NTP.begin("time.nist.gov", "pool.ntp.org");
  bool ntp_rc = NTP.waitSet(10000);

  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  sprintf(_str,"NTP time is %s\n", asctime(&timeinfo));
  Serial.print(_str);

  return (int) WL_CONNECTED;

}

//*-------------------
void setup_adc() {

  /*------------------------------------
     setup the ADC port
     ADC0=GPIO26, ADC_CHANNEL=0, Hi-Z, No pulls, disable the pin as digital input
  */
  adc_gpio_init(26 + 0);
  adc_init();
  adc_select_input( 0 );
  adc_fifo_setup(
    true,    // Write each completed conversion to the sample FIFO
    true,    // Enable DMA data request (DREQ)
    1,       // DREQ (and IRQ) asserted when at least 1 sample present
    false,   // We won't see the ERR bit because of 8 bit reads; disable.
    false     // Shift each sample to 8 bits when pushing to FIFO
  );
  adc_set_clkdiv( 8000 );
  sleep_ms(200);

  dma_chan = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dma_chan);
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);

  Serial.println("setup_adc() completed");
}
//*----------------
void collect_adc() {

  adc_fifo_drain();
  adc_irq_set_enabled(false);
  adc_run(false);
      
  dma_channel_configure(dma_chan, &cfg,
      fresh_signal,    // dst
      &adc_hw->fifo,  // src
      960,          // transfer count
      true            // start immediately
      );
  adc_irq_set_enabled(true);
  adc_run(true);
}
//*----------
void inc_collect_power() {

  for (uint idx_block = 0; idx_block < 80; idx_block++) {
    collect_adc();
    dma_channel_wait_for_finish_blocking(dma_chan);
  }
}
void initSi5351() {

  //------------------------------- SET SI5351 VFO -----------------------------------
  // The crystal load value needs to match in order to have an accurate calibration
  //----------------------------------------------------------------------------------
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);// SET For Max Power
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA); // Set for reduced power for RX
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA); // Set for reduced power for RX

  si5351.set_clock_pwr(SI5351_CLK2, 0); // Turn on calibration Clock
  si5351.set_clock_pwr(SI5351_CLK0, 0); // Turn off transmitter clock

  si5351.output_enable(SI5351_CLK0, 0);   //RX off
  si5351.output_enable(SI5351_CLK2, 0);   //RX off

  unsigned long freq = 7074000UL;
  si5351.set_freq(freq * 100ULL, SI5351_CLK1);

  si5351.set_clock_pwr(SI5351_CLK1, 1); // Turn on receiver clock
  si5351.output_enable(SI5351_CLK1, 1);   // RX on

}

//*--------------
void setup() {

  Serial.begin(115200);
  while (!Serial);
  delay(200);
  Serial.flush();
  Serial.println("Serial port initialized");

  Wire.setSDA(16);
  Wire.setSCL(17);
  Wire.begin();
  
  EEPROM.begin(512);
  int temp;
  EEPROM.get(10, temp);
  EEPROM.put(10, temp);
  EEPROM.commit();

  initSi5351();
  
  int rc=setup_wifi();
  sprintf(_str,"Wifi setup completed rc(%d)\n",rc);
  Serial.print(_str);
  
  setup_adc();
  Serial.println("adc setup completed");


  tft.init();
  tft.setRotation( 1 );   //ROTATION_SETUP
  tft.fillScreen(TFT_BLACK);     //TFT_CYAN
  tft.fillScreen(TFT_BLACK);

  // Swap the colour byte order when rendering
  tft.setSwapBytes(true);

  
}
//*---------------
void loop() {

  uint32_t tstart=time_us_32();
  inc_collect_power();
  sprintf(_str,"loop() inc_collect_time(%ul)\n",time_us_32()-tstart);
  Serial.print(_str);
  tft.fillRect(0 , 0, 480, 32, TFT_YELLOW);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(_str, 10,  20, 1); // Print the label

}
