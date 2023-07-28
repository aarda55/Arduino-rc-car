const int tolerance = 7;      //tolaranze value for inputs
const int minPwmSignal = 0;   //min PWM-signal strength
const int maxPwmSignal = 255; //max PWM-signal strength

const int multiplier = 10000;                 //multiplier for movement inputs
const int maxBackward = 0.0566 * multiplier;  //input on max backwards speed
const int noMovement = 0.0852 * multiplier;   //input on no movement speed
const int maxForward = 0.1101 * multiplier;   //input on max forward speed

const int numberOfSpeedSamples = 5;
float avgSpeedPercentage[numberOfSpeedSamples];
int i = 0;

const int controllerInputPin = 9;


void setup() {
  pinMode(controllerInputPin, INPUT);;
  Serial.begin(9600);
}

/**
   Map a given int value to a PWM-Signal for speed information
*/
int mapToPwmSignal(int value, int minValue, int maxValue) {
  int mappedPwmSignal = map(value, minValue, maxValue, minPwmSignal, maxPwmSignal);
  Serial.print("Mapped PWM-Signal: ");
  Serial.println(mappedPwmSignal);
  if (mappedPwmSignal <= tolerance) mappedPwmSignal = 0;
  return min(mappedPwmSignal, maxPwmSignal);
}
/**
  Determin wether the signal is a forward or backward moving 
*/
void writeSpeed(float value) {
  int intValue = (int) (value * multiplier);
  int mappedPwmSignal;
  if (intValue < noMovement) {
    //Move backward
    mappedPwmSignal = mapToPwmSignal(intValue, noMovement, maxBackward);
    return;
  }

  //Move forward
  mappedPwmSignal = mapToPwmSignal(intValue, noMovement, maxForward);
}

void calibrateController() {
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

  writeSpeed(averageHighLowPercentage);
}


void loop() {
  calibrateController();
}