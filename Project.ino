#include "HX711.h"

// being falling from chair alert
HX711 hx;

#define  calibration_factor         600       // This value is obtained using the SparkFun_HX711_Calibration sketch
#define  HX711_DOUT_PIN             4         // dout pin
#define  HX711_SCK_PIN              3         // sck pin
#define  NUM_OF_READINGS            20        // used to average on readings
#define  NUM_STORED_READINGS        5         // size of the readings array
#define  MAX_ACCEPTABLE_WEIGHT      5         // if weight is exceeds this -> OVERLOAD!
const double Weight_Threshold = 10; // maximum acceptable [alert-less] rate of change of weight with respect to time

const int button = 2;
const int buzzer = 11;

volatile bool buzzer_ringing = 0;

struct HXPAIR 
{
    // this struct is used to store weight readings / time
    volatile unsigned long t_reading = 0;
    volatile double w_reading = 0;
};

struct HXPAIR readings[NUM_OF_READINGS];
// end falling from chair alert

void setup()
{
    //for DEBUG 
    Serial.begin(9600);
     
    pinMode(button,  INPUT);
    pinMode(buzzer,  OUTPUT);
    buzzer_ringing = LOW;

    attachInterrupt(digitalPinToInterrupt(button),resett,RISING);
    
    // begin falling from chair alert
    hx.begin(HX711_DOUT_PIN, HX711_SCK_PIN);

    hx.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
    hx.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
    
    for (int i = 0; i < NUM_STORED_READINGS; i++)
        readings[i].t_reading = 0;// time zero = time for this i   was not set yet
    //end falling from chair alert
}

void read_and_store()
{
    // wake up the HX711
    hx.power_up();

    //get current time in millisec
    unsigned long curr_time = millis();

    //get the current weight reading
    double curr_value = hx.get_units(NUM_OF_READINGS);

    // other varients
    // hx.read();// reads the current value
    // hx.read_average(NUM_OF_READINGS);// reads but with average
    // hx.get_value(NUM_OF_READINGS);// read_average()-TARE
    // hx.get_units(NUM_OF_READINGS);// get_value()/SCALE

    // get more accurate
    curr_time = curr_time / 2.0 + millis() / 2.0;

    // put the HX711 to sleep mode, redundant??
    hx.power_down();

    // shift the readings array
    for (int i = NUM_STORED_READINGS - 1; i > 0; i--)
    {
        readings[i].t_reading = readings[i-1].t_reading;
        readings[i].w_reading = readings[i-1].w_reading;
    }

    // store the most recent reading
    readings[0].t_reading = curr_time;
    readings[0].w_reading = curr_value;
}

void resett()
{
   //shut-down if button pressed
  if(buzzer_ringing==LOW)
     {
        Serial.println("button is pressed, buzzer already off, no action");
        return;
     }

   Serial.println("button is pressed, turning off buzzer");
   noTone(buzzer);
   buzzer_ringing = LOW;
   //clearing the array as we are selecting multiple readings,
   //we dont want same reading to induce alarm more than once
   for (int i = 0; i < NUM_STORED_READINGS; i++)
        readings[i].t_reading = 0;// time zero = time for this i   was not recorded yet

}

void loop() 
{ 
  // TEST BLOCK //////////////////////////////////
    delay(70);
    Serial.println("ABBBBBAAA");
    Serial.print("Button is: ");
    Serial.println(digitalRead(button));

    Serial.print("Buzzer is: ");
    Serial.println(buzzer_ringing);

  ///////////////////////////////////////////////
  
  //return as we don't care what happens
  if(buzzer_ringing)
   {
       Serial.println("ringing....");      
       return;
   }
   
  //get current reading
  read_and_store();
 
  Serial.print("Current readings: ");
  for (int i = 0; i < NUM_STORED_READINGS; i++)
      {
         Serial.print(readings[i].w_reading);
         Serial.print(" ");
      }
  Serial.println();
  
  Serial.print("Current TIME readings: ");
  for (int i = 0; i < NUM_STORED_READINGS; i++)
      {
         Serial.print(readings[i].t_reading);
         Serial.print(" ");
      }
  Serial.println();
   
  //exit if there is still weight ?
  /*if(readings[0].w_reading)
      {
        Serial.println("There is still weight --> no fall or kidnap ");
        return;
      }*/
      
  //if no readings yet, theoritcially impossible but meh
  if(readings[0].t_reading==0)
      {
        Serial.println("no readings yet ");
        return;
      }
      
  //Get the maximum "speed" over previous readings
  double speed = 0;
  for(int i =0;i<NUM_STORED_READINGS-1;i++)
  {  
    //break if there is no more VALID readings
    if( readings[i+1].t_reading == 0 )
      break;

    double dw = readings[i].w_reading-readings[i+1].w_reading;
    //Serial.println(dw);
    double dt = (readings[i].t_reading-readings[i+1].t_reading);
    //Serial.println(dt);

    double curr = dw/dt;
    //Serial.print(curr);
    speed = max(speed, abs(curr) );
  }

 speed *= 1000;
 Serial.print("max speed: ");
 Serial.println(speed);

  if(speed < Weight_Threshold)
      {
        Serial.println("sub-threshold change ");
        return;
      }

   Serial.println("DANGER !! RINGING ");
   buzzer_ringing = HIGH;
   tone (buzzer,3500);  

}
