#include "HomeSpan.h" 
#include "GarageDoor.h"

// When one of the pins changes, toggle a variable to TRUE so we can respond inside the main loop()
bool sensor_interrupt = false;
void handle_sensor_change() {
  sensor_interrupt = true;
}

static uint32_t next_state_check_millis = 0;
GarageDoor *garageDoor;

void setup() {
  Serial.begin(115200);

  homeSpan.begin(Category::GarageDoorOpeners, "Porte de garage");
  homeSpan.setLogLevel(2);

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify(); 
  new Characteristic::Manufacturer("Benjamin Michotte");
  new Characteristic::SerialNumber("123-ABC-123");
  new Characteristic::Model("ESP32-garage-lock");
  new Characteristic::FirmwareRevision("0.1");
  garageDoor = new GarageDoor();

  pinMode(PIN_SENSOR_OPENED, INPUT_PULLUP);
  pinMode(PIN_SENSOR_CLOSED, INPUT_PULLUP);

  // Set the control pin to output
  pinMode(PIN_OPERATOR_CONTROL, OUTPUT);

  // Set interrupts to watch for changes in the open/close sensors
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_CLOSED), handle_sensor_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_OPENED), handle_sensor_change, CHANGE);

  const uint32_t t = millis();
	next_state_check_millis = t + 30 * 1000;
}

// Called when setting target door state
int get_current_door_state() {
  // Stash the current state so we can detect a change
  int current_state = garageDoor->current->getVal();
  int target_state = garageDoor->target->getVal();

  // Read the sensors and use some logic to determine state
  if (digitalRead(PIN_SENSOR_OPENED) == LOW) {
    // If PIN_SENSOR_OPENED is low, it's being pulled to ground, which means the switch at the top of the track is closed, which means the door is open
    current_state = currentDoorStateOpen;
  } else if (digitalRead(PIN_SENSOR_CLOSED) == LOW) {
    // If PIN_SENSOR_CLOSED is low, it's being pulled to ground, which means the switch at the bottom of the track is closed, which means the door is closed
    current_state = currentDoorStateClosed;
  } else {
    // If neither, then the door is in between switches, so we use the last known state to determine which way it's probably going
    if (current_state == currentDoorStateClosed) {
      // Current door state was "closed" so we are probably now "opening"
      current_state = currentDoorStateOpening;
    } else if ( current_state == currentDoorStateOpen) {
      // Current door state was "opened" so we are probably now "closing"
      current_state = currentDoorStateClosing;
    }

    // If it is traveling, then it might have been started by the button in the garage. Set the new target state:
    if (current_state == currentDoorStateOpening) {
      target_state = targetDoorStateOpen;
    } else if (current_state = currentDoorStateClosing) {
      target_state = targetDoorStateClosed;
    }
    // ... and then notify HomeKit clients
    LOG1("Target door state: %i", target_state);
    garageDoor->target->setVal(target_state);
  }

	LOG1("Current door state: %i", current_state);
	return current_state;
}

void loop() {
  if (sensor_interrupt) {
    const int new_state = get_current_door_state();
    garageDoor->current->setVal(new_state);
    sensor_interrupt = false;
  }  

  homeSpan.poll();
}