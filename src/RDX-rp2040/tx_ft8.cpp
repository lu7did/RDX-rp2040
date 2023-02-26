#include <Arduino.h>
#include "RDX-rp2040.h"
#include "constants.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * tx_ft8.cpp
 * Handling of the actual transmission of ft8 tones already defined
 * Code excerpts from
 * originally from ft8_lib by Karlis Goba (YL3JG)
 * excerpts taken from pi_ft8_xcvr by Godwin Duan (AA1GD) 2021
 * excerpts taken from Orange_Thunder by Pedro Colla (LU7DZ) 2018
 *
 * Adaptation to ADX-rp2040 project by Pedro Colla (LU7DZ) 2022
 * 
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include "tx_ft8.h"
#include "pico/stdlib.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 * receives the tones[] stream to be sent, the base RF_Freq and the offset AF frequency to use
 * transmit the 79 tones over a cycle of 160 mSecs each
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
CALLBACK  txIdle=NULL;
void send_ft8(uint8_t tones[], uint32_t RF_freq, uint16_t AF_freq){

    startTX();
    tft_set(BUTTON_TX,1);
    uint32_t af=((uint32_t)AF_freq)*100UL;
    uint32_t rf=((uint32_t)RF_freq)*100UL;

    si5351.output_enable(SI5351_CLK1, 0);   //RX off
    si5351.set_clock_pwr(SI5351_CLK1, 0); // Turn on receiver clock

    si5351.set_freq(rf+af, SI5351_CLK0);
    si5351.set_clock_pwr(SI5351_CLK0, 1); // Turn on receiver clock
    si5351.output_enable(SI5351_CLK0, 1);   //TX on  
    digitalWrite(TX, 1);

    /*
    digitalWrite(RX, LOW);
    si5351.output_enable(SI5351_CLK1, 0);   //RX off
    si5351.set_clock_pwr(SI5351_CLK1, 0); // Turn on receiver clock

    si5351.set_clock_pwr(SI5351_CLK0, 1); // Turn on receiver clock
    
    digitalWrite(TX, 1);
    si5351.output_enable(SI5351_CLK0, 1);   //TX on  
    
    //_INFOLIST("%s TX+ RF=%lu AF=%d rf=%lu af=%lu\n",__func__,RF_freq,AF_freq,rf,af);
    */
    uint32_t tbar=time_us_32();
    for (uint8_t i = 0; i < 79; i++){
           
        uint32_t tstop = time_us_32() + 159000;
        uint32_t ftones=(uint32_t)tones[i]*625;
        //_INFOLIST("%s RF=%lu AF=%lu ftones=%lu f=%lu\n",__func__,rf,af,ftones,rf+af+ftones);
        si5351.set_freq(rf+af+ftones, SI5351_CLK0);
        while (time_us_32() < tstop) {
          
            int TXSW_State = digitalRead(TXSW);
            if (TXSW_State == LOW) {
                ManualTX();
                return;
            }
            /*  TX doesn't work correctly with this callback.
            if (txIdle != NULL) txIdle();
            */
            
            if (time_us_32()-tbar > 1000000) {
               tbar=time_us_32();
               tft_setBarTx();
            }
            
            
        }
    }

    digitalWrite(TX, 0);
    si5351.output_enable(SI5351_CLK0, 0);   //RX off
    si5351.set_clock_pwr(SI5351_CLK0, 0); // Turn on receiver clock
    digitalWrite(RX, HIGH);
    
    si5351.set_freq(rf, SI5351_CLK1);
    si5351.set_clock_pwr(SI5351_CLK1, 1); // Turn on receiver clock
    si5351.output_enable(SI5351_CLK1, 1);   //RX on

    stopTX();
    tft_set(BUTTON_TX,0);
    /*
    digitalWrite(TX, 0);
    si5351.output_enable(SI5351_CLK0, 0);   //RX off
    si5351.set_clock_pwr(SI5351_CLK0, 0); // Turn on receiver clock

    digitalWrite(RX, HIGH);
    
    si5351.set_freq(rf, SI5351_CLK1);
    si5351.set_clock_pwr(SI5351_CLK1, 1); // Turn on receiver clock
    si5351.output_enable(SI5351_CLK1, 1);   //RX on
    //_INFOLIST("%s TX-\n",__func__);
    */
}
