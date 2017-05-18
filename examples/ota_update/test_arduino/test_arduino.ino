/*
   Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>

   This file is subject to the terms and conditions of the GNU Lesser
   General Public License v2.1. See the file LICENSE in the top level
   directory for more details.
*/

/*
   This adruino sketch is part of an integration test script for
   testing the RIOT OTA update mechanism.
*/

String command;

/*
   Seedstudio Relay shield v3:
   relais 1: pin 7
   relais 2: pin 6
   relais 3: pin 5
   relais 4: pin 4      connect to Nucleo power jumper (use NC)
*/

int pin_pwr = 4;

void setup() {
  pinMode(pin_pwr, OUTPUT);

  digitalWrite(pin_pwr, LOW);

  Serial.begin(115200);
}

void loop() {
  command = "";

  while (Serial.available()) {
    if (Serial.available() > 0) {
      char data = Serial.read();
      command += data;
    }
    delay(3);
  }

  if (command.length() > 0) {
    if (command.equals("HELLO\n")) {
      digitalWrite(pin_pwr, LOW);
      digitalWrite(pin_rst, HIGH);
      Serial.write("ACK.HELLO\n");

    } else if (command.startsWith("PIN.")) {
      if (command.equals("PIN.PWR.0\n")) {
        digitalWrite(pin_pwr, LOW);
        Serial.write("ACK.PWR.0\n");

      } else if (command.equals("PIN.PWR.1\n")) {
        digitalWrite(pin_pwr, HIGH);
        Serial.write("ACK.PWR.1\n");

      } else {
        Serial.write("ERR.PIN\n");
      }

    } else {
      Serial.write("ERR\n");
    }
  }

  delay(20);

}
