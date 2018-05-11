#include "ble_config.h"

/*
 * Simple Bluetooth Demo
 * This code demonstrates how the RedBear Duo board communicates with 
 * the paired Android app. The user can use the Android app to 
 * do digital read & write, analog read & write, and servo control.
 * Created by Liang He, April 27th, 2018
 * 
 * The Library is created based on Bjorn's code for RedBear BLE communication: 
 * https://github.com/bjo3rn/idd-examples/tree/master/redbearduo/examples/ble_led
 * 
 * Our code is created based on the provided example code (Simple Controls) by the RedBear Team:
 * https://github.com/RedBearLab/Android
 */

#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define RECEIVE_MAX_LEN    3
#define SEND_MAX_LEN    4
#define BLE_SHORT_NAME_LEN 0x08 // must be in the range of [0x01, 0x09]
#define BLE_SHORT_NAME 'S','A','M','D','e','m','o'  // define each char but the number of char should be BLE_SHORT_NAME_LEN-1


/* Define the pins on the Duo board
 */
#define BRIGHTNESS_OUT_PIN         D0
#define R_OUT_PIN                  D1
#define G_OUT_PIN                  D2
#define B_OUT_PIN                  D3 
#define COLOR_CTRL_IN_PIN          A6
#define BRIGHTNESS_CTRL_IN_PIN     A7

// Servo declaration
Servo myservo;

