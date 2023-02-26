//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=**=*=*
// Common configuration resources and definitions
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=**=*=*
#include <Arduino.h>
#include "RDX-rp2040.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
   decode_ft8.cpp
   ft8 decoding functions
   Code excerpts from
   originally from ft8_lib by Karlis Goba (YL3JG)
   excerpts taken from pi_ft8_xcvr by Godwin Duan (AA1GD) 2021
   excerpts taken from Orange_Thunder by Pedro Colla (LU7DZ) 2018

   Adaptation to ADX-rp2040 project by Pedro Colla (LU7DZ) 2022

  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "unpack.h"
#include "ldpc.h"
#include "decode.h"
#include "constants.h"
#include "crc.h"

#include "kiss_fftr.h"

#include "decode_ft8.h"
#include "rx_ft8.h"
#include "pico/multicore.h"

//#include "hardware/timer.h" //won't be needed once delay is replaced with interrupt

const float fft_norm = 2.0f / nfft;
CALLBACK fftReady=NULL;
CALLBACK fftEnd=NULL;
CALLQSO  qsoReady=NULL;
int num_adc_blocks=(int) (decoding_time * kFSK_dev);


//AA1GD-added array of structures to store info in decoded messages 8/22/2021
//LU7DZ-do not understant why, but until I do I left there         11/25/2022

//setup fft freq output power, which will be accessible by both cores
//maybe should be put in decode_ft8.c?

uint8_t mag_power[num_blocks * kFreq_osr * kTime_osr * num_bins] = {0};
waterfall_t power = {
  .num_blocks = num_blocks,
  .num_bins = num_bins,
  .time_osr = kTime_osr,
  .freq_osr = kFreq_osr,
  .mag = mag_power
};

volatile int offset = 0;
bool flag_first = true;
uint8_t adc_error = 0x00;
uint32_t tstart;
int magint[960]={0};
bool led1=HIGH;


kiss_fftr_cfg fft_cfg;

float window[nfft]; //I wonder if static will work here

float hann_i(int i, int N)
{
  float x = sinf((float)3.1416 * i / N); //replaced M_PI with a float
  return x * x;
}

void make_window(void) {
  const int len_window = (int) (1.8f * block_size); // hand-picked and optimized
  for (int i = 0; i < nfft; ++i)
  {
    window[i] = (i < len_window) ? hann_i(i, len_window) : 0;
  }

}

static float max2(float a, float b)
{
  return (a >= b) ? a : b;
}

// Compute FFT magnitudes (log power) for each timeslot in the signal
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
   this procedure is where the energy is extracted
   it has been refactored completely to implement the quicksilver algorithm where the processsing
   of the fft and other errands for each sample batch (block_size samples) is performed while the
   next ADC sample is taken.
   The FFT magnitudes are computed (log power) for each processing tick in the signal (960 samples)
  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
uint8_t inc_extract_power(uint dmachan, int16_t signal[], bool lastFrame)
{
  /*------------------------------------*
     loop over two possible time offsets
     [0,block_size/2]
     keep watching the ADC sampling and
     interrupt if the time was exceeded
    ------------------------------------*/
  for (int time_sub = 0; time_sub < power.time_osr; ++time_sub)
  {
    kiss_fft_scalar timedata[nfft];
    kiss_fft_cpx freqdata[nfft / 2 + 1];
    float mag_db[nfft / 2 + 1];
    /*------------------------------------*
       extract windowed signal block
      ------------------------------------*/
    for (int pos = 0; pos < nfft; ++pos)
    {
      timedata[pos] = signal[(time_sub * subblock_size) + pos] / 2048.0f;
    }
    kiss_fftr(fft_cfg, timedata, freqdata);
    /*------------------------------------*
       Compute log magnitude (dB)
      ------------------------------------*/
    for (int idx_bin = 0; idx_bin < nfft / 2 + 1; ++idx_bin)
    {
      float mag2 = (freqdata[idx_bin].i * freqdata[idx_bin].i) + (freqdata[idx_bin].r * freqdata[idx_bin].r);
      mag_db[idx_bin] = 10.0f * log10f(1E-12f + mag2 * fft_norm * fft_norm);
      int sig_db=(int)(2 * mag_db[idx_bin] + 240);
      magint[idx_bin]=magint[idx_bin]+(sig_db < 0) ? 0 : ((sig_db > 255) ? 255 : sig_db);
    }
/*-------------- CALLBACK   ---------------*/    
    if (fftReady != NULL) fftReady();
    
    /*--------------------------------------*
       Loop over two possible frequency bins
       offset (for averaging)
      --------------------------------------*/

    // Loop over two possible frequency bin offsets (for averaging)
    for (int freq_sub = 0; freq_sub < power.freq_osr; ++freq_sub)
    {
      for (int pos = 0; pos < power.num_bins; ++pos)
      {
        float db = mag_db[pos * power.freq_osr + freq_sub];
        /*------------------------------------*
           scale dB to uint8_t range and clamp
           the value (range 0-240) -120..0 dB
           in 0.5 dB steps
          ------------------------------------*/
        int scaled = (int)(2 * db + 240);
       
        power.mag[offset] = (scaled < 0) ? 0 : ((scaled > 255) ? 255 : scaled);
        power.mag[offset] = scaled;
        offset++;
      }
    }
  }
  return 0x00;
}
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
   this procedure is where the energy is collected
   it has been refactored completely to implement a dual core processing where the ADC are sampled
   in core1 whilst the signal processing is made in core 0.
  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
