//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=**=*=*
// Common configuration resources and definitions
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=**=*=*
#include <Arduino.h>
#include "RDX-rp2040.h"
#include "crc.h"
#include "constants.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * gen_ft8.cpp
 * Generation and encoding of ft8 messages
 * Code excerpts from
 * originally from ft8_lib by Karlis Goba (YL3JG)
 * excerpts taken from pi_ft8_xcvr by Godwin Duan (AA1GD) 2021
 * excerpts taken from Orange_Thunder by Pedro Colla (LU7DZ) 2018
 *
 * Adaptation to ADX-rp2040 project by Pedro Colla (LU7DZ) 2022
 * 
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include "gen_ft8.h"
#include "pack.h"
#include "encode.h"
#include "constants.h"

//recreated 9/20/21

//created 9/21/21
//CQ calls will always be manually generated

void manual_gen_message(char message[], message_info Station, UserSendSelection stype, char *myCall, char *myGrid){
/*
       _INFOLIST("%s call(%s) grid(%s) snr(%s) cq(%s) grid(%s) snr(%s) rsnr(%s) RRR(%s) RR73(%s) 73(%s)\n",__func__, \
                  Station.station_callsign, \
                  Station.grid_square, \
                  Station.snr_report, \
                  BOOL2CHAR(stype.call_cq), \ 
                  BOOL2CHAR(stype.send_grid), \ 
                  BOOL2CHAR(stype.send_snr), \
                  BOOL2CHAR(stype.send_Rsnr), \
                  BOOL2CHAR(stype.send_RRR), \
                  BOOL2CHAR(stype.send_RR73), \
                  BOOL2CHAR(stype.send_73)); 
*/
  
    char snr_in_string[4];
    sprintf(snr_in_string, "%d", Station.self_rx_snr);

    if (stype.call_cq){
        strcat(message, "CQ ");
        strcat(message, myCall);
        strcat(message, " ");
        strcat(message, myGrid);
    } else if(stype.send_grid){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message, " ");
        strcat(message, myGrid);
    } else if(stype.send_snr){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message, " ");
        strcat(message, snr_in_string);
    } else if(stype.send_Rsnr){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," R");
        strcat(message, snr_in_string);
    } else if(stype.send_RRR){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," RRR");
    } else if(stype.send_RR73){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," RR73");
    } else if(stype.send_73){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," 73");
    }
}

void auto_gen_message(char message[], message_info Station, char *myCall, char *myGrid){

    //should make it so if it's not addressed to you, won't respond
    char snr_in_string[14];
    //itoa(Station.self_rx_snr, snr_in_string, 10);
    //this sprintf is making strings from numbers, but isn't making the right strings...
    sprintf(snr_in_string, "%d", Station.self_rx_snr);

    if (Station.type_cq){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message, " ");
        strcat(message, myGrid);
    }

    else if (Station.type_grid){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," ");
        strcat(message, snr_in_string);

    }

    //need to distinguish between Rsnr (recieved) and snr
    //if Rsnr is recieved send out an RRR or RR73
    else if (Station.type_snr){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," R");
        strcat(message, snr_in_string);
    }

    else if (Station.type_RRR){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," 73");
        //set global variable send to false. after this, we're done sending
    }

    else if (Station.type_73){
        strcat(message, Station.station_callsign);
        strcat(message, " ");
        strcat(message, myCall);
        strcat(message," 73");
        //set global variable send to false. after this, we're done sending

    }
}

void generate_ft8(char message[], uint8_t tone_sequence[])
{
    // First, pack the text data into binary message

    uint8_t packed[FT8_LDPC_K_BYTES];
    int rc = pack77(message, packed);

    if (rc < 0)
    {
        _INFOLIST("%s Cannot parse message! RC = %d\n", __func__,rc);
    }

    //_INFOLIST("%s Packed data: ",__func__);
    /*
    for (int j = 0; j < 10; ++j)
    {
      
        sprintf(hi,"%02x ", packed[j]);
        _SERIAL.print(hi);
    }
    _SERIAL.print("\n");
    */
    
    int num_tones = FT8_NN;
    // Second, encode the binary message as a sequence of FSK tones
    genft8(packed, tone_sequence);
    //_INFOLIST("%s FSK tones: ",__func__);
    /*
    for (int j = 0; j < num_tones; ++j)
    {
        sprintf(hi,"%d", tone_sequence[j]);
        _SERIAL.print(hi);
    }
    _SERIAL.print("\n");
    */
    return;
}
