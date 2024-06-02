const int PIN_OPERATOR_CONTROL = 14; // D5
const int PIN_SENSOR_OPENED = 5;     // D1
const int PIN_SENSOR_CLOSED = 4;     // D2

const int currentDoorStateOpen  = 0;
const int currentDoorStateClosed = 1;
const int currentDoorStateOpening = 2; 
const int currentDoorStateClosing = 3;
const int currentDoorStateStopped = 4; 

const int targetDoorStateOpen = 0;
const int targetDoorStateClosed = 1;

struct GarageDoor : Service::GarageDoorOpener {

  Characteristic::CurrentDoorState *current;            // reference to the Current Door State Characteristic (specific to Garage Door Openers)
  Characteristic::TargetDoorState *target;              // reference to the Target Door State Characteristic (specific to Garage Door Openers)  
  SpanCharacteristic *obstruction;                      // reference to the Obstruction Detected Characteristic (specific to Garage Door Openers)

  GarageDoor() : Service::GarageDoorOpener() {
        
    current = new Characteristic::CurrentDoorState(currentDoorStateClosed);
    target = new Characteristic::TargetDoorState(targetDoorStateClosed);
    obstruction = new Characteristic::ObstructionDetected(false);
    
    Serial.print("Configuring Garage Door Opener");
    Serial.print("\n");

    // force close 
    target->setVal(targetDoorStateClosed);
    current->setVal(targetDoorStateClosed);
  }

  void pushButton() {
    digitalWrite(PIN_OPERATOR_CONTROL, HIGH);
    delay(500);
    digitalWrite(PIN_OPERATOR_CONTROL, LOW);
  }

  boolean update() {
    LOG1("Updating\n");
    if (target->getNewVal() == currentDoorStateOpen) {
      LOG1("Opening Garage Door\n");
      current->setVal(currentDoorStateOpening);
      obstruction->setVal(false);
    } else {
      LOG1("Closing Garage Door\n");
      current->setVal(currentDoorStateClosing);
      obstruction->setVal(false);
    }

    this->pushButton();
    
    return(true);
  }

  void loop() {
    if (current->getVal() == target->getVal()) {
      return;
    }

    // if (current->getVal()== currentDoorStateClosing && random(100000)==0){    // here we simulate a random obstruction, but only if the door is closing (not opening)
    //   current->setVal(currentDoorStateStopped);                             // if our simulated obstruction is triggered, set the curent-state to 4, which means "stopped"
    //   obstruction->setVal(true);                      // and set obstruction-detected to true
    //   LOG1("Garage Door Obstruction Detected!\n");
    // }

    if (current->getVal() == currentDoorStateStopped) {                       // if the current-state is stopped, there is nothing more to do - exit loop()     
      return;
    }

    // This last bit of code only gets called if the door is in a state that represents actively opening or actively closing.
    // If there is an obstruction, the door is "stopped" and won't start again until the HomeKit Controller requests a new open or close action

    // if (target->timeVal() > 5000) {      // simulate a garage door that takes 5 seconds to operate by monitoring time since target-state was last modified
    //   current->setVal(target->getVal()); // set the current-state to the target-state
    // }    
  }
};