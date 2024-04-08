#include <Arduino.h>
#include <PPMReader.h>

// Initialize a PPMReader on digital pin 3 with 6 expected channels.
byte interruptPin = 3;
byte channelAmount = 8;
PPMReader ppm(interruptPin, channelAmount);

void setup() {
    Serial.begin(115200);
}


void loop() {
    // Print latest valid values from all channels
    for (byte channel = 1; channel <= channelAmount; ++channel) {
        unsigned value = ppm.latestValidChannelValue(channel, 0);
        Serial.print(">rx"); 
        Serial.print(channel); 
        Serial.print(" : "); 
        Serial.print(value); 
        //Serial.print("§deg");
        Serial.print("\n");
        //if(channel < channelAmount) Serial.print('\t');
    }
    //Serial.println();
    delay(20);
}