// UUID is used to find the device by other BLE-abled devices
static uint8_t service1_uuid[16]    = { 0x71,0x3d,0x00,0x00,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
static uint8_t service1_tx_uuid[16] = { 0x71,0x3d,0x00,0x03,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
static uint8_t service1_rx_uuid[16] = { 0x71,0x3d,0x00,0x02,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };

// Define the receive and send handlers
static uint16_t receive_handle = 0x0000; // recieve
static uint16_t send_handle = 0x0000; // send

static uint8_t receive_data[RECEIVE_MAX_LEN] = { 0x01 };
static uint8_t send_data[SEND_MAX_LEN] = { 0x00 };

// Define the configuration data
static uint8_t adv_data[] = {
  0x02,
  BLE_GAP_AD_TYPE_FLAGS,
  BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE, 
  
  BLE_SHORT_NAME_LEN,
  BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME,
  BLE_SHORT_NAME, 
  
  0x11,
  BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
  0x1e,0x94,0x8d,0xf1,0x48,0x31,0x94,0xba,0x75,0x4c,0x3e,0x50,0x00,0x00,0x3d,0x71 
};

static btstack_timer_source_t send_characteristic;

// Index to the color table that displayed by RGB LED
// 0 -- Red; 1 -- Green; 2 -- Blue; 3 -- Orange; 
static int current_color_idx = 0;
static byte color_table[][3] = {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {0, 255, 255} };
static int color_table_len = sizeof(color_table) / sizeof(color_table[0]);

// Input pin state.
static byte brightness = 0;

unsigned long previous_time = 0;

/**
 * @brief Callback for writing event.
 *
 * @param[in]  value_handle  
 * @param[in]  *buffer       The buffer pointer of writting data.
 * @param[in]  size          The length of writting data.   
 *
 * @retval 
 */
int bleWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size) {
  Serial.print("Write value handler: ");
  Serial.println(value_handle, HEX);

  if (receive_handle == value_handle) {
    memcpy(receive_data, buffer, RECEIVE_MAX_LEN);
    Serial.print("Write value: ");
    for (uint8_t index = 0; index < RECEIVE_MAX_LEN; index++) {
      Serial.print(receive_data[index], HEX);
      Serial.print(" ");
    }
    Serial.println(" ");
    
    /* Process the data
     */
    if (receive_data[0] == 0x01) { // Command is to control digital out pin
      current_color_idx++;
      if (current_color_idx >= color_table_len) {
        current_color_idx = 0;
      }
      previous_time = millis();
    }
    else if (receive_data[0] == 0x03) { // Command is to control Servo pin
      brightness = receive_data[1];
      previous_time = millis();
    }
  }
  return 0;
}

/**
 * @brief Timer task for sending status change to client.
 * @param[in]  *ts   
 * @retval None
 * 
 * Send color and brightness data back to 
 * the other BLE-abled devices
 */
static void  send_notify(btstack_timer_source_t *ts) {
  // report the current brightness.
  send_data[0] = (0x0B);
  send_data[1] = (brightness);
  send_data[2] = (0x00);
  send_data[3] = (0x00);
  if (ble.attServerCanSendPacket())
    ble.sendNotify(send_handle, send_data, SEND_MAX_LEN);
    
  // report the current color selection.
  // Serial.println("send_notify digital reading ");
  send_data[0] = (0x0A);
  send_data[1] = (color_table[current_color_idx][0]);
  send_data[2] = (color_table[current_color_idx][1]);
  send_data[3] = (color_table[current_color_idx][2]);

  ble.sendNotify(send_handle, send_data, SEND_MAX_LEN);
  
  // Restart timer.
  ble.setTimer(ts, 200);
  ble.addTimer(ts);
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("Simple Controls demo.");

  // Initialize ble_stack.
  ble.init();
  configureBLE(); //lots of standard initialization hidden in here - see ble_config.cpp
  // Set BLE advertising data
  ble.setAdvertisementData(sizeof(adv_data), adv_data);
  
  // Register BLE callback functions
  ble.onDataWriteCallback(bleWriteCallback);

  // Add user defined service and characteristics
  ble.addService(service1_uuid);
  receive_handle = ble.addCharacteristicDynamic(service1_tx_uuid, ATT_PROPERTY_NOTIFY|ATT_PROPERTY_WRITE|ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, receive_data, RECEIVE_MAX_LEN);
  send_handle = ble.addCharacteristicDynamic(service1_rx_uuid, ATT_PROPERTY_NOTIFY, send_data, SEND_MAX_LEN);

  // BLE peripheral starts advertising now.
  ble.startAdvertising();
  Serial.println("BLE start advertising.");

  /*
   * initialize pin modes
   */
  pinMode(BRIGHTNESS_OUT_PIN, OUTPUT);
  pinMode(R_OUT_PIN, OUTPUT);
  pinMode(G_OUT_PIN, OUTPUT);
  pinMode(B_OUT_PIN, OUTPUT);
  pinMode(COLOR_CTRL_IN_PIN, INPUT);
  pinMode(BRIGHTNESS_CTRL_IN_PIN, INPUT);

  pinMode(D7, OUTPUT);
  
  // Start a task to check status.
  send_characteristic.process = &send_notify;
  ble.setTimer(&send_characteristic, 500);//2000ms
  ble.addTimer(&send_characteristic);
}

void loop() {
  digitalWrite(D7, LOW);

  unsigned long current_time = millis(); // grab current time
  if (current_time -  previous_time > 10000) {
    // no command from remote control for 10 seconds, switch to physical control
    int color_raw = analogRead(COLOR_CTRL_IN_PIN);
    current_color_idx = map(color_raw, 0, 4096, 0, color_table_len);
    
    int brightness_raw = analogRead(BRIGHTNESS_CTRL_IN_PIN);
    if (brightness_raw < 100) {
      brightness_raw = 100;
    }
    if (brightness_raw > 4000) {
      brightness_raw = 4000;
    }
    brightness = 255 - map(brightness_raw, 100, 4000, 0, 255);    

    Serial.print("Color: ");
    Serial.print(current_color_idx);
    Serial.print(", Brightness: ");
    Serial.println(brightness);
  }

  // Output color level
  analogWrite(R_OUT_PIN, 255 - color_table[current_color_idx][0]);
  analogWrite(G_OUT_PIN, 255 - color_table[current_color_idx][1]);
  analogWrite(B_OUT_PIN, 255 - color_table[current_color_idx][2]);

  // Output brightness level
  analogWrite(BRIGHTNESS_OUT_PIN, brightness);

  delay(500);
}
