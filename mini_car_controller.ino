/*****************************************************************************************************************************************/
//Power wheels drive-by-wire controller code
//By Mom's Mechanic
/*****************************************************************************************************************************************/
// Variables, Defines
/*****************************************************************************************************************************************/
// Direction
#define forward_mode    1
#define stopped    0
#define reverse_mode   -1

// Commands
#define cmd_none   0
#define cmd_fwd    1
#define cmd_rev    3

// Input definitions
#define ON  LOW
#define OFF  HIGH

// Input pin definitions
#define gazpin 4  // High speed
#define revpin  2  // Reverse
#define fwdindicatorpin 7
#define revindicatorpin 6
#define lightswitchpin 8
#define lightpin 12

// Output pin definitions
#define fwdpwm 9   // PWM for forward
#define revpwm 11  // PWM for reverse

// Variables
int pwmspeed=0;            // To store current commanded speed: value may be from -255 (reverse) to 255 (forward). Zero means stopped
byte command;              // to store Commands
boolean revstate = false;
boolean lighton = false;
int lightbuttonstate;
int lastlightbuttonstate = OFF;

// Values for accel/decel/braking
#define accel_rate  4
#define decel_rate    3
#define brake_rate    6

// Value for delay when changing direction without stopping first
#define directionchangedelay 1000

// values for maximum commanded motor speed (maximum is 255 for forward directions, and -255 for reverse)
#define maxfwd  255
#define maxrev  -255

/*****************************************************************************************************************************************/

long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;

void setup()
{
  //Serial.begin(9600);
  // Set up input & output pin modes
  pinMode(lightswitchpin,INPUT_PULLUP);
  pinMode(gazpin,INPUT_PULLUP);
  pinMode(revpin,INPUT_PULLUP);
  pinMode(lightpin,OUTPUT);
  pinMode(fwdpwm,OUTPUT);
  pinMode(revpwm,OUTPUT);
  pinMode(fwdindicatorpin,OUTPUT);
  pinMode(revindicatorpin,OUTPUT);
}
  
void loop()
{
  //read the pin statuses
  command=readCommand();
  displayDirectionStatus();
  setLightState(lighton);
  switch(command)
  {
    case cmd_none:
    {
      if(pwmspeed != 0){
        //Serial.println("slowdown");
        slowdown();
      }
      break;
    }
    case cmd_fwd:
    {
      //Serial.println("forward");
      forward();
      break;
    }
    case cmd_rev:
    {
      //Serial.println("reverse");
      reverse();
      break;
    }
    default:
    {
      //Error! ALL STOP!
      //Serial.println("ALLSTOP!");
      allstop();
      break;
    }
  }
}
 
/*****************************************************************************************************************************************/
int readCommand(){
  // Read the input pins and return a value for the current input state
  int count=0;
  if(digitalRead(gazpin)==ON){
    //Serial.println("gazpinON");
    count+=1;
  }
  revstate = (digitalRead(revpin)==ON);
  
  if(count > 0 && revstate){
    //Serial.println("revpinON");
    count+=2;
  }

  int actuallightbuttonstate = digitalRead(lightswitchpin);

  if (actuallightbuttonstate != lastlightbuttonstate) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (actuallightbuttonstate != lightbuttonstate) {
      lightbuttonstate = actuallightbuttonstate;
  
      // only toggle the LED if the new button state is HIGH
      if (lightbuttonstate == ON) {
        lighton = !lighton;
      }
    }
  }
  lastlightbuttonstate = actuallightbuttonstate;
  revstate = (digitalRead(revpin)==ON);

  return count;
}
/*****************************************************************************************************************************************/
/*****************************************************************************************************************************************/
void slowdown(){
  // slows vehicle down when pedal is released
  if(pwmspeed>0){ // motor is currently set to forward
    pwmspeed-=decel_rate;
    if(pwmspeed<0){
      pwmspeed=0;
    }
  } else
  if(pwmspeed<0){ // motor is current set to reverse 
    pwmspeed+=decel_rate;
    if(pwmspeed>0){
      pwmspeed=0;
    }
  }
  commandMotor();
}
/*****************************************************************************************************************************************/
void forward(){
  if(pwmspeed==0){ // go from stopped to fwd2
    accelerate(forward_mode);
  } else 
  if(pwmspeed<0){ // go from reverse to fwd2
    brake();
  } else
  if(pwmspeed<maxfwd){  // continue to accelerate to fwd2
    accelerate(forward_mode);
  }
}
/*****************************************************************************************************************************************/
void reverse(){
  if(pwmspeed==0){   // go from stopped to reverse
    accelerate(reverse_mode);           // change to acceratehi() if you want to reverse at a quicker rate!
  } else 
  if(pwmspeed>0){   // go from fwd1/2 to reverse
    brake();
  } else
  if(pwmspeed>maxrev){        // continue to accelerate to reverse
    accelerate(reverse_mode);           // change to acceratehi() if you want to reverse at a quicker rate!
  }
}
/*****************************************************************************************************************************************/
void allstop(){
  // Emergency brake! Used when Error condition detected
  pwmspeed=0;
  commandMotor();
  delay(3000); // delay before starting up again
}
/*****************************************************************************************************************************************/
/*****************************************************************************************************************************************/
void accelerate(int direction){

  pwmspeed+=(direction * accel_rate);
  if (pwmspeed>maxfwd) {
    pwmspeed = maxfwd;
  }
  if (pwmspeed<maxrev) {
    pwmspeed = maxrev;
  }
  commandMotor();
}
/*****************************************************************************************************************************************/
void brake(){
  // Stop at high rate, used when lever is changed direction and pedal is pressed before vehicle has come to a stop.
  if(pwmspeed>0){  // slow from forward direction
    pwmspeed-=brake_rate;
    if(pwmspeed<0){
      pwmspeed=0;
    }
  } else
  if(pwmspeed<0){  // slow from reverse direction
    pwmspeed+=brake_rate;
    if(pwmspeed>0){
      pwmspeed=0;
    }
  }
  commandMotor();
  if(pwmspeed==0){    // add a delay (that'll teach 'em not to mess around with the lever!)
    delay(directionchangedelay);
  }
}
/*****************************************************************************************************************************************/
/*****************************************************************************************************************************************/
void commandMotor(){
  //Serial.println(pwmspeed);
  delay(10);  
  // send the command to the motor
  if(pwmspeed==0){ // All stopped
    analogWrite(fwdpwm,0);
    analogWrite(revpwm,0);
  } else 
  if(pwmspeed>0){ // forward motion
    analogWrite(revpwm,0);
    analogWrite(fwdpwm,pwmspeed);
  } else {  // reverse motion
    analogWrite(fwdpwm,0);
    analogWrite(revpwm,-1*pwmspeed);
  }
}
/*****************************************************************************************************************************************/
void displayDirectionStatus() {
  if (pwmspeed < 0) {
    digitalWrite(revindicatorpin, HIGH);
    digitalWrite(fwdindicatorpin, LOW);
  }
  if (pwmspeed > 0) {
    digitalWrite(fwdindicatorpin, HIGH);
    digitalWrite(revindicatorpin, LOW);
  }
  if (pwmspeed == 0) {
    if (revstate) {
      digitalWrite(revindicatorpin, HIGH);
      digitalWrite(fwdindicatorpin, LOW);
    } else {
      digitalWrite(fwdindicatorpin, HIGH);
      digitalWrite(revindicatorpin, LOW);
    }
  }
}

void setLightState(boolean state) {
  if (state) {
    digitalWrite(lightpin, HIGH);
  } else {
    digitalWrite(lightpin, LOW);
  }
}
