// --------------------------------------
// Includes
// --------------------------------------
#include <string.h>
#include <stdio.h>
#include <Wire.h>

// --------------------------------------
// Global Constants
// --------------------------------------

//Number of Secondary Cycles within one Main Cycle
#define NUM_SC 6
//In micro seconds
#define SC_TIME 200
//Comunication server constants
#define PORT 9000
#define MESSAGE_SIZE 8
#define SLAVE_ADDR 0x8
//Messages
#define ERROR_MSG "MSG: ERR\n"
#define OK_MSG "%s  OK\n>"
#define UP_SLP_MSG "SLP:  UP\n"
#define FLAT_SLP_MSG "SLP:FLAT\n"
#define DOWN_SLP_MSG "SLP:DOWN\n>"
#define SPD_MSG "SPD:%s\n"
#define LIT_MSG "LIT:%s%%\n"
#define SET 1
#define CLR 0
//Pin numbers
#define GAS_PIN_OUT 13
#define BRK_PIN_OUT 12
#define MIX_PIN_OUT 11
#define SPD_PIN_OUT 10
#define LAM_PIN_OUT 7
#define SLP_PIN_IN_1 9
#define SLP_PIN_IN_2 8
#define LIT_PIN_IN A0

//Units m/s^2
#define GAS_POWER 0.5
#define BRAKE_POWER 0.5
#define INERTIA 0.25
//Units: m/s
#define MAX_SPEED 70
#define MIN_SPEED 40
//Units: %
#define MAX_LIT 99
#define MIN_LIT 0

// --------------------------------------
// Global Variables
// --------------------------------------

//0 --> off
//1 --> on
int mixer_status = 0;
int gas_status = 0;
int brake_status = 0;
int lamp_status  = 0;
// Admissible values [0%, 99%]
int luminosity = 0;
// Admissible values = -1(downwards), 0(flat), 1(upwards)
int slope = 0;
//cannot be negative
float speed = 0.0;
//Auxiliary variables for the messaging service
bool request_received = false;
bool answer_requested = false;
char request [MESSAGE_SIZE + 1];
char response [MESSAGE_SIZE + 1];
int count = 0;//Aux variable to count the number of characters red from input
int sc = 0;//Counter for the secondary cycles elapsed since the beginning of the execution
unsigned long lastCall = 0;//Auxiliary timer variable

void setup()  
{
  //Set up all the pins
  pinMode(GAS_PIN_OUT, OUTPUT);
  pinMode(BRK_PIN_OUT, OUTPUT);
  pinMode(MIX_PIN_OUT, OUTPUT);
  pinMode(SPD_PIN_OUT, OUTPUT);
  pinMode(SLP_PIN_IN_1, INPUT);
  pinMode(SLP_PIN_IN_2, INPUT);
    
  // Initialize I2C communications as a worker
  Wire.begin(SLAVE_ADDR);
  
  // Function to run when data requested from master
  Wire.onRequest(requestEvent);
  
  // Function to run when data received from master
  Wire.onReceive(receiveEvent);
  
  Serial.begin(PORT);
  
}

  

void loop()
{
  unsigned long start;
  unsigned long end;
  switch(sc%NUM_SC){
   
    case 0:    
    start = micros();
    
    slope_request();
    
    end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;

    case 1:
    start = micros();
    gas_request();
    speed_request();
    brake_request();
    
    end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;
    
    case 2:
    start = micros();
    mixer_request();
    
    end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;

    case 3:
    start = micros();
    slope_request();
    
    end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;

    case 4:
    start = micros();
    gas_request();    
    speed_request();
    brake_request();
    mixer_request();    
    
    end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;
    
    case 5:
    //start = micros();
    light_request();    
    lamp_request();
   
    //end = micros();
    //delayMicroseconds(SC_TIME-(end-start));
    break;
  } 
  sc = sc + 1;
}

// --------------------------------
// AUXILIARY FUNCTIONS
// --------------------------------



