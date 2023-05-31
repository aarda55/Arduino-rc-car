const int tolerance = 7;      //tolaranze value for inputs
const int minPwmSignal = 0;   //min PWM-signal strength
const int maxPwmSignal = 255; //max PWM-signal strength

const int multiplier = 10000;                 //multiplier for movement inputs
const int maxBackward = 0.0566 * multiplier;  //input on max backwards speed
const int noMovement = 0.0852 * multiplier;   //input on no movement speed
const int maxForward = 0.1101 * multiplier;   //input on max forward speed

const int minVoltageInput = 800;  //min value that should be read from the voltageInputPin to prevent destroying the battery (below 3.3V per cell) (minInput = 1023 / 8,4 * minVoltage)

const int numberOfSamples = 5;
float avgSpeedPercentage[numberOfSamples];
int i = 0;

const int controllerInputPin = 9;
const int forwardPin = 6;
const int backwardPin = 5;
const int connectedPin = 4;
const int bridgeForwardEnablePin = 7;
const int bridgeBackwardEnablePin = 8;
const int voltageInputPin = A0;

const int UltraSchallTrigger = 10;
const int UltraEcho = 11;
float sum[5];
float distance = 0;

void setup() {
  pinMode(controllerInputPin, INPUT);
  pinMode(UltraSchallTrigger, INPUT);
  pinMode(UltraEcho, INPUT);
  pinMode(forwardPin, OUTPUT);
  pinMode(backwardPin, OUTPUT);
  pinMode(connectedPin, INPUT);
  pinMode(bridgeForwardEnablePin, OUTPUT);
  pinMode(bridgeBackwardEnablePin, OUTPUT);
  pinMode(voltageInputPin, INPUT);
  //Serial.begin(9600);
}

/**
 * Map a given int value to a PWM-Signal for speed
 */
int mapToPwmSignal(int value, int minValue, int maxValue) {
  int mappedPwmSignal = map(value, minValue, maxValue, minPwmSignal, maxPwmSignal);
  //Serial.print("Mapped PWM-Signal: ");
  //Serial.println(mappedPwmSignal);
  if(mappedPwmSignal <= tolerance) mappedPwmSignal = 0;
  return min(mappedPwmSignal, maxPwmSignal);
}

/**
 * Set the speed
 */
void writeSpeed(float value) {
  //Serial.print("High-Low-Percentage: ");
  //Serial.println(value, 4);
  int intValue = (int) (value * multiplier);  //make input to an int to be comparble with the movement speed constants
  int mappedPwmSignal;
  if(intValue < noMovement) {
    //Move backward
    mappedPwmSignal = mapToPwmSignal(intValue, noMovement, maxBackward);
    analogWrite(backwardPin, mappedPwmSignal);
    analogWrite(forwardPin, 0);
    //Serial.print("Backward: ");
    //Serial.println(mappedPwmSignal);
    return;
  }

  //Move forward
  mappedPwmSignal = mapToPwmSignal(intValue, noMovement, maxForward);
  analogWrite(forwardPin, mappedPwmSignal);
  analogWrite(backwardPin, 0);
  //Serial.print("Forward: ");
  //Serial.println(mappedPwmSignal);
}

/**
 * Uses ultra-sound sensor to stop vehicle from contacting obstacles at high speeds
 */
void Distance(){
  analogWrite(UltraSchallTrigger, HIGH);
  delayMicroseconds(10);
  analogWrite(UltraSchallTrigger, LOW);

  float duration = pulseIn(UltraEcho, HIGH, 30000);
  if (duration > 0) {
  float distance = (duration * 0.034) / 2;
  sum[i++]= distance;
  if(i == 5){
    float total = 0;
    for(i=0; i<5; ++i){
      total+=sum[i];
    }
    float average = total/5;
    //Serial.print("Distance: ");
    //Serial.println(average);
    i=0;
  }
  analogWrite(UltraSchallTrigger, LOW);
  delayMicroseconds(10);
}
  while(distance<10){
    digitalWrite(forwardPin, LOW);
  }
}

void loop() {
  
  //Stop movement to prevent destroying the battery
  if(analogRead(voltageInputPin) <= minVoltageInput) {
    digitalWrite(bridgeForwardEnablePin, LOW);
    digitalWrite(bridgeBackwardEnablePin, LOW);
    return;
  }
  
  Distance();

  //Test if controller is connected and stop on disconnect
  float connected = pulseIn(connectedPin, HIGH);
  //Serial.print("Is connected: ");
  //Serial.println(!!connected);
  if(connected < tolerance){
    //Serial.println("Disable H-Bridge");
    digitalWrite(bridgeForwardEnablePin, LOW);
    digitalWrite(bridgeBackwardEnablePin, LOW);
    return;
  }
  //Serial.println("Enable H-Bridge");
  digitalWrite(bridgeForwardEnablePin, HIGH);
  digitalWrite(bridgeBackwardEnablePin, HIGH);

  //Calculate the average speed
  float high = pulseIn(controllerInputPin, HIGH);
  float low = pulseIn(controllerInputPin, LOW);
  avgSpeedPercentage[i++] = high / (high+low);
  if(numberOfSamples == i){
    float ges = 0;
    for(i = 0; i < numberOfSamples; i++){
        ges += avgSpeedPercentage[i];
    }

    //Write the speed
    writeSpeed(ges / numberOfSamples);
    i = 0;
  }
}