/*
HC-SR04 Ping distance sensor
*/

#include <SoftwareSerial.h>
#include <MP3Trigger.h>
#include <Servo.h>
#include <LED.h>

// Pin assignments:
#define triggerRx 2
#define triggerTx 3
#define buttonServoPin 5
#define ledPin 6
#define headServoPin 9
#define buttonPin 10
#define trigPin 11
#define echoPin 12
#define bodyLedPin 13

long duration, distance, lastDistance, count;

// Button
int buttonState = 0;         // variable for reading the pushbutton status

// Servo Stuff
Servo headServo;  // create servo object to control a servo
int headServoPos = 80;    // variable to store the servo position
int randomDestinationPos = headServoPos;    // variable to store the servo position
int headIncrement = 3 ;
int foundTargetPos = 0 ;
const int headServoMaxLeft = 73 ;
const int headServoMaxRight = 87 ;
Servo buttonServo;  // create servo object to control the buttonServo
const int buttonServoUp = 80 ;
const int buttonServoDown = 10 ;
int buttonServoPos = buttonServoUp;    // start position for the button; up


// MP3 Trigger:
SoftwareSerial trigSerial = SoftwareSerial(triggerRx, triggerTx);
MP3Trigger trigger;
boolean played_panic_sound, played_close_sound, played_far_sound;

// Led:
LED led = LED(ledPin);
int brightness = 64 ;
int fadeSpeed = 3 ;

LED bodyLed = LED(bodyLedPin);

// Behaviour variables
String mode ;  // awake, sleeping, or dead
String state ;  // panic, close, or far
long snoozeTimer;
const int panicDistance = 15 ;
const int closeDistance = 30 ;
const int farDistance = 45 ;
const int snoozeAmount = 35 ;

void setup() {
  count = 0 ; // set our timer to 0 - should be incremented every 10msec
  mode = "dead" ;  // We start off dead - no activity
  state = "far" ;  // Initialize a state

  delay(2000); // wait for MP3 trigger to boot
  
  // Start serial communication with the trigger (over SoftwareSerial)
  trigger.setup(&trigSerial);
  trigSerial.begin( MP3Trigger::serialRate() );

  // Debug output to serial
  Serial.begin (9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  randomSeed(analogRead(0));
  
  headServo.attach(headServoPin);  // attaches the servo on pin 9 to the servo object
  buttonServo.attach(buttonServoPin);  // attaches the servo on pin 9 to the servo object

  headServo.write(headServoPos);
  buttonServo.write(buttonServoUp);


  played_panic_sound = false ;
  played_close_sound = false ;
  played_far_sound = false ;
}

int getDistance() {
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  return min(distance,100) ; // no use for data > 100
}

void getKilled() {
    led.on() ;
    trigger.trigger(random(10, 22));

    // Random death throes:
    for (int i=0; i <= 2; i++){
//    while( trigger.mPlay() ) {
        headServo.write(random(70,120));
        led.blink(random(40,70),random(5,15)) ;
        delay(100);
    }
    buttonServo.write(buttonServoUp) ;
    trigger.trigger(22); // 022 Switch_Off_Final.mp3
    led.fadeOut( 1500 ) ;
    delay(1000);
    mode = "dead" ;
}

void resurrect() {
    // Hello!
    headServo.write(90);
    buttonServo.write(buttonServoUp) ;
    led.on() ;
    trigger.trigger(random(52, 57));
    mode = "awake" ;
    snoozeTimer = 0 ;
    delay(1500);
}

void napTime() {
    headServo.write(90);
    trigger.trigger(random(40, 45));
    led.fadeOut( 1500 ) ;
    mode = "sleeping" ;
    buttonServo.write(buttonServoUp) ;
}

void updateLed() {
  if( mode == "awake" and (count%distance) == 0 and state != "far" ) {
        led.toggle() ; // closer = faster blinking
  } else if( mode == "awake" and (count%30) == 0 and state == "far" ) {
        led.on() ; // far = no blinking
  } else if( mode == "sleeping" and (count%5) == 0 ) {
    // change the brightness for next time through the loop:
    brightness = brightness + fadeSpeed;
    
    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 5 || brightness >= 100) {
        fadeSpeed = -fadeSpeed;
    }
    led.setValue(brightness) ;
  }
}