// --------------------------------------
// Handler function: receiveEvent
// --------------------------------------
void receiveEvent(int num)
{
   char aux_str[MESSAGE_SIZE+1];
   int i=0;

   // read message char by char, up to MESSAGE_SIZE
   for (int j=0; j<num; j++) {
      char c = Wire.read();
      if (i < MESSAGE_SIZE) {
         aux_str[i] = c;
         i++;
      }
   }
   aux_str[i]='\0';

   // if message is correct, load it
   if ((num == MESSAGE_SIZE) && (!request_received)) {
      memcpy(request, aux_str, MESSAGE_SIZE+1);
      request_received = true;
   }
}

// --------------------------------------
// Handler function: requestEvent
// --------------------------------------
void requestEvent()
{
   // if there is an answer send it, else error
   if (answer_requested) {
      Wire.write(response,MESSAGE_SIZE);
      memset(response,'\0', MESSAGE_SIZE+1);
   } else {
      Wire.write("NOT REDY",MESSAGE_SIZE);
   }
  
   // set answer empty
   request_received = false;
   answer_requested = false;
   memset(request,'\0', MESSAGE_SIZE+1);
   memset(response,'0', MESSAGE_SIZE);
}

// ------------------------------------
// Resets the buffers in order to 
//   continue with the communication
// ------------------------------------
int resetBuffer(){
   // set buffers and flags
      memset(request,'\0', MESSAGE_SIZE+1);
      request_received = false;
      answer_requested = true;  
}

int slope_request(){
   
//update the slope value
   int errno = readSlope();
  //Process a SLOPE request
  if((request_received) &&
        (0 == strcmp("SLP: REQ",request)) ){    
 
    switch(errno){
      case -1:
         sprintf(response, UP_SLP_MSG);       
         break;
      case 0:
         sprintf(response, FLAT_SLP_MSG);
         break;
      case 1:
         sprintf(response, DOWN_SLP_MSG);
         break;
       default:
        sprintf(response, ERROR_MSG);  
    }
          
    resetBuffer();
    return 0;
  }
}

int speed_request(){
   int errno = computeSpeed();
    //Process a SPEED request
  if ((request_received) &&
        (0 == strcmp("SPD: REQ",request)) ){        
    char num_str[5];
      dtostrf(speed,4,1,num_str);      
   sprintf(response, SPD_MSG, num_str);
   resetBuffer();  
  }
  
  return 0;
}


int gas_request(){
   int errno = 0;
   if (request_received && 0 == strcmp("GAS: SET",request)){	      
     errno = switchGas(SET);
     //Process a GAS request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "GAS");
      }      
      resetBuffer();      
   }else if (request_received && (0 == strcmp("GAS: CLR",request))){
     errno = switchGas(CLR);
     //Process a GAS request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "GAS");
      }
     resetBuffer();      
   }
   return 0;  
}

int brake_request(){   
  int errno = 0;
   if (request_received && 0 == strcmp("BRK: SET",request)){

      errno = switchBrake(SET);
     //Process a BRK request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "BRK");
      }
      resetBuffer();
      
   }else if (request_received && 0 == strcmp("BRK: CLR",request)){
   	errno = switchBrake(CLR);
     //Process a BRK request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "BRK");
      }
      resetBuffer();
   }
   return 0;  
}

int mixer_request(){
	int errno = 0;
   if (request_received && 0 == strcmp("MIX: SET",request)){
	errno = switchMixer(SET);
      //Process a MIX request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "MIX");
      }
      resetBuffer();      
      
   }else if(request_received && 0 == strcmp("MIX: CLR",request)){
   	errno = switchMixer(CLR);
     //Process a MIX request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "MIX");
      }
      resetBuffer();      
     
   }
   return 0;
}

int light_request(){	   
  if ( (request_received) &&
        (0 == strcmp("LIT: REQ",request)) ) {  
    
    	int input = analogRead(LIT_PIN_IN);
  		luminosity = map(input, 0, 1023, MIN_LIT, MAX_LIT);
     char cstr[5];
	 itoa(luminosity, cstr, 10);            
      // send the answer for slope request
      sprintf(response,LIT_MSG,cstr);

      // set buffers and flags
      memset(request,'\0', MESSAGE_SIZE+1);
      request_received = false;
      answer_requested = true;
   }
  
  return 0;
}

int lamp_request(){
	int errno = 0;
   if (request_received && 0 == strcmp("LAM: SET",request)){
	errno = switchLamp(SET);
      //Process a LAM request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "LAM");
      }
      resetBuffer();      
      
   }else if(request_received && 0 == strcmp("LAM: CLR",request)){
   	errno = switchLamp(CLR);
     //Process a LAM request
      if (errno == 0){
         sprintf(response, ERROR_MSG);
      }else{
         sprintf(response, OK_MSG, "LAM");
      }
      resetBuffer();      
     
   }
   return 0;

}


// --------------------------------------
// Function: computeSpeed
//	Definition: Computes the current speed, always 0 <= speed 
//	Returns: 1 if successful. 0 otherwise
// --------------------------------------
int computeSpeed(){
  float acceleration = slope*INERTIA + gas_status*GAS_POWER +
    brake_status*BRAKE_POWER;
  //speed = speed_0 + acceleration*time; 
  
    
  speed += acceleration/10;  
  //Map input value [40, 70] into output range [0, 255]  
  if(speed>70){
    digitalWrite(SPD_PIN_OUT, HIGH);
  }else if(speed<40){
    digitalWrite(SPD_PIN_OUT, LOW);
  }else{
    int out = map(speed, 40, 70, 0, 255);
  	analogWrite(SPD_PIN_OUT, out);
  }
  
  return 1;

}


// --------------------------------------
// Function: readSlope
//	Definition: Read the slope sensor to obtain the current slope value
//	Returns: The value of the slope. 1 upwards, 0 flat, -1 downwards
// --------------------------------------
int readSlope(){
	/*
  	pin1 	pin2	SLP
  	0 		0 		-1
    1 		0 		0
    0 		1 		0
    1 		1 		1
  */
	int base_value = -1;
	int pin1 = digitalRead(SLP_PIN_IN_1);
	int pin2 = digitalRead(SLP_PIN_IN_2);  	
  	slope = base_value + pin1 + pin2;    	
  	return slope;
}

int readLight(){
	
  	long input = analogRead(LIT_PIN_IN);
  	luminosity = map(input, 0, 1023, MIN_LIT, MAX_LIT);  	    	
  	return 1;

}


// --------------------------------------
// Function: switchMixer
//	Definition: Turns on/off the mixer based on the request
//	Returns: 1 if the operation was sucessful. 0 otherwise
// --------------------------------------

int switchMixer(int status){
  int errno = 1;
 
  switch(status){
    case SET:    
      mixer_status = 1;     
      break;
    
    case CLR:    
      mixer_status = 0;
      break;
    default:
      //The status value is invalid
      errno = 0;
  }
	//Display the new status of the mixer
  digitalWrite(MIX_PIN_OUT, mixer_status);
  return errno;
}
// --------------------------------------
// Function: switchAccelerator
//	Definition: Turns on/off the accelerator based on the request
//	Returns: 1 if the operation was sucessful. 0 otherwise
// --------------------------------------
int switchGas(int status){
  int errno = 1;
  
  switch(status){
    case SET:    	
      gas_status = 1;     
      break;
    
    case CLR:   
      gas_status = 0;
      break;
    default:
      //The status value is invalid
      errno = 0;
  }
	//Display the new status of the accelerator
  digitalWrite(GAS_PIN_OUT, gas_status);
  return errno;
}
// --------------------------------------
// Function: switchBrake
//	Definition: Turns on/off the brake based on the request
//	Returns: 1 if the operation was sucessful. 0 otherwise
// --------------------------------------
int switchBrake(int status){

  int errno = 1;
  switch(status){
    case SET:
      brake_status = 1;
      break;
    case CLR:
      brake_status = 0;    
      break;
    default:
      //The status value is invalid
      errno = 0;
  }
	//Display the new status of the accelerator
  digitalWrite(BRK_PIN_OUT, brake_status);
  return errno;
}

int switchLamp(int status){
	 int errno = 1;
  switch(status){
    case SET:
      lamp_status = 1;
    digitalWrite(LAM_PIN_OUT, HIGH);
      break;
    case CLR:
      lamp_status = 0;    
    digitalWrite(LAM_PIN_OUT, LOW);
      break;
    default:
      //The status value is invalid
      errno = 0;
  }
	//Display the new status of the accelerator
  //digitalWrite(LAM_PIN_OUT, HIGH);
  return errno;
}
