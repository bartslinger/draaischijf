#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "dial.h"
#include "ringer.h"
#include "config.h"
#include "wificonfig.h"
/*

   Upload command with OTA:
   python ~/.arduino15/packages/esp8266/hardware/esp8266/2.4.1/tools/espota.py -i draaischijf.local -r -f ~/git/draaischijf/draaischijf.ino.generic.bin

being called:
NUM

+316xxxxxxxx

MP

NUM

+316xxxxxxxx

MP

*/

enum Mode {
  NORMAL,
  PROGRAM
};


Mode run_mode = NORMAL;

volatile bool tick = false;

volatile uint32_t realtime_error_count = 0;

void ICACHE_RAM_ATTR onTimerISR() {
  if (tick) {
    realtime_error_count++;
  }
  tick = true;
  // Execute at 100Hz
}

void setup() {
  pinMode(EARPIECE, INPUT);
  pinMode(DIAL_PIN, INPUT);
  pinMode(RINGER_RESET_PIN, OUTPUT);
  pinMode(RINGER_STEP_PIN, OUTPUT);
  digitalWrite(RINGER_RESET_PIN, LOW);
  digitalWrite(RINGER_STEP_PIN, LOW);

  if (digitalRead(EARPIECE) == HIGH) {
    run_mode = PROGRAM;
  }

  switch (run_mode) {
    case NORMAL:
      setup_normal_mode();
      break;

    case PROGRAM:
      setup_program_mode();
      break;
  }

}

void setup_normal_mode() {
  Serial.begin(115200);
  //Serial.println("Booting normal mode");

  
  dial_state = &Dial_waiting_for_number;
  dial_state(ENTRY);

  ringer_state = &Ringer_off;
  ringer_state(ENTRY);
  
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(10000 * 5); // 10 ms
}

void setup_program_mode() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("draaischijf");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool pb_state = true;

void loop() {

  switch (run_mode) {
    case NORMAL:
      loop_normal_mode();
      break;

    case PROGRAM:
      loop_program_mode();
      break;
  }
}

uint32_t loop_counter = 0;
char serial_buffer[30];
uint8_t sbuf_idx = 0;
bool ringing = true;

unsigned long last_ring_request = 0;
bool earpiece_state = false;
bool pushbutton_state = true;

void loop_normal_mode() {
  if (tick) {

    unsigned long now = millis();
    while(Serial.available()) {
      serial_buffer[sbuf_idx++] = Serial.read();
      if (sbuf_idx >= 30 || serial_buffer[sbuf_idx-1] == '\n') {
        // parse buffer
        if (!strncmp(serial_buffer, "NUM", 3)) {
          last_ring_request = now;
        } else if (!strncmp(serial_buffer, "MP", 2)) {

        }
        sbuf_idx = 0;
      }
    }
    
    // ringer
    bool new_ringing = (now - last_ring_request <= 4000);
    if (new_ringing != ringing) {
      ringing = new_ringing;
      // change event
      if (ringing && !earpiece_state) {
          ringer_state(START_RINGING);
      } else {
          ringer_state(STOP_RINGING);
      }
    }
    ringer_state(PERIODIC);

    // pickup the phone
    bool new_earpiece_state = digitalRead(EARPIECE);
    if (new_earpiece_state != earpiece_state) {
      earpiece_state = new_earpiece_state;
      if (earpiece_state) {
        // answer call
        Serial.print("AT#CE\r\n");
        ringer_state(STOP_RINGING);
      } else {
        // end call
        Serial.print("AT#CG\r\n");
        Dial_state_transition(&Dial_waiting_for_number);
        dial_output.ready = false;
      }
    }
    
    dial_input.dial_pin = digitalRead(DIAL_PIN);
    dial_state(PERIODIC);
    loop_counter++;
    if (loop_counter == 500) {
      loop_counter = 0;
      //Serial.println(digitalRead(EARPIECE));
      //Serial.print("RT errors: ");
      //Serial.println(realtime_error_count);
    }

    if (dial_output.ready && earpiece_state) {
      dial_output.ready = false;
      Serial.print("AT#CW");
      Serial.print(dial_output.number);
      Serial.print("\r\n");
    }

    bool new_pushbutton_state = digitalRead(PUSHBUTTON);
    if (pushbutton_state != new_pushbutton_state) {
      pushbutton_state = new_pushbutton_state;
      if (!pushbutton_state) {
        ringer_state(START_RINGING);
      } else {
        ringer_state(STOP_RINGING);
      }
    }

    tick = false;
  }
}

void loop_program_mode() {
  ArduinoOTA.handle();
}