void loop() {

  if( (count%10) == 0 ) { // Every 100ms read buttonState
    buttonState = digitalRead(buttonPin);

    if( buttonState == HIGH and mode == "dead" ) {
      resurrect() ;
    }
    
    if( buttonState == HIGH and mode == "sleeping" ) {
      getKilled() ;
    }
  }

  if( (count%20) == 0 and mode == "awake" ) { // Every 300ms
    distance = getDistance() ;
    lastDistance = distance ;
    bodyLed.blink(10,1);
    distance = ( lastDistance + distance ) / 2 ; // average out the 
  }

  if( (count%20) == 0 and mode == "awake" ) {
    if ( distance < panicDistance ) {
          if( state != "panic" ) {        
//          if( ! played_panic_sound ) {
            trigger.trigger(random(60, 66));
            Serial.println("Played panic sound");
          }
         snoozeTimer = 0 ;                // Something is happening, reset snoozeTimer          
         foundTargetPos = headServoPos ;  // We found a target to fixate on at headServoPos
         state = "panic" ;
         buttonServo.write(buttonServoDown) ;
        
      } else if (distance < closeDistance ) {
          if( state != "close" and ! trigger.mPlay() ) {
//          if( ! played_close_sound ) {
            trigger.trigger(random(1, 8));
            Serial.println("Played close sound");

          }
         snoozeTimer = 0 ;                // Something is happening, reset snoozeTimer
         foundTargetPos = headServoPos ;  // We found a target to fixate on at headServoPos         
         state = "close" ;
          buttonServo.write(buttonServoUp) ;         
      } else if( distance > farDistance ) {
          if( state != "far" and ! trigger.mPlay() ) {
//          if( ! played_far_sound ) {
            trigger.trigger(random(30, 35));
            snoozeTimer = 0 ;
            Serial.println("Played far sound");
          }
  //       delay(1500);
         foundTargetPos = 0 ;    // start sweeping again
         state = "far" ;
         buttonServo.write(buttonServoUp) ;
      }
      if( distance > farDistance and state == "far" ) {  // If we are in state FAR already we need to get ready to sleep
         snoozeTimer++ ;
      }
  }

  if( (count%30) == 0 and mode == "awake" ) {
      /* If we have no target, sweep wider */
      if( foundTargetPos == 0 ) {
        if( headServoPos == randomDestinationPos ) {
          randomDestinationPos = random( headServoMaxLeft, headServoMaxRight + 1 ) ;
        }
        
        if( headServoPos < randomDestinationPos ) {
           headServoPos += headIncrement ;
        } else if( headServoPos > randomDestinationPos ) { 
           headServoPos -= headIncrement ;
        } 
    
      /* If we have found a target, we fixate on it! */
      } else {
        if( random(0,2) == 0 ) {
           headServoPos = foundTargetPos + 2 ;
        } else {
          headServoPos = foundTargetPos - 2 ;
        }
      }

      headServo.write(headServoPos);              // tell servo to go to position in variable 'pos'
//      delay(500);                       // waits 15ms for the servo to reach the position
  }

  // Sleep mode activated:
  if( snoozeTimer > snoozeAmount and mode == "awake" ) {
    napTime() ;
  }

  // Update MP3 trigger state:
  if( (count%10) == 0 ) {  // Every ~100ms
    trigger.update();
  }

  updateLed() ; // call the update led subroutine, which does it's own timing

  if( (count%50) == 0 ) {  // Every ~500ms
    Serial.print("mode: ");
    Serial.print(mode);
    Serial.print(" state: ");
    Serial.print(state);
    Serial.print(" distance: ");
    Serial.print(distance);
    Serial.print(" button: ");
    Serial.print(buttonState);
    Serial.print(" headServoPos: ");
    Serial.print(headServoPos);
    Serial.print(" trigger.mplay: ");
    Serial.print( trigger.mPlay() ) ;
    Serial.print("; snoozetimer:");
    Serial.println(snoozeTimer);
  }

  count++ ;
  delay(10);
}
