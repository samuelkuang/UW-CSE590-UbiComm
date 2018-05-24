#include "ble_config.h"

/*
 * Provides skeleton code to interact with the Android FaceTrackerBLE app 
 * 
 * Created by Jon Froehlich, May 7, 2018
 * 
 * Based on previous code by Liang He, Bjoern Hartmann, 
 * Chris Dziemborowicz and the RedBear Team. See: 
 * https://github.com/jonfroehlich/CSE590Sp2018/tree/master/A03-BLEAdvanced
 */

#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define RECEIVE_MAX_LEN  5 // TODO: change this based on how much data you are sending from Android 
#define SEND_MAX_LEN    3

// Must be an integer between 1 and 9 and and must also be set to len(BLE_SHORT_NAME) + 1
#define BLE_SHORT_NAME_LEN 8 

// The number of chars should be BLE_SHORT_NAME_LEN - 1. So, for example, if your BLE_SHORT_NAME was 'J', 'o', 'n'
// then BLE_SHORT_NAME_LEN should be 4. If 'M','a','k','e','L','a','b' then BLE_SHORT_NAME_LEN should be 8
// TODO: you must change this name. Otherwise, you will not be able to differentiate your RedBear Duo BLE
// device from everyone else's device in class.
#define BLE_SHORT_NAME 'S','a','m','u','L','a','b'  

/* Define the pins on the Duo board
 * TODO: change and add/subtract the pins here for your applications (as necessary)
 */
#define TRIG_PIN D0
#define ECHO_PIN D1
#define G_OUT_PIN D2
#define B_OUT_PIN D3 
#define R_OUT_PIN D8
#define SERVO_OUT_PIN D4
#define BUZZER_OUT_PIN D9

#define MAX_SERVO_ANGLE  180
#define MIN_SERVO_ANGLE  0

#define BLE_DEVICE_CONNECTED_DIGITAL_OUT_PIN D7

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;

// happiness meter (servo)
Servo _posServo;

// Device connected and disconnected callbacks
void deviceConnectedCallback(BLEStatus_t status, uint16_t handle);
void deviceDisconnectedCallback(uint16_t handle);

// UUID is used to find the device by other BLE-abled devices
static uint8_t service1_uuid[16]    = { 0x71,0x3d,0x00,0x00,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
static uint8_t service1_tx_uuid[16] = { 0x71,0x3d,0x00,0x03,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };
static uint8_t service1_rx_uuid[16] = { 0x71,0x3d,0x00,0x02,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e };

// Define the receive and send handlers
static uint16_t receive_handle = 0x0000; // recieve
static uint16_t send_handle = 0x0000; // send

static int siren_tone[][2] = {{440, 200}, {494, 500}, {523, 300}};
static int siren_tone_len = sizeof(siren_tone) / sizeof(siren_tone[0]);
int current_siren_idx = 0;
int ToneCounter = 0;

int LightCounter = 0;
// Index to the color table that displayed by RGB LED
// 0 -- Red; 1 -- Green; 2 -- Blue; 3 -- White; 
static int current_color_idx = 0;
static byte color_table[][3] = {{255, 0, 0}, {0, 0, 255}, {255, 255, 255} };
static int color_table_len = sizeof(color_table) / sizeof(color_table[0]);

static int alarm_pause_counter = 0;

static uint8_t receive_data[RECEIVE_MAX_LEN] = { 0x01 };
int bleReceiveDataCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size); // function declaration for receiving data callback
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
static void bleSendDataTimerCallback(btstack_timer_source_t *ts); // function declaration for sending data callback
int _sendDataFrequency = 200; // 200ms (how often to read the pins and transmit the data to Android)

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("Face Tracker BLE Demo.");

  // Initialize ble_stack.
  ble.init();
  
  // Register BLE callback functions
  ble.onConnectedCallback(bleConnectedCallback);
  ble.onDisconnectedCallback(bleDisconnectedCallback);

  //lots of standard initialization hidden in here - see ble_config.cpp
  configureBLE(); 
  
  // Set BLE advertising data
  ble.setAdvertisementData(sizeof(adv_data), adv_data);
  
  // Register BLE callback functions
  ble.onDataWriteCallback(bleReceiveDataCallback);

  // Add user defined service and characteristics
  ble.addService(service1_uuid);
  receive_handle = ble.addCharacteristicDynamic(service1_tx_uuid, ATT_PROPERTY_NOTIFY|ATT_PROPERTY_WRITE|ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, receive_data, RECEIVE_MAX_LEN);
  send_handle = ble.addCharacteristicDynamic(service1_rx_uuid, ATT_PROPERTY_NOTIFY, send_data, SEND_MAX_LEN);

  // BLE peripheral starts advertising now.
  ble.startAdvertising();
  Serial.println("BLE start advertising.");

  // Setup pins
  pinMode(R_OUT_PIN, OUTPUT);
  pinMode(G_OUT_PIN, OUTPUT);
  pinMode(B_OUT_PIN, OUTPUT);
  _posServo.attach(SERVO_OUT_PIN);
  _posServo.write( (int)((MAX_SERVO_ANGLE - MIN_SERVO_ANGLE) / 2.0) );

    // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(BUZZER_OUT_PIN, OUTPUT);
  
  // Start a task to check status of the pins on your RedBear Duo
  // Works by polling every X milliseconds where X is _sendDataFrequency
  send_characteristic.process = &bleSendDataTimerCallback;
  ble.setTimer(&send_characteristic, _sendDataFrequency); 
  ble.addTimer(&send_characteristic);
}

