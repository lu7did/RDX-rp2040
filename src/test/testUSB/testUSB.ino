#include <LittleFS.h>
#include "SingleFileDrive.h"

bool safe=true;
void myPlugCB(uint32_t data) {
    safe=false;
    // Tell my app not to write to flash, we're connected
}

void myUnplugCB(uint32_t data) {
    safe=true;
    // I can start writing to flash again
}

void myDeleteCB(uint32_t data) {
  
    // Maybe LittleFS.remove("myfile.txt")?  or do nothing
}

void setup() {
    Serial.begin(115200);
    while(!Serial);
    Serial.flush();
    Serial.println("USB FS alive");
    
    LittleFS.begin();
    singleFileDrive.onPlug(myPlugCB);
    singleFileDrive.onUnplug(myUnplugCB);
    singleFileDrive.onDelete(myDeleteCB);
    singleFileDrive.begin("littlefsfile.csv", "Data Recorder.csv");
    // ... rest of setup ...
}

void loop() {
    // Take some measurements, delay, etc.
    if (safe) {
        delay(1000);
        noInterrupts();
        File f = LittleFS.open("littlefsfile.csv", "a");
        int data1=1;
        int data2=2;
        int data3=3;
        f.printf("%d,%d,%d\n", data1, data2, data3);
        f.close();
        interrupts();
        Serial.println("Recording");
    } else {
       Serial.println("Not safe");
       delay(1000);
    }
}
