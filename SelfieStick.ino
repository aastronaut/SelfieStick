/**
 * Prototype code for selfi-stick by Thomas Waidhofer.
 * 
 * A stick with a tilt sensor, triggering a shot with a relais.
 * The shot processes is indicated by LEDs.
 * 
 * TILT_PIN uses a Force Tilt Sensor to detect the correct
 * position mechanically. I use a lowpass to avoid bouncing,
 * but a timing solution would be more appropriate, due to
 * float arithmetic.
 * 
 * Adjust the pins and add LEDs if you want to.
 * Set the timings accordingly.
 * Configure the force tilt lowpass filter for a good result.
 * 
 * 1/10/2017
 * aastronaut | F. Gutwiller
 * See the LICENCE file in repository.
 */

// pins
const unsigned int TILT_PIN = 12;             // sensor for rotation
const unsigned int RELAIS_PIN = 3;            // relais triggering shot
const unsigned int LED_PINS[] = {4, 5, 6};    // leds indicating process

// timings
const unsigned int BLINK_SEQUENCE = 1500;     // timing between ledchange
const unsigned int AFTER_LAST_LED = 500;      // last led to relais time
const unsigned int RELAIS_TRIGGER_TIME = 20;  // relais trigger
const unsigned int PAUSE_AFTER_SHOT = 5000;   // after shot

// filter, adjust for force tilt sensor
const float LOW_PASS = 0.5f;                  // 1.0 for no filter
const float THRESHOLD_HIGH = 0.7f;            // >= 0.5
const float THRESHOLD_LOW = 0.3f;             // < 0.5



// states
enum States {
  WAIT_FOR_LIFT,  // off state, awaiting positioning
  BLINK_LEDS,     // indicate process by blinking LEDs
  TRIGGER_SHOT,   // trigger relais
  SHOT_DONE,      // turn LEDs and relais off simultaneously
  PAUSE,          // wait until new shot
} state;

// for readability
const int LED_COUNT = sizeof(LED_PINS)/sizeof(*LED_PINS);
void reset(States newState = WAIT_FOR_LIFT);



void setup() {
  // save the attachment of an external resistor
  pinMode(TILT_PIN, INPUT_PULLUP);

  // init outputs
  pinMode(RELAIS_PIN, OUTPUT);
  for(int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  // init state
  state = WAIT_FOR_LIFT;
}



void loop() {
  if(isTilted()) {
    shotCycle();
  }

  // reset only when necessary
  else if(state != WAIT_FOR_LIFT) {
    reset();
  }

  // to save some of an arduinos lifetime
  delay(10);
}



boolean isTilted() {
  static boolean tilted;
  static float tiltLevel;

  float newTilt = (float) digitalRead(TILT_PIN);
  tiltLevel += (newTilt - tiltLevel) * LOW_PASS;

  if(
    (!tilted && tiltLevel >= THRESHOLD_HIGH) ||
    ( tilted && tiltLevel <= THRESHOLD_LOW )
    ) {
    tilted = !tilted;
  }

  return tilted;
}

void shotCycle() {
  static int ledIndex;
  static unsigned long timer;

  unsigned long now;
  
  switch(state) {

    // off state, awaiting positioning
    case WAIT_FOR_LIFT:
      ledIndex = 0;
      timer = millis();
      digitalWrite(LED_PINS[ledIndex++], HIGH);
      state = BLINK_LEDS;
      break;

    // indicate process by blinking LEDs
    case BLINK_LEDS:
      now = millis();
      if(now - timer >= BLINK_SEQUENCE) {
        timer = now;
        digitalWrite(LED_PINS[ledIndex++], HIGH);

        // change state if all LEDs are illuminated
        if(ledIndex == LED_COUNT) {
          state = TRIGGER_SHOT;
        }
      }
      break;

    // trigger relais
    case TRIGGER_SHOT:
      now = millis();
      if(now - timer >= AFTER_LAST_LED) {
        timer = now;
        digitalWrite(RELAIS_PIN, HIGH);
        state = SHOT_DONE;
      }
      break;

    // turn LEDs and relais off simultaneously
    case SHOT_DONE:
      now = millis();
      if(now - timer >= RELAIS_TRIGGER_TIME) {
        timer = now;
        reset(PAUSE);
      }
      break;

    // wait until new shot
    case PAUSE:
      now = millis();
      if(now - timer >= PAUSE_AFTER_SHOT) {
        state = WAIT_FOR_LIFT;
      }
      break;
  }
}

void reset(States newState = WAIT_FOR_LIFT) {
  // shut down relais
  digitalWrite(RELAIS_PIN, LOW);

  // turn off LEDs
  for(int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }

  // reset state
  state = newState;
}