void loop() 
{

  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float cm;
  float inches;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  while ( digitalRead(ECHO_PIN) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  // TODO: We probably need to check for a timeout here just in case
  // the ECHO_PIN never goes HIGH... so like
  // while ( digitalRead(ECHO_PIN) == 1 && micros() - t1 < threshold);
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed 
  // of sound in air at sea level (~340 m/s).
  // Datasheet: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
  cm = pulse_width / 58.0;
  inches = pulse_width / 148.0;

  if (cm < 75 && alarm_pause_counter == 0) {
    alarm();
  }
  else {
    if (alarm_pause_counter != 0) {
      if (alarm_pause_counter > 0) {
          alarm_pause_counter -= 60;
          if (alarm_pause_counter < 0) {
            alarm_pause_counter = 0;
          }
      }
    }
    
    // Clean up light
    analogWrite(R_OUT_PIN, 255);
    analogWrite(G_OUT_PIN, 255);
    analogWrite(B_OUT_PIN, 255);
    current_color_idx = 0;
    
    // Turn off sound
    noTone(BUZZER_OUT_PIN);
    current_siren_idx = 0;
  }
  

  // The HC-SR04 datasheet recommends waiting at least 60ms before next measurement
  // in order to prevent accidentally noise between trigger and echo
  // See: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
  delay(60);
}

void alarm() {
  if (ToneCounter < 60) {
    noTone(BUZZER_OUT_PIN);
    ToneCounter = siren_tone[current_siren_idx][1];
    tone(BUZZER_OUT_PIN,siren_tone[current_siren_idx][0], ToneCounter);
    current_siren_idx++;
    if (current_siren_idx >= siren_tone_len) {
      current_siren_idx = 0;
    }
  }
  ToneCounter = ToneCounter - 60;

  if (LightCounter < 60) {
    // Output color level
    analogWrite(R_OUT_PIN, 255 - color_table[current_color_idx][0]);
    analogWrite(G_OUT_PIN, 255 - color_table[current_color_idx][1]);
    analogWrite(B_OUT_PIN, 255 - color_table[current_color_idx][2]);
    LightCounter = 300;
    current_color_idx++;
    if (current_color_idx >= color_table_len) {
      current_color_idx = 0;
    }
  }
  LightCounter -= 60;
}


/**
 * @brief Connect handle.
 *
 * @param[in]  status   BLE_STATUS_CONNECTION_ERROR or BLE_STATUS_OK.
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void bleConnectedCallback(BLEStatus_t status, uint16_t handle) {
  switch (status) {
    case BLE_STATUS_OK:
      Serial.println("BLE device connected!");
      digitalWrite(BLE_DEVICE_CONNECTED_DIGITAL_OUT_PIN, HIGH);
      break;
    default: break;
  }
}

/**
 * @brief Disconnect handle.
 *
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void bleDisconnectedCallback(uint16_t handle) {
  Serial.println("BLE device disconnected.");
  digitalWrite(BLE_DEVICE_CONNECTED_DIGITAL_OUT_PIN, LOW);
}

/**
 * @brief Callback for receiving data from Android (or whatever device you're connected to).
 *
 * @param[in]  value_handle  
 * @param[in]  *buffer       The buffer pointer of writting data.
 * @param[in]  size          The length of writting data.   
 *
 * @retval 
 */
int bleReceiveDataCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size) {

  if (receive_handle == value_handle) {
    memcpy(receive_data, buffer, RECEIVE_MAX_LEN);
    Serial.print("Received data: ");
    for (uint8_t index = 0; index < RECEIVE_MAX_LEN; index++) {
      Serial.print(receive_data[index]);
      Serial.print(" ");
    }
    Serial.println(" ");
    
    // process the data. 
    if (receive_data[0] == 0x01) { //receive the face data 
      _posServo.write((int)receive_data[4]);
    }
    else if (receive_data[0] == 0x02) { // received voice command
      switch(receive_data[4]) {
        case 0x01: // pause
          alarm_pause_counter = 3000;
          break;
        case 0x02: // turn off alarm
          alarm_pause_counter = -1;
          break;
        case 0x04: // turn on alarm
          alarm_pause_counter = 0;
          break;
        default:
          break;        
      }
    }
  }
  return 0;
}

/**
 * @brief Timer task for sending status change to client.
 * @param[in]  *ts   
 * @retval None
 * 
 * Send the data from either analog read or digital read back to 
 * the connected BLE device (e.g., Android)
 */
static void bleSendDataTimerCallback(btstack_timer_source_t *ts) {
  send_data[0] = (0x0A);
  send_data[1] = (0x00);
  send_data[2] = (0x00);
  send_data[3] = (0x00);

  ble.sendNotify(send_handle, send_data, SEND_MAX_LEN);
   
  // Restart timer.
  ble.setTimer(ts, 2000);
  ble.addTimer(ts);
}
