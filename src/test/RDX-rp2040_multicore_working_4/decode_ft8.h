#pragma once
#include "decode.h"
#include "kiss_fftr.h"
/*---------------------------------------------
   FT8 protocol QSO data structure
*/
struct m {
  int8_t   self_rx_snr; // this is a map between -120dB to 0dB as 0 to 240 in 0.5 dB increments
  uint16_t af_frequency;
  uint16_t qsowindow;
  float    time_offset;
  char     full_text[20];
  bool     addressed_to_me;
  bool     type_cq;
  bool     type_grid;
  bool     type_snr;
  bool     type_Rsnr;
  bool     type_RRR;
  bool     type_73;
  bool     type_RR73;
  char     station_callsign[8];
  char     grid_square[5];
  char     snr_report[5];
};
typedef m message_info;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                                      Decoding parameters                                                              *
/*                              Tune for performance and capacity considerations                                         *
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#define kFreq_osr              1 //both default 2
#define kTime_osr              1
#define kFSK_dev            6.25 // tone deviation in Hz and symbol rate
#define sample_rate         6000
#define decoding_time       12.8
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

enum {num_bins = (int)(sample_rate / (2 * kFSK_dev))}; // number bins of FSK tone width that the spectrum can be divided into
enum {block_size = (int)(sample_rate / kFSK_dev)};     // samples corresponding to one FSK symbol
enum {subblock_size = block_size / kTime_osr};
enum {nfft = block_size * kFreq_osr};
enum {num_blocks = (int) (decoding_time * kFSK_dev)};
enum {num_samples_processed = nfft * (1 + kTime_osr) / 2};

#ifdef MULTICORE
extern int num_adc_blocks;
#endif //MULTICORE

//extern uint8_t mag_power[num_blocks * kFreq_osr * kTime_osr * num_bins];
extern waterfall_t power;


float hann_i(int i, int N);
void make_window(void);
static float max2(float a, float b);

// Compute FFT magnitudes (log power) for each timeslot in the signal
uint8_t inc_extract_power(uint dmachan, int16_t signal[],bool lastFrame);
void inc_collect_power();
int decode_ft8(message_info message_list[]);
void identify_message_types(message_info message_list[], char *my_callsign);