void inc_collect_power() {

  process_adc();

 /*-----------------------[CALLBACK]-------------------------*/
  if (fftEnd!=NULL) fftEnd();

}

void get_collect_power() {

  size_t fft_work_size;
  kiss_fftr_alloc(nfft, 0, 0, &fft_work_size);
  void *fft_work = malloc(fft_work_size);
  fft_cfg = kiss_fftr_alloc(nfft, 0, fft_work, &fft_work_size);
  memset(magint, 0, sizeof(magint));
  
  for (uint idx_block = 0; idx_block < num_blocks; idx_block++) {
    uint32_t fifoADC = 0;
    while(!multicore_fifo_pop_timeout_us((uint64_t)1000,&fifoADC)) {
    }   
    
    if (sizeSignal()>1) {
       _INFO("Warning signal queue size(%d)\n",sizeSignal());
    }    
    while(!sem_try_acquire(&ipc)) {
    }
    popSignal();
    sem_release(&ipc);
    
    for (int i = 0; i < nfft; i++) {
        old_signal[i] -= DC_BIAS;
    }
    adc_error = inc_extract_power(dma_chan, old_signal,false);
    if (adc_error != 0x00) {
       _INFO("Warning signal processing error(%x)\n",adc_error);
    }
  }
  free(fft_work);
  offset = 0;
  return;
}
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
   Process candidates by Costas sync score and localize them in time and frequency
 *                                                                                                *
  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=**/
int decode_ft8(message_info message_list[])
{
  int qsowindow=getQSOwindow();
  candidate_t candidate_list[kMax_candidates];
  int num_candidates = find_sync(&power, kMax_candidates, candidate_list, kMin_score);

  /*------------------------------------*
     Check for duplicates using a hash
     table for decoded messages
    ------------------------------------*/
  int num_decoded = 0;
  message_t decoded[kMax_decoded_messages];
  message_t *decoded_hashtable[kMax_decoded_messages];
  /*------------------------------------*
     Compute the SNR
    ------------------------------------*/

  //if using calc_noise type 1 or 2 use the below two funcions. type 3 goes right before calc_snr
  int noise_avg = calc_noise(&power, NULL);
  //_INFOLIST("%s Noise average: %d \n",__func__,noise_avg);

  // Initialize hash table pointers
  for (int i = 0; i < kMax_decoded_messages; ++i)
  {
    decoded_hashtable[i] = NULL;
  }

  // Go over candidates and attempt to decode messages
  for (int idx = 0; idx < num_candidates; ++idx)
  {
    //AA1GD added to try correctly stop program when decoded>kMax_decoded_messages
    if (num_decoded >= kMax_decoded_messages) {
      //_INFOLIST("%s decoded more than kMax_decoded_messages, Decoded %d messages and force ended\n", __func__, num_decoded);
      return (num_decoded);
    }

    const candidate_t *cand = &candidate_list[idx];
    if (cand->score < kMin_score)
      continue;

    float freq_hz = (cand->freq_offset + (float)cand->freq_sub / kFreq_osr) * kFSK_dev;
    float time_sec = (cand->time_offset + (float)cand->time_sub / kTime_osr) / kFSK_dev;

    message_t message;
    decode_status_t status;

    if (!decode(&power, cand, &message, kLDPC_iterations, &status))
    {
      if (status.ldpc_errors > 0)
      {
        //_INFOLIST("%s LDPC decode: %d errors\n", __func__,status.ldpc_errors);
      }
      else if (status.crc_calculated != status.crc_extracted)
      {
        //_INFOLIST("%s  CRC mismatch!\n",__func__);
      }
      else if (status.unpack_status != 0)
      {
        //_INFOLIST("%s Error while unpacking!\n",__func__);
      }
      continue;
    }

    int idx_hash = message.hash % kMax_decoded_messages;
    bool found_empty_slot = false;
    bool found_duplicate = false;
    do
    {
      if (decoded_hashtable[idx_hash] == NULL)
      {
        found_empty_slot = true;
      }
      else if ((decoded_hashtable[idx_hash]->hash == message.hash) && (0 == strcmp(decoded_hashtable[idx_hash]->text, message.text)))
      {
        found_duplicate = true;
      }
      else
      {
        // Move on to check the next entry in hash table
        idx_hash = (idx_hash + 1) % kMax_decoded_messages;
      }
    } while (!found_empty_slot && !found_duplicate);

    if (found_empty_slot)
    {
      // Fill the empty hashtable slot
      memcpy(&decoded[idx_hash], &message, sizeof(message));
      decoded_hashtable[idx_hash] = &decoded[idx_hash];

      int snr = calc_snr(&power, cand, noise_avg);

      //_INFOLIST("%s %x   %3d  %+3.1f %4d ~  %s\n", __func__, num_decoded, snr, time_sec, (int) freq_hz, message.text);
      //_INFOLIST("%s estimated snr: %d\n", __func__, snr);
      
      message_list[num_decoded].self_rx_snr = snr;
      message_list[num_decoded].af_frequency = (uint16_t) freq_hz;
      message_list[num_decoded].time_offset = time_sec;
      strcpy(message_list[num_decoded].full_text, message.text);
      message_list[num_decoded].qsowindow=qsowindow;

      /*--------------------------[qsoReady]------------------------------*/
      if (qsoReady != NULL) qsoReady(num_decoded);

      ++num_decoded;
    }
  }
  //_INFOLIST("%s Decoded %d messages\n", __func__, num_decoded);
  return num_decoded;
}

