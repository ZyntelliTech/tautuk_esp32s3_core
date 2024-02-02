/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "MQLibrary.h"

static const char *TAG = "TauTuk";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLUE_LED_GPIO   GPIO_NUM_19
#define RED_LED_GPIO    GPIO_NUM_17
#define GREEN_LED_GPIO  GPIO_NUM_18

#define Voltage_Resolution      5000 // Calibration the resistor divider
#define RatioMQ135CleanAir      3.6

#define R1                      10
#define R2                      10

#define PACKET_SEND_FREQUENCY       120000
#define GAS_DELAY                   5000
#define GAS_DATA_POINTS             5

#define BLINK_PERIOD     1000

float CO        = 0.0;
float Alcohol   = 0.0;
float CO2       = 0.0;
float Toluen    = 0.0;
float NH4       = 0.0;
float Aceton    = 0.0;

static uint8_t s_led_state = 0;
/*********************** Function Declaration ********************/
static void blink_led(void);
static void configure_led(void);
static int read_sensor(void);


/********************** Fundion Definition **************************************/

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLUE_LED_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLUE_LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLUE_LED_GPIO, GPIO_MODE_OUTPUT);
}

static int read_sensor(void)
{
    update(); // Update data, the arduino will read the voltage from the analog pin

    setA(605.18); setB(-3.937); // Configure the equation to calculate CO concentration value
    CO = readSensor(false, 0.0, false); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    setA(77.255); setB(-3.18); //Configure the equation to calculate Alcohol concentration value
    Alcohol = readSensor(false, 0.0, false); // SSensor will read PPM concentration using the model, a and b values set previously or from the setup

    setA(110.47); setB(-2.862); // Configure the equation to calculate CO2 concentration value
    CO2 = readSensor(false, 0.0, false); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    setA(44.947); setB(-3.445); // Configure the equation to calculate Toluen concentration value
    Toluen = readSensor(false, 0.0, false); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
  
    setA(102.2 ); setB(-2.473); // Configure the equation to calculate NH4 concentration value
    NH4 = readSensor(false, 0.0, false); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

    setA(34.668); setB(-3.369); // Configure the equation to calculate Aceton concentration value
    
    Aceton = readSensor(false, 0.0, false); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
    printf("|   %f",CO); 
    printf("   |   %f",Alcohol);
    // Note: 400 Offset for CO2 source: https://github.com/miguel5612/MQSensorsLib/issues/29
/*
Motivation:
We have added 400 PPM because when the library is calibrated it assumes the current state of the
air as 0 PPM, and it is considered today that the CO2 present in the atmosphere is around 400 PPM.
https://www.lavanguardia.com/natural/20190514/462242832581/concentracion-dioxido-cabono-co2-atmosfera-bate-record-historia-humanidad.html
*/
    printf("   |   %f",(CO2 + 400)); 
    printf("   |   %f",Toluen); 
    printf("   |   %f",NH4); 
    printf("   |   %f",Aceton);
    printf("   |\n"); 
/*
Exponential regression:
GAS      | a      | b
CO       | 605.18 | -3.937  
Alcohol  | 77.255 | -3.18 
CO2      | 110.47 | -2.862
Toluen  | 44.947 | -3.445
NH4      | 102.2  | -2.473
Aceton  | 34.668 | -3.369
*/
    return 1;
}
void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();
    initADC();
    
    //Set the Power voltage of the MQ-135 sensor
    setVoltResolution(Voltage_Resolution);
    //Set math model to calculate the PPM concentration and the value of constants
    setRegressionMethod(1); //_PPM =  a*ratio^b
    
  /*
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */

/*****************************  MQ Init ********************************************/ 
//Remarks: Configure ADC module
/************************************************************************************/ 
    setR1(R1);
    setR2(R2);
    //RL should be configurated after setting R1 and R2
    setRL(10);

/*****************************  MQ CAlibration ********************************************/ 
// Explanation: 
// In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
// and on clean air (Calibration conditions), setting up R0 value.
// We recomend executing this routine only on setup in laboratory conditions.
// This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
// Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
    ESP_LOGI(TAG, "Calibrating please wait.");
    float calcR0 = 0;
    for(int i = 1; i<=10; i ++)
    {
      update(); // Update data, the arduino will read the voltage from the analog pin
      calcR0 += calibrate(RatioMQ135CleanAir);
      printf(".");
    }
    setR0(calcR0/10);
    ESP_LOGI(TAG, "calcR0 = %f,  done!.", calcR0);
  
    if(isinf(calcR0)) {
        ESP_LOGW(TAG, "Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); 
        while(1);
    }
    if(calcR0 == 0){
        ESP_LOGW(TAG, "Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); 
        while(1);
    }
/*****************************  MQ CAlibration ********************************************/ 

    ESP_LOGI(TAG, "** Values from MQ-135 ****");
    printf("|    CO   |  Alcohol |   CO2  |  Toluen  |  NH4  |  Aceton  |\n");  
    
    while (1) {
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //blink_led();
        /* Toggle the LED state */
        //s_led_state = !s_led_state;
        read_sensor();
        
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
