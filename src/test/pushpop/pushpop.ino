//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       External libraries used                                               *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
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

/*-------------- REQUIRED -----------------*/
#include <iostream>
#include <queue>
using namespace std;
#include "pico/sem.h"

#include <WiFi.h>
#include <Time.h>
#include <stdbool.h>

bool waitSem=true;


/*--------------------------------------------------------
 * Structures needed to manage a IPC safe queue
 */
static struct semaphore ipc;
queue<int> q;
/*--------------------------------------------------------*/

char hi[1024];
int qcnt=0;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


void setup() {
  Serial.begin(115200);
  while (!Serial);


  /*-------------
   Înit the semaphore, allow one first entry
   */
  sem_init(&ipc, 1, 1);
  Serial.println("Semaphore initialized");
  
  /*---------------
   * This is to avoid the race of the core1
   */
  waitSem=false;

}
void setup1() {
  /*---------------
   * wait till initialization is performed
   */
  while (waitSem);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2120);
  Serial.println("Waiting ");
  Serial.print(" ");
  int i=0;
  /*----------------------------
   * Acquire semaphore
   */
  while(!sem_try_acquire(&ipc)) {
     Serial.print(".");
     i++;
     if (i>10) {
        Serial.println(" ");
        i=0;
     }   
  }
  Serial.println(" ");
  Serial.println("Acquired<1>");

  /*------------------------------
   * Once iin safe condition 
   * empty the queue
   */
  while (!q.empty()) {

  /*-----------------------------
   * First extract
   */
     int data=q.front();
     
     Serial.print(data);
     Serial.print(" ");
  /*-----------------------------   
   * then clear the front
   */
     q.pop();
  }
  Serial.println(" ");
  Serial.flush();

  /*-----------------------------
   * Release the resource
   */
  sem_release(&ipc);
  
}
void loop1() {

  // put your main code here, to run repeatedly:
  
  delay(815);

  /*-----------------------------------
   * Acquire the semaphore
   */
  while(!sem_try_acquire(&ipc));
  Serial.println("Acquired<2>");

  /*----------------------------------
   * Ônce secured place an element at 
   * bottom
   */
  q.push(qcnt);
  
  Serial.print("Wrote ");
  Serial.println(qcnt);
  qcnt++;
  Serial.flush();

  /*--------------------------------
   * Release the semaphore
   */
  sem_release(&ipc);
  
}
