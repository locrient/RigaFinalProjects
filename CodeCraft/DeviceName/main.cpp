/* Copyright (c) 2022 Perlatecnica APS ETS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
/****************************************************
*            RAPID PROTOTYPING WITH NUCLEO          *
* Example Code 13: Ultrasound HCSR04                *
* Author: Mauro D'Angelo                            *
* Organization: Perlatecnica APS ETS                *  
*****************************************************/

#include "mbed.h"
#include "hcsr04.h"
#include "DS1302.h"

//Motor output
DigitalOut MotorPlus(A4, 0);
DigitalOut MotorMinus(A5, 0);

//RTC setup
#define INITIAL_RUN     //Comment this line if the DS1302 is already running

DS1302 clk(D6, D3, D8);

//Rain detector
//AnalogIn  rainAO(A2);
DigitalIn rainDO(D2, PullUp);

// Photoresistor connected to analog input pin A0
AnalogIn lightSensor(A0);

// It creates an instance of HCSR04, and assigns trigger and echo pins
HCSR04 sensor(PB_8, PB_9);

// It tooggles timestamp from 21PM to 10AM. DEMO purpose
InterruptIn cal_button(PC_13) ;

// YL-69 AO wired to A1 (PA_1)
AnalogIn soil(A1);
// fill these after calibration
uint16_t SOIL_DRY_RAW = 4000;  // reading in air (dry)
uint16_t SOIL_WET_RAW = 1200;  // reading in water (wet)
// Map 12-bit raw to 0–100% (0=dry, 100=wet)
static uint8_t map_to_percent(uint16_t raw) {
    if (SOIL_DRY_RAW < SOIL_WET_RAW) { uint16_t t=SOIL_DRY_RAW; SOIL_DRY_RAW=SOIL_WET_RAW; SOIL_WET_RAW=t; }
    if (raw > SOIL_DRY_RAW) raw = SOIL_DRY_RAW;
    if (raw < SOIL_WET_RAW) raw = SOIL_WET_RAW;
    uint32_t num = (uint32_t)(SOIL_DRY_RAW - raw) * 100u;
    uint32_t den = (uint32_t)(SOIL_DRY_RAW - SOIL_WET_RAW);
    return den ? (uint8_t)(num / den) : 0;
}
//float ema = 0.0f;               // simple smoother
//const float alpha = 0.2f;

// Serial connection to PC
Serial pc(USBTX, USBRX);

// Ticker for periodic interrupts
Ticker five_second_ticker;

// Global variables for measurement
volatile bool measurement_requested = false;
volatile bool toggle_requested = false;
float HCS04_referenceValue = 0;  // Calibrated reference value
bool HCS04_isCalibrated = false;
int HCS04_measurement_count = 0;

bool bulb_notification_sent = false;    // Track if notification was sent
bool last_bulb_state = false;           // Track previous bulb state

bool bin_notification_sent = false;     // Track if bin notification was sent
bool last_bin_state = false;            // Track previous bin state

// Function prototypes
void timer_isr();
void button_isr();
void HCS04_calibration();
float HCS04_measurement();
bool IsBinFull();   //return true if ultrasonic sensor says so
bool IsLightOFF();  //return true if LDR sensor says so
bool IsSoilDry();   //return true if soil sensor says so
bool IsRaining();   //return true if rain sensor says so
bool IsNightTime(); //return true if RTC says so
void RTC_Init();
void print_current_time();



int main() {
    pc.baud(115200);
    pc.printf("CodeCraft system started\r\n");
    pc.printf("Measurements every 5 seconds\r\n");

    // Configure button with pull-up and attach interrupt
    cal_button.mode(PullUp);
    cal_button.fall(&button_isr);

    // Attach the measurement timer interrupt
    five_second_ticker.attach(&timer_isr, 5.0f);

    RTC_Init();

    HCS04_calibration();

    // Main loop
    while(1) {    
        // Check for calibration request
        if (toggle_requested) {
            toggle_requested = false;
            if(!IsNightTime()) {
                clk.set_time(1757451600);  //09.09.2025 21:00:00
                pc.printf("Time changed ");
                print_current_time();
            }
            else {
                clk.set_time(1760091115);  //10.09.2025 10:00:00
                pc.printf("Time changed ");
                print_current_time();
            }
        }
        
        // Check for measurement request
        if (measurement_requested) {
            measurement_requested = false;
            

            // Bin monitoring
            bool current_bin_state = IsBinFull();
            
            if (current_bin_state && !last_bin_state) {
                // Bin just became full - send notification
                pc.printf("EMPTY BIN - Bin is full and needs emptying ");
                print_current_time();
                bin_notification_sent = true;
            } 
            else if (!current_bin_state && last_bin_state) {
                // Bin was emptied - reset notification
                pc.printf("Bin has been emptied ");
                print_current_time();
                bin_notification_sent = false;
            }
            last_bin_state = current_bin_state;
            
            // Bulb monitoring
            bool current_bulb_state = IsLightOFF() & IsNightTime();
            
            if (current_bulb_state && !last_bulb_state) {
                // Bulb just went out - send notification
                pc.printf("Change bulb - light is off during nighttime ");
                print_current_time();
                bulb_notification_sent = true;
            } 
            else if (!current_bulb_state && last_bulb_state) {
                // Bulb was fixed or it's no longer nighttime - reset notification
                pc.printf("Bulb is working or daytime started ");
                print_current_time();
                bulb_notification_sent = false;
            }
            
            last_bulb_state = current_bulb_state;
            

            // Soil monitoring and auto pump if neccessary
            if(IsSoilDry()&!IsNightTime()&!IsRaining()) {
                MotorPlus = 1;
            }
            else {
                MotorPlus = 0;
            }


        }
    }
}

