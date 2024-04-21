#include <Arduino.h>
#include <PPMReader.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <QuickPID.h>

//! /////////////////////////////////////// !//
//!         KEEP STUFF MODULAR!             !//
//! there's gonna be an RPi stuck between   !//
//!     inputs and outputs eventually!      !//
//! /////////////////////////////////////// !//

// Initialize a PPMReader on digital pin 3 with 8 expected channels.
byte interruptPin = 7; // pin7
const byte channelAmount = 8;
PPMReader ppm(interruptPin, channelAmount);

const int MPU = 0x68;

const int LEFT_TRACK_FORWARDS_PIN = 6;
const int LEFT_TRACK_BACKWARDS_PIN = 8;
const int RIGHT_TRACK_FORWARDS_PIN = 4;
const int RIGHT_TRACK_BACKWARDS_PIN = 5;
const int LEFT_TRACK_ENABLE_PIN = 9;
const int RIGHT_TRACK_ENABLE_PIN = 10;

int rx[channelAmount] = {0, 0, 0, 0, 0, 0, 0, 0};
// 0-3 are the sticks
// 4 is the button
// 5-7 are knobs

const int speedDeadzone = 25; // 0-255 //10 is the noisefloor
const int steerDeadzone = 25; // 0-255
const int trackDeadzone = 1;
int trackPreload = 0;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ; // acceleromiter/gyro stuff

float setpoint, input, output; // PID stuff
float Kp = 2, Ki = 5, Kd = 1;
QuickPID myPID(&input, &output, &setpoint);

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

void drive(int, int);
void readMPU(bool printout = 0);

void setup()
{
    Serial.begin(115200);
    pinMode(LEFT_TRACK_FORWARDS_PIN, OUTPUT);
    pinMode(LEFT_TRACK_BACKWARDS_PIN, OUTPUT);
    pinMode(RIGHT_TRACK_FORWARDS_PIN, OUTPUT);
    pinMode(RIGHT_TRACK_BACKWARDS_PIN, OUTPUT);
    pinMode(LEFT_TRACK_ENABLE_PIN, OUTPUT);
    pinMode(RIGHT_TRACK_ENABLE_PIN, OUTPUT);
    mpu.begin();
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    readMPU();

    setpoint = 0;
    myPID.SetTunings(Kp, Ki, Kd);
    myPID.SetMode(myPID.Control::automatic);
    delay(100);
}

void loop()
{
    // Print latest valid values from all channels
    for (byte channel = 0; channel <= (channelAmount - 1); ++channel)
    {
        unsigned value = ppm.latestValidChannelValue(channel + 1, 0);
        if (channel == 5)
            rx[channel] = map(value, 1876, 1060, -255, 255); // 5 needs to be inverted
        else
            rx[channel] = map(value, 1876, 1060, 255, -255);
        if (rx[channel] > 255)
            rx[channel] = 255;
        else if (rx[channel] < -255)
            rx[channel] = -255;
        Serial.print(">rx");
        Serial.print(channel);
        Serial.print(" : ");
        Serial.print(rx[channel]);
        // Serial.print("§deg");
        Serial.print("\n");
    }

    drive(rx[1], rx[0]);

    readMPU(1);

    delay(20);
}

void readMPU(bool printout)
{

    mpu.getEvent(&a, &g, &temp);

    if (printout)
        Serial.print(">AcX:");
    Serial.println(a.acceleration.x);
    Serial.print(">AcY:");
    Serial.println(a.acceleration.y);
    Serial.print(">AcZ:");
    Serial.println(a.acceleration.z);

    Serial.print(">GyX:");
    Serial.println(g.gyro.x);
    Serial.print(">GyY:");
    Serial.println(g.gyro.y);
    Serial.print(">GyZ:");
    Serial.println(g.gyro.z);

    Serial.print(">temp:");
    Serial.println(temp.temperature);
}