//need to finish this. Sept. 19 2021 also need to fix snr linear regression
//should input global struct message_info current_station to compare current station
void identify_message_types(message_info message_list[], char *my_callsign) {

  
  for (int i = 0; i < kMax_decoded_messages; i++) {

    if (!(message_list[i].af_frequency)) { //checks if its empty
      return;
    }


    if (strstr(message_list[i].full_text, my_callsign)) {
      message_list[i].addressed_to_me = true;
    }

    if (strstr(message_list[i].full_text, "CQ")) {
      message_list[i].type_cq = true;
    }

    if (strstr(message_list[i].full_text, "RRR")) {
      message_list[i].type_RRR = true;
    }

    if (strstr(message_list[i].full_text, "73")) {
      message_list[i].type_73 = true;
    }

    if (strstr(message_list[i].full_text, "RR73")) {
      message_list[i].type_RR73 = true;
    }

    char message_buffer[25];
    strcpy(message_buffer, message_list[i].full_text);
    const char delim[2] = " ";

    char *first_word;
    first_word = strtok(message_buffer, delim);

    char *second_word;
    second_word = strtok(NULL, delim);

    char *third_word;
    third_word = strtok(NULL, delim);

    //fourth word supports CQ extra tags like DX, POTA, QRP
    char *fourth_word;
    fourth_word = strtok(NULL, delim);

    if (strlen(second_word) > 3 && !fourth_word) {
       strcpy(message_list[i].station_callsign, second_word);
    } else {
      if (strlen(third_word) > 3 && fourth_word)  {
         strcpy(message_list[i].station_callsign, third_word);
      }
    }

    if (!third_word) {
      strcpy(third_word, "N/A");
    }

    if (message_list[i].type_cq) {
      if (!fourth_word) {
        strcpy(message_list[i].grid_square, third_word);
      } else {
        strcpy(message_list[i].grid_square, fourth_word);
      }
    } else { 
      if (strlen(third_word) == 4 && strchr(third_word, '-')==NULL && strchr(third_word, '+')==NULL && strcmp(third_word, "RR73")!=0 && strcmp(third_word,"73")!=0) {
         message_list[i].type_grid = true;
         strcpy(message_list[i].grid_square, third_word);
      } else {
         if (strchr(third_word, '-') || strchr(third_word, '+')) {
            if (strchr(third_word, 'R')) {
               message_list[i].type_Rsnr = true;
            } else {
               message_list[i].type_snr = true;
            }
            strcpy(message_list[i].snr_report, third_word);
         }
      }
    }  
  }
  return;
}