void timer_isr() {
    // ISR should be short - just set a flag
    measurement_requested = true;
}

void button_isr() {
    // Button press ISR - simple debouncing
    static uint32_t last_press = 0;
    uint32_t now = us_ticker_read() / 1000; // Convert to ms
    
    if ((now - last_press) > 500) { // 500ms debounce
        toggle_requested = true;
        last_press = now;
    }
}

void HCS04_calibration() {
    pc.printf("\r\n=== Starting Calibration ===\r\n");
    
    float sum = 0;
    int valid_measurements = 0;
    
    for (int i = 0; i < 3; i++) {
        float value = HCS04_measurement();
        if (value >= 0) {
            sum += value;
            valid_measurements++;
        } else {
            pc.printf("Calibration %d failed\r\n", i+1);
        }
        ThisThread::sleep_for(1000); // Wait between measurements
    }
    
    if (valid_measurements > 0) {
        HCS04_referenceValue = sum / valid_measurements;
        HCS04_isCalibrated = true;
        pc.printf("=== Calibration Complete ===\r\n");
        pc.printf("HCS-04 reference value: %.0f cm\r\n", HCS04_referenceValue);
        pc.printf("============================\r\n\r\n");
    } else {
        pc.printf("=== Calibration Failed ===\r\n");
        pc.printf("==========================\r\n\r\n");
    }
}
float HCS04_measurement() {
    int measure[5] = {0};
    float averageValue = 0;
    
    // Take 5 measurements
    for (int i = 0; i < 5; i++) {
        sensor.start();
        ThisThread::sleep_for(100); // Wait for measurement
        measure[i] = sensor.get_dist_cm();
    }
    
    // Calculate average
    averageValue = sensor.filter(measure, 5);
    
    return averageValue;
}
bool IsBinFull() {
    float current_value = HCS04_measurement();
            
        if (HCS04_isCalibrated) {
            float difference = current_value - HCS04_referenceValue;
            //pc.printf("Current: %.0f cm, Ref: %.0f cm, Diff: %.0f cm", 
            //                current_value, HCS04_referenceValue, difference);
            
            if (difference < -15.0) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
}
bool IsLightOFF() {
    // Read analog value (0.0 to 1.0)
        float analog_value = lightSensor.read();
        
        if (analog_value > 0.7f) {
            return true;
        } else {
            return false;
        }
        
        ThisThread::sleep_for(500);
}
bool IsSoilDry() {
    // read_u16 is 0..65535; F401 ADC is 12-bit -> shift to 0..4095
    uint16_t raw12 = soil.read_u16() >> 4;

    uint8_t pct = map_to_percent(raw12);

    ThisThread::sleep_for(500);

    if (pct<30) {
        return true;
    }
    else {
        return false;
    }
}
bool IsRaining() {
    //float raw = rainAO.read();    // 0.0 .. 1.0 (proportional to 3.3V)
    int rain = rainDO.read();    // 0 = rain detected (threshold)
    thread_sleep_for(200);

    if (rain == 0) {
        return true;
    }
    else {
        return false;
    }
}
void RTC_Init() {
    #ifdef INITIAL_RUN
    clk.set_time(1757451600);  //09.09.2025 21:00:00
    #endif
    
    char storedByte = clk.recallByte(0);
    clk.storeByte(0, storedByte + 1);

    time_t current_time = clk.time(NULL);
    struct tm *timeinfo = localtime(&current_time);
    pc.printf("Current Time is ");
    pc.printf("[%02d:%02d:%02d] \n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
void print_current_time() {
    time_t current_time = clk.time(NULL);
    struct tm *timeinfo = localtime(&current_time);
    pc.printf("[%02d:%02d:%02d] \n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
bool IsNightTime() {
    // Get full timestamp and extract hours
    time_t current_time = clk.time(NULL);
    struct tm *timeinfo = localtime(&current_time);
    int hours = timeinfo->tm_hour;  // 0-23 format

    if(hours<8 | hours>19) {
        return true;
    }
    else {
        return false;
    }
}
