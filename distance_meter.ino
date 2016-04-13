#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 9
#include <avr/sleep.h>
#include <avr/power.h>

Adafruit_SSD1306 display(OLED_RESET);

// Ultrasonic varuable
#define echoPin 7 // Echo Pin
#define trigPin 8 // Trigger Pin
int maximumRange = 400; // Maximum range needed
int minimumRange = 2; // Minimum range needed
long duration, distance; // Duration used to calculate distance
long listDistance[20];
int indexDistance = 0;
float old_distance = 0;
float new_distance = 0;

int count_error = 0;
String unit_distance[] = {"cm", "m", "ft"};
int index_unit = 0;
//=======================

// button variable
const int unitButton = 4;
const int meterButton = 3;
const int resetButton = 2;
const int buzzerPin = 6; // Buzzer piazo
const int laser = 5; 
//=========================


int ledState[] = {HIGH, HIGH, HIGH};
int buttonState[] = {LOW, LOW, LOW};
int lastButtonState[] = {LOW, LOW, LOW};

long lastDebounceTime = 0;
long debounceDelay = 50;
long count_time = 0; // count time to sleep when no work.
//==========================
int statusAwake = 0;

void pin2Interrupt(void) //Service routine for pin2 interrupt 
{
  /* This will bring us back from sleep. */
  
  /* We detach the interrupt to stop it from 
   * continuously firing while the interrupt pin
   * is low.
   */
   count_time = millis();
   statusAwake = 1;
}

void enterSleep(void)
{ 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); //initialize I2C addr 0x3c
  display.clearDisplay(); // clears the screen and buffer
  display.display();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  sleep_enable();
   /* Setup pin2 as an interrupt and attach handler. */
  attachInterrupt(0, pin2Interrupt, CHANGE);
   
  sleep_mode();
  
  /* The program will continue from here. */
  
  /* First thing to do is disable sleep. */
  sleep_disable(); 

  detachInterrupt(0);
  show_oled(convert_unit(old_distance, unit_distance[index_unit]),
       convert_unit(new_distance, unit_distance[index_unit]), unit_distance[index_unit]);
}

void setup() {
  pinMode(unitButton, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(resetButton, INPUT);
  pinMode(meterButton, INPUT);
  pinMode(laser, OUTPUT);

  digitalWrite(laser, LOW);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(buzzerPin, LOW);
  begin_state();
}

void loop() {
  int unitRead = digitalRead(unitButton);
  int sonicRead = digitalRead(meterButton);
  int resetRead = digitalRead(resetButton);
  debounce_button(unitRead, 0, buzzerPin); // change unit
  sonic_button(sonicRead, 1, buzzerPin); // push to turn on laser then pull up to turn ultrasonic. 
  debounce_button(resetRead, 2, buzzerPin); // reset
  if (millis() - count_time >= 30000 ){ // wait for 10s to sleep mode
    enterSleep();
    }
}

void begin_state(){
   show_oled(0,0, unit_distance[index_unit]);
}

void debounce_button(int reading, int index_ledState, int ledPin){
  if (reading != lastButtonState[index_ledState]){
    lastDebounceTime = millis();
    digitalWrite(ledPin, LOW);
    }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState[index_ledState]) {
      buttonState[index_ledState] = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState[index_ledState] == HIGH) {
        digitalWrite(ledPin, HIGH);
        count_time = millis();
        if(index_ledState == 0){
          index_unit++;
          index_unit %= 3;
          show_oled(convert_unit(old_distance, unit_distance[index_unit]),
              convert_unit(new_distance, unit_distance[index_unit]), unit_distance[index_unit]);
        }
        else{
          if(statusAwake == 0){
             reset();
            }
          else{
            statusAwake = 0;
            }
          }
      }
    }

  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState[index_ledState] = reading;
  }

void sonic_button(int reading, int index_ledState, int ledPin){
  if (reading != lastButtonState[index_ledState]){
    lastDebounceTime = millis();
    digitalWrite(ledPin, LOW);
    }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState[index_ledState]) {
      digitalWrite(ledPin, HIGH);
      buttonState[index_ledState] = reading;
      digitalWrite(laser, HIGH);
      // only toggle the LED if the new button state is HIGH
      if (buttonState[index_ledState] == LOW) {
        count_time = millis();
        digitalWrite(laser, LOW);
        digitalWrite(ledPin, LOW);
        for(int i=0; i<20; i++){
          get_distance();
        }
      }
    }
  }


  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState[index_ledState] = reading; //goto State
  }
  
void get_distance(){
  // open ultra sonic
   digitalWrite(trigPin, LOW); 
   delayMicroseconds(2); 
  
   digitalWrite(trigPin, HIGH);
   delayMicroseconds(10); 
   
   digitalWrite(trigPin, LOW);
   duration = pulseIn(echoPin, HIGH);
   
   //Calculate the distance (in cm) based on the speed of sound.
   distance = duration/58.2;
 
  // calculate distance and write data to oled 
   if (distance >= maximumRange || distance <= minimumRange){
   /* Send a negative number to computer and Turn LED ON 
   to indicate "out of range" */
     count_error++;
     if (count_error >= 10){
       count_error = 0;
       indexDistance = 0;
       show_oled(convert_unit(old_distance, unit_distance[index_unit]),
               -1, unit_distance[index_unit]);
     }
   }
   else {
   /* Send the distance to the computer using Serial protocol, and
   turn LED OFF to indicate successful reading. */
   if (indexDistance < 20){
      listDistance[indexDistance] = distance;
      indexDistance++;
      }
    else{ // get 20 distances to calculate average.
      float sumDistance = 0;
      for(int i=0;i<20; i++){
        sumDistance += listDistance[i];
        }
      sumDistance = sumDistance/20.0;
       new_distance = sumDistance;
       show_oled(convert_unit(old_distance, unit_distance[index_unit]),
           convert_unit(new_distance, unit_distance[index_unit]), unit_distance[index_unit]);
       old_distance = new_distance;
       indexDistance = 0;
      }
   }
   
   //Delay 50ms before next reading.
   delay(50);
  
}

float convert_unit(float distance, String unit){
    if(unit == "m"){
      return distance/100.0;
    }
    else if (unit == "ft"){
      return distance/30.0;
    }    
    else{
      return distance;
    }
}

void show_oled(float old_distance, float new_distance, String unit){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); //initialize I2C addr 0x3c
  display.clearDisplay(); // clears the screen and buffer
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Distance Meter");

  if ((new_distance != 0) && (new_distance != -1)){ // normal get distance
    display.setTextSize(1);
    if(old_distance != 0){
      display.print(old_distance);
      display.println(" "+unit);
      }
    else{
      display.println(" ");
      }
    display.setTextColor(BLACK, WHITE);
    display.setTextSize(2);
    display.print(new_distance);
    display.println(" "+unit);
    display.setTextColor(WHITE, BLACK);
    display.display();
    }
   else if(new_distance == -1){ // out of range.
    display.setTextSize(1.5);
    display.println("Out of range!");
    display.setTextColor(BLACK, WHITE);
    display.display(); 
    delay(1000);
    show_oled(old_distance, 0, unit);
    }
   else if(new_distance == 0){ // begin status
    display.setTextSize(1);
    if (old_distance != 0){
      display.print(old_distance);
      display.println(" "+unit);      
      }
    display.setTextColor(BLACK, WHITE);
    display.display();    
    }
}

void reset(){
  new_distance = 0;
  old_distance = 0;
  begin_state();
}
