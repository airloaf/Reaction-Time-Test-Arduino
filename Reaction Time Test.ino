#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Constants to represent Buzzer pin, LED pins, number of LEDS and the center LED index
const int BUZZER = 8;
const int numLEDS = 4;

const int ledInputs[] =   {2,  0,  1,  3};
const int ledOutputs[] = {12, 10, 11, 13};

const int centerIndex = 0;

// Array to hold baseline for each LED
// used to determine if light is on LED
int bases[numLEDS];

// The current LED index
int currentLED;

// The LCD display object
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/*
 * Enum to represent the current state of the device
 * 
 * baseline - Represents the state in which we gather
 * sensor values to create a baseline light reading
 * 
 * switch - Represents the state when switching between 
 * LEDs
 * 
 * sense - Represents the state in which we compare the
 * baseline to the current sensor values to figure out
 * if the laser has been pointed to the led
 * 
 */
enum SensorState{
  BASELINE,
  SWITCH,
  SENSE
} s;


// Number of readings for the baseline
const int MAX_READINGS = 10;

// Std deviation and the treshold z-score for outliers
int stdDev = 30;
int threshold = 3;

// hold the currentTime for determining reaction time
// and the bestDif for holding the best reaction time
long currentTime;
int bestDif;

// Prints the name of the project onto the LCD
void printTitle(){
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Reaction Time");
  lcd.setCursor(5,1);
  lcd.print("Test");
}

// Prints the best and current reaction time
void printTimes(){
    long previousTime = currentTime;
    currentTime = millis();

    // Calculated reaction time
    int dif = (currentTime - previousTime);

    // Check if new time is better than previous best
    if(bestDif > dif || bestDif == -1){
      bestDif = dif;
    }

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Current: " );
    lcd.setCursor(9,0);
    lcd.print(dif);
    lcd.setCursor(14,0);
    lcd.print("ms");

    lcd.setCursor(0,1);
    lcd.print("Best: ");
    lcd.setCursor(9, 1);
    lcd.print(bestDif);
    lcd.setCursor(14,1);
    lcd.print("ms");
}

// Prints a status update that baselines are being collected for each LED
void printBaseline(){
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Gathering");
  lcd.setCursor(3,1);
  lcd.print("Baselines");
}

// Prints a status update that it is recording your time
void printRecording(){
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Recording");
  lcd.setCursor(5,1);
  lcd.print("Time");
}

void setup(){

  // Initialize serial communications
  Serial.begin(9600);

  // Set the random seeds
  randomSeed(analogRead(0));
  
  // Initialize the lcd display
  lcd.begin(16,2);

  // Set the LED outputs
  for(int LED: ledOutputs)
    pinMode(LED, OUTPUT);

  // Set the current LED to the center most one
  currentLED = centerIndex;

  // Set the current best to an arbitary number, -1
  bestDif = -1;

  // Set the current state to the baseline
  s = BASELINE;
  
}

void loop(){

  // Detect which state we are in and go to the associated method call
  switch(s){
    case BASELINE:
      baseline();
    break;
    case SWITCH:
      switchLEDs();
    break;
    case SENSE:
      sense();
    break;
  }
  
}

// Method to collect a baseline
void baseline(){

  // Update status to collecting baselines
  printBaseline();

  // Delay to avoid reading spike at the beginning
  delay(250);

  // time to delay so we can obtain all readings in 1 second (1000 ms).
  float readingDelay = 1000 / MAX_READINGS;

  // Sums for each LED for calculating base
  int sumReadings[numLEDS];
  for(int led = 0; led < numLEDS; led++){
    sumReadings[led] = 0;
  }

  // Sum the readings and delay accordingly
  for(int i = 0; i < MAX_READINGS; i++){
    for(int led = 0; led < numLEDS; led++){
      sumReadings[led] += analogRead(ledInputs[led]);
    }
      delay(readingDelay);
  }

  // Obtain the average
  for(int led = 0; led < numLEDS; led++){
    bases[led] = sumReadings[led] / MAX_READINGS;
    Serial.println(bases[led]);
  }

  // Set the current lit LED to be the center LED
  // set it to high and switch the state to sense light values
  currentLED = centerIndex;
  digitalWrite(ledOutputs[currentLED], HIGH);
  s = SENSE;

  // Prints the name to indicate it is ready for testing
  printTitle();
  
}

// Switches the active LED
void switchLEDs(){

  // Checks if the center LED was the previous LED hit
  if(currentLED == centerIndex){

    // Choose the next LED and start recording time
    currentTime = millis();

    currentLED = random(1, numLEDS);
    printRecording();
    
  }else{
    // Print reaction time and set the next LED to the center most one.
    printTimes();
    currentLED = centerIndex;
  }

  // Sets the current LED on and switches states to sense
  digitalWrite(ledOutputs[currentLED], HIGH);

  s = SENSE;
  
}

// Senses light values
void sense(){
  
  // get the sensor reading
  int val = analogRead(ledInputs[currentLED]);

  // detect outlier
  if( val >= 3* stdDev + bases[currentLED]){
    // Turn off the LED
    digitalWrite(ledOutputs[currentLED], LOW);

    // Buzz to indicate its been hit and switch states to Switch state
    tone(BUZZER, 500, 500);
    Serial.println(val);
    s = SWITCH;
  }
  
}