void drive(int speed, int steer)
{
    // expects -255 to +255 inputs. forward +, right +
    int leftTrack = 0;
    int rightTrack = 0;
    trackPreload = (rx[5] + 257) / 4;

    if ((speed < speedDeadzone) && (speed > -speedDeadzone))
        speed = 0;
    else if (speed < speedDeadzone)
        speed = speed + speedDeadzone;
    else
        speed = speed - speedDeadzone;
    if ((steer < steerDeadzone) && (steer > -steerDeadzone))
        steer = 0;
    else if (steer < speedDeadzone)
        steer = steer + speedDeadzone;
    else
        steer = steer - speedDeadzone;

    Serial.print(">speed:");
    Serial.println(speed);

    Serial.print(">steer:");
    Serial.println(steer);

    // steering mixer
    leftTrack = speed + steer;
    rightTrack = speed - steer;

    if (speed > 0 && leftTrack > speed)
        leftTrack = speed;
    if (speed > 0 && rightTrack > speed)
        rightTrack = speed;
    if (speed < 0 && leftTrack < speed)
        leftTrack = speed;
    if (speed < 0 && rightTrack < speed)
        rightTrack = speed;

    // trackspeed response curve
    if (leftTrack > 0)
        leftTrack = 2 + (2 * (pow(2, (float(leftTrack) / 32.0)))); // change linear stick movement to curve response
    else
        leftTrack = 2 - (2 * (pow(2, (float(abs(leftTrack) / 32.0))))); // because the mafs don't work for negative values

    if (rightTrack > 0)
        rightTrack = 2 + (2 * (pow(2, (float(rightTrack) / 32.0)))); // change linear stick movement to curve response
    else
        rightTrack = 2 - (2 * (pow(2, (float(abs(rightTrack) / 32.0))))); // because the mafs don't work for negative values

    // cap max track speed
    leftTrack = (leftTrack < 255) ? leftTrack : 255;   // cap max values
    leftTrack = (leftTrack > -255) ? leftTrack : -255; // cap max values
    // leftTrack = (leftTrack > 240) ? 255: leftTrack;  //top deadzone

    rightTrack = (rightTrack < 255) ? rightTrack : 255;
    rightTrack = (rightTrack > -255) ? rightTrack : -255;
    // rightTrack = (rightTrack > 240) ? 255: rightTrack;

    Serial.print(">trackDeadZone:");
    Serial.println(trackDeadzone);
    Serial.print(">negtrackDeadZone:");
    Serial.println(-trackDeadzone);

    if ((leftTrack < trackDeadzone) && (leftTrack > -trackDeadzone))
    {
        leftTrack = 0;
        analogWrite(LEFT_TRACK_ENABLE_PIN, 0);
        digitalWrite(LEFT_TRACK_FORWARDS_PIN, 0);
        digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 0);
    }
    else
    {
        if (leftTrack > -trackDeadzone)
        {
            digitalWrite(LEFT_TRACK_FORWARDS_PIN, 1);
            digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 0);
        }
        else if (leftTrack < trackDeadzone)
        {
            digitalWrite(LEFT_TRACK_FORWARDS_PIN, 0);
            digitalWrite(LEFT_TRACK_BACKWARDS_PIN, 1);
        }
        leftTrack = map(abs(leftTrack), 0, 255, trackPreload, 255);
        analogWrite(LEFT_TRACK_ENABLE_PIN, abs(leftTrack));
    }

    if (rightTrack < trackDeadzone && rightTrack > -trackDeadzone)
    {
        rightTrack = 0;
        analogWrite(RIGHT_TRACK_ENABLE_PIN, 0);
        digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
        digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
    }
    else
    {
        if (rightTrack > -trackDeadzone)
        {
            digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 1);
            digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
        }
        else if (rightTrack < trackDeadzone)
        {
            digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
            digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 1);
        }
        rightTrack = map(abs(rightTrack), 0, 255, trackPreload, 255);
        analogWrite(RIGHT_TRACK_ENABLE_PIN, abs(rightTrack));
    }

    Serial.print(">leftTrack:");
    Serial.println(leftTrack);
    Serial.print(">rightTrack:");
    Serial.println(rightTrack);
    /*
        if(rightTrack == 0) {
            analogWrite(RIGHT_TRACK_ENABLE_PIN, 0);
            digitalWrite(RIGHT_TRACK_FORWARDS_PIN, 0);
            digitalWrite(RIGHT_TRACK_BACKWARDS_PIN, 0);
        } else {

        }
        */
}
