#include <Arduino.h>
#include <PPMReader.h>

//! /////////////////////////////////////// !//
//!         KEEP STUFF MODULAR!             !//
//! there's gonna be an RPi stuck between   !//
//!     inputs and outputs eventually!      !//
//! /////////////////////////////////////// !//

// Initialize a PPMReader on digital pin 3 with 8 expected channels.
byte interruptPin = 7; //pin7
const byte channelAmount = 8;
PPMReader ppm(interruptPin, channelAmount);

const int LEFT_TRACK_FORWARDS_PIN = 6;
const int LEFT_TRACK_BACKWARDS_PIN = 8;
const int RIGHT_TRACK_FORWARDS_PIN = 4;
const int RIGHT_TRACK_BACKWARDS_PIN = 5;
const int LEFT_TRACK_ENABLE_PIN = 9;
const int RIGHT_TRACK_ENABLE_PIN = 10;

int rx[channelAmount]={0,0,0,0,-255,0,0,0};
//0-3 are the sticks
//4 is the button
//5-7 are knobs

int speedDeadzone = 10; //0-255
int steerDeadzone = 10; //0-255



void drive(int, int);

void setup() {
    Serial.begin(115200);
}


void loop() {
    // Print latest valid values from all channels
    for (byte channel = 0; channel <= (channelAmount-1); ++channel) {
        unsigned value = ppm.latestValidChannelValue(channel+1, 0);
        rx[channel]=map(value,1876,1060,255,-255);
        Serial.print(">rx"); 
        Serial.print(channel); 
        Serial.print(" : "); 
        Serial.print(rx[channel]); 
        //Serial.print("§deg");
        Serial.print("\n");
        //if(channel < channelAmount) Serial.print('\t');
    }
    //Serial.println();


    drive(rx[1], rx[0]);
    Serial.print(">drive1:");
    Serial.println(rx[1]);
    
    Serial.print(">drive0:");
    Serial.println(rx[0]);
    Serial.println(">button:pressed|t");


    delay(20);
    

}


void drive(int speed, int steer)
{
    //expects -255 to +255 inputs. forward +, right +
    int leftTrack = 0;
    int rightTrack = 0;
    if((speed<speedDeadzone)&&(speed>-speedDeadzone)) speed = 0;
    if((steer<steerDeadzone)&&(steer>-steerDeadzone)) steer = 0;
    
    //steering mixer
    leftTrack = (-speed - steer)/2;
    leftTrack = (leftTrack < 255) ? leftTrack : 255; //cap max values
    leftTrack = (leftTrack > 240) ? 255: leftTrack;  //top deadzone
    
    rightTrack = (-speed + steer)/2;
    rightTrack = (rightTrack < 255) ? rightTrack : 255;
    rightTrack = (rightTrack > 240) ? 255: rightTrack;

    if(leftTrack == 0) {
        analogWrite(LEFT_TRACK_ENABLE_PIN, 0);
        digitalWrite(LEFT_TRACK_FORWARDS_PIN, 0);
        digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 0);
    } else {
        if(leftTrack > 0) {
            digitalWrite(LEFT_TRACK_FORWARDS_PIN, 1);
            digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 0);
        } else if(leftTrack < 0) {
            digitalWrite(LEFT_TRACK_FORWARDS_PIN, 0);
            digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 1);
        }
        //analogWrite(LEFT_TRACK_ENABLE_PIN, abs(leftTrack));
        analogWrite(LEFT_TRACK_ENABLE_PIN, map(abs(leftTrack),0,255,rx[4],255));
    }

    if(rightTrack == 0) {
        analogWrite(RIGHT_TRACK_ENABLE_PIN, 0);
        digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
        digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
    } else {
        if(rightTrack > 0) {
            digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 1);
            digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
        } else if(rightTrack < 0) {
            digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
            digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 1);
        }
        analogWrite(RIGHT_TRACK_ENABLE_PIN, abs(rightTrack));
    }
/*
    if(rightTrack == 0) {
        analogWrite(RIGHT_TRACK_ENABLE_PIN, 0);
        digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
        digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
    } else {

    }
    */
}
