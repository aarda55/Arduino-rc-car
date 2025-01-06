const int tolerance = 10;       //tolarance value for inputs
const int minPwmSignal = 0;    //min PWM-signal strength
const int maxPwmSignal = 255;  //max PWM-signal strength

const int numberOfSpeedSamples = 2;           //number of samples for speed
const int multiplier = 10000;                 //multiplier for movement inputs
const int maxBackward = 0.0555 * multiplier;  //input on max backwards speed
const int noMovement = 0.0849 * multiplier;   //input on no movement speed
const int maxForward = 0.1091 * multiplier;   //input on max forward speed

const int numberOfDistanceSamples = 3;          //number of samples for distance
const float minDistanceToObject = 40;           //set the distance at which the car does not drive forward anymore
const float speedLimitationDistance = 280;      //set the distance in cm at which the speed will be limited depending on the distance
const int maxPwmSignalWithObjectInFront = 180;  //the max PWM-signal strength when an object is in the distance to trigger the limitation

const int minVoltageInput = 800;  //min value that should be read from the voltageInputPin to prevent destroying the batterie (below 3.3V per cell) (minInput = 1023 / (4.2V) * minVoltagePerCell)

const int controllerInputPin = 9;
const int forwardPin = 6;
const int backwardPin = 5;
const int connectedPin = 4;
const int bridgeForwardEnablePin = 7;
const int bridgeBackwardEnablePin = 8;
const int voltageInputPin = A0;
const int ultraSchallTriggerPin = 10;
const int ultraSchallEchoPin = 11;

void setup() {
  pinMode(controllerInputPin, INPUT);
  pinMode(forwardPin, OUTPUT);
  pinMode(backwardPin, OUTPUT);
  pinMode(connectedPin, INPUT);
  pinMode(bridgeForwardEnablePin, OUTPUT);
  pinMode(bridgeBackwardEnablePin, OUTPUT);
  pinMode(voltageInputPin, INPUT);
  pinMode(ultraSchallTriggerPin, OUTPUT);
  pinMode(ultraSchallEchoPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  //Stop movement to prevent destroying the batterie
  if (!hasEnoughBatteryPower()) {
    setBrideEnabled(false);
    return;
  }

  //Test if controller is connected and stop on disconnect
  if (!isConnected()) {
    setBrideEnabled(false);
    return;
  }

  //Enable H-Bridge
  setBrideEnabled(true);

  drive();
}

/**
 * Check if the battery has enough power
 */
bool hasEnoughBatteryPower() {
  bool hasEnoughPower = analogRead(voltageInputPin) > minVoltageInput;
  //Serial.print("Has enough battery power: ");
  //Serial.println(hasEnoughPower);
  return hasEnoughPower;
}

/**
 * Check if the controller is connected
 */
bool isConnected() {
  bool connected = pulseIn(connectedPin, HIGH) > tolerance;
  //Serial.print("Is connected: ");
  //Serial.println(connected);
  return connected;
}

/**
 * Enable or disable movement completely
 */
void setBrideEnabled(bool enabled) {
  digitalWrite(bridgeForwardEnablePin, enabled);
  digitalWrite(bridgeBackwardEnablePin, enabled);
}

/**
 * Make the car drive
 */
void drive() {
  //save a few samples into an array
  float highLowPercentages[numberOfSpeedSamples];
  for (int i = 0; i < numberOfSpeedSamples; ++i) {
    float high = pulseIn(controllerInputPin, HIGH);
    float low = pulseIn(controllerInputPin, LOW);
    highLowPercentages[i] = high / (high + low);
  }

  //Calculate average
  float averageHighLowPercentage = 0;
  for (int i = 0; i < numberOfSpeedSamples; ++i) {
    averageHighLowPercentage += highLowPercentages[i];
  }
  averageHighLowPercentage /= numberOfSpeedSamples;

  //Serial.print("High-Low-Percentage: ");
  //Serial.println(averageHighLowPercentage, 4);
  writeSpeed(averageHighLowPercentage);
}

/**
 * Map a given int value to a PWM-Signal for speed
 */
int mapToPwmSignal(int value, int minValue, int maxValue) {
  int mappedPwmSignal = map(value, minValue, maxValue, minPwmSignal, maxPwmSignal);
  //Serial.print("Mapped PWM-Signal: ");
  //Serial.println(mappedPwmSignal);
  if (mappedPwmSignal <= tolerance) return 0;
  return min(mappedPwmSignal, maxPwmSignal);
}

/**
 * Set the speed
 */
void writeSpeed(float value) {
  int intValue = (int)(value * multiplier);  //make input to an int to be comparble with the movement speed constants
  int mappedPwmSignal;

  if (intValue < noMovement) {
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
  int distanceToNextObject = (int) getDistanceToNextObject();
  if(distanceToNextObject < speedLimitationDistance) {
    //limit speed when an object is oin front depending on the distance
    int limitedMaxSpeed = map(distanceToNextObject, minDistanceToObject, speedLimitationDistance, minPwmSignal, maxPwmSignalWithObjectInFront);
    limitedMaxSpeed = max(limitedMaxSpeed, 0);
    limitedMaxSpeed = min(limitedMaxSpeed, maxPwmSignal);
    mappedPwmSignal = min(limitedMaxSpeed, mappedPwmSignal);
  }

  analogWrite(forwardPin, mappedPwmSignal);
  analogWrite(backwardPin, 0);
  //Serial.print("Forward: ");
  //Serial.println(mappedPwmSignal);
}

/**
 * Get the distance to the next object
 */
float getDistance() {
  digitalWrite(ultraSchallTriggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultraSchallTriggerPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(ultraSchallTriggerPin, LOW);

  float duration = pulseIn(ultraSchallEchoPin, HIGH);
  float distance = (duration * 0.03432) / 2;

  /*Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println("cm");*/

  return distance;
}

/**
 * Can the car drive forward or is an obstacle in front
 */
float getDistanceToNextObject() {
  float distances[numberOfDistanceSamples];

  for(int i = 0; i < numberOfDistanceSamples; ++i) {
      distances[i] = getDistance();
  }

  float averageDistance = 0;
  for(int i = 0; i < numberOfDistanceSamples; ++i) {
    averageDistance += distances[i];
  }
  averageDistance /= numberOfDistanceSamples;

  /*Serial.print("Average-Distance: ");
  Serial.print(averageDistance);
  Serial.println("cm");*/

  return averageDistance;
}
