
// PWM + GPIO_PIN Interrupt
#include "pwm.h"
#include "MyAdc.h"
#include <Wire.h>
#include "HDC1000.h"
#include <WiFi.h>
#include <stdarg.h>

#define TIMER_BASE TIMERA2_BASE
#define TIMER TIMER_B

// CREAT PWM FROM TIMERA2 TIMER_B
Timer timerA2PWM(TIMER_BASE,TIMER,INVERSION_YES);
cc3200Adc adcCh3(ADC_CH_3);
HDC1000 tempHumiditySensor;
//HDC1000 mySensor(0x41, 2) <-- DRDYn enabled and connected to Arduino pin 2 (allows for faster measurements).
// Initialize the Wifi client library
WiFiClient client;



// global variable
#define NoOfData 25
#define NoOfAverage 10
#define PWMPeriod 10.0    // milisecond
#define PWMDuty 0.32      // milisecond
#define MOVING_AVEGRAGE_1  20            // moving average of 20 samples
#define MOVING_AVEGRAGE_30  30
#define NoOfDataToEncode  10
#define DC_FAN  5


double P1Threshold = 0.52 ;   // P1 threshold
double  P2Threshold = 0.55 ;   // P2 threshold
double adcVoltAverage;
unsigned long adcTimeStamp,adcTS1,adcTS2, timeDifference; // adc timestamp
unsigned long p1Count, p2Count, p1CountAverage, p2CountAverage;
boolean flagFor1sec = false;  // 1 sec flag
unsigned long tick1Second, tick30Second, tick1Minute, tick1Hour;
double *p1CountArray, *p2CountArray;
double adcVoltMovingAverage1Array[MOVING_AVEGRAGE_1];
unsigned long t1,t2,t3;
double Temperature,Humidity,AdcVoltage;
uint16_t HDC1000Configuration;              //HDC1000 current configuration
unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1 * 30 * 1000L; // delay between updates, in milliseconds
const unsigned long sleepInterval = 15 * 1000L;   // sleep 15 sec until postingInterval
// int *humidityArray , *temperatureArray, *adcVoltageArray;      // dynamic memory for temperature & humidity measurements
int humidityArray[NoOfDataToEncode];
int temperatureArray[NoOfDataToEncode]; 
int adcVoltageArray[NoOfDataToEncode];

void setup()
{
// dynamicMemoryAllocation();
 intialization(); 
 }

void loop()
{
  // put your main code here, to run repeatedly:
  #define HIBERNATE_TIME ((32768)*(5)*(60))      // 4 min ; time based on 32.768 kHz clock
 // #define RESET_INTERVAL_HOUR         6      // software reset every 6 hour
 static  char **dataToPostToGoogle;              // double pointer to encoded data string
 // display adc results every 1 second
   if(flagFor1sec) 
   {
      flagFor1sec = false;   
     displayResults();
    }
  
  // TURN-ON FAN 10 SEC BEFORE TRANSMISSION  
  if (millis() - lastConnectionTime > postingInterval - 10000L) 
  {
    digitalWrite(DC_FAN,1); // turn on fan
    
  }
  // if postingInverval seconds have passed since your last connection
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval)
     {
       //     char **dataToPostToGoogle;
       dataToPostToGoogle = postDataEncoding(temperatureArray,humidityArray,adcVoltageArray);
       httpRequest(dataToPostToGoogle);
       Serial.println("Hibernating ...");delay(100);
       timerA2PWM.softReset(HIBERNATE_TIME);
     }
     // software reset
    // if (tick1Minute > 1)
     //  timerA2PWM.softReset(HIBERNATE_TIME);
}

/***************************************************************************/
/*********************** SUB-ROUTINES **************************************/
/**************************************************************************/
/*
void dynamicMemoryAllocation(){
  // allocate dynamic memory for calculating moving average
//  adcVoltMovingAverage1Array = (double*)calloc(MOVING_AVEGRAGE_1,sizeof(double));
//  p1CountArray = (double*)calloc(MOVING_AVEGRAGE_30,sizeof(double));
//  p2CountArray = (double*)calloc(MOVING_AVEGRAGE_30,sizeof(double));
  temperatureArray = (int*)calloc(NoOfDataToEncode, sizeof(int));
  humidityArray = (int*)calloc(NoOfDataToEncode, sizeof(int));
  adcVoltageArray = (int*)calloc(NoOfDataToEncode, sizeof(int));
}*/

void intialization() {
  setupPWM_Timer();
  Serial.begin(9600);
  adcCh3.begin();
  tempHumiditySensor.begin();
  pinMode(DC_FAN,OUTPUT);  // FAN ON-OFF CONTRON
//  digitalWrite(DC_FAN,1); // turn on fan 
  setupWifi("NetweeN","car391133"); // setup wifi
//  setupWifi("ByoungLoh","car391133"); // setup wifi
//   setupWifi("SungwonGawon2_5G","car391133"); // setup wifi
}
 
void storeDataForTransmission(int *array, int data)
{
  // shift all data in the array to the right     
  for ( char k=0; k <NoOfDataToEncode-1; k++)
      *(array+k) = *(array+k+1);
  // store a new data to array    
      *(array + NoOfDataToEncode-1) = data;
}


// Encode int only for posting to Google sheet
static char** postDataEncoding(int* temperature, int* humidity, int *adcVoltage ) 
{
  #define BUFFER_SIZE  80  
 static char *bufferEncoding[NoOfDataToEncode]; 
 char batteryStatus = (HDC1000Configuration & 0x0800) >> 11 ;
 for(char k=0;k<NoOfDataToEncode; k++)
  {
    bufferEncoding[k]     = (char*)calloc(BUFFER_SIZE, sizeof(char));
    sprintf( bufferEncoding[k], "id=rm01&time=2016&temperature=%d&humidity=%d&PM2_5=%d&BatteryStatus=%d", *(temperature+k),*(humidity+k),*(adcVoltage +k),batteryStatus);
  }
  return  bufferEncoding;
}


void displayResults() {
  Serial.print("count: ");                  Serial.print(tick1Second);
  Serial.print("\t adc volt(v): ");          Serial.print(adcVoltAverage);
  Serial.print("\t TIME(min):  ");           Serial.print(tick1Minute);
  Serial.print("\t t3(isr loop time, ms) :  ");           Serial.print(t3);
 // Serial.print("\t p1Count:  ");      Serial.print(p1Count);
 // Serial.print("\t p2Count:  ");      Serial.print(p2Count);
//  Serial.print("\t p1Avg:  ");              Serial.print(p1CountAverage);
//  Serial.print("\t p2Avg:  ");              Serial.print(p2CountAverage);
  Serial.print("\t Temperature: ");         Serial.print(Temperature); 
  Serial.print("\t Humidity: ");            Serial.print(Humidity);Serial.println("%");
}

void setupPWM_Timer()
{
  timerA2PWM.pinMuxAndPeripheralClockConfigure(PRCM_TIMERA2, PIN_64, PIN_MODE_3);
  timerA2PWM.configure( TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PWM ); // half width, PWM
  timerA2PWM.setInterrupt(TIMER_CAPB_EVENT, TIMER_EVENT_NEG_EDGE, pwmISR);
  timerA2PWM.setPeriod(PWMPeriod);   //    period in milisecond
  timerA2PWM.setDuty(PWMDuty);     //     duty in milisecond
  timerA2PWM.enable();         //     enable timer
}

// isr on pwm negative-edge
void pwmISR()
{
  static int isr_cnt,k, data;
  unsigned long ulstatus;
  double adcVoltage;  // adc voltage
  double adcVoltArray[NoOfData];
  
  t1 = millis();  //pwmISR loop-time check
  adcTS1 = adcCh3.getTimeStamp(); // get adc start time
 
  for (char i=0; i<NoOfData; i++)
  {
    adcVoltArray[i]  =  adcCh3.getAdcValue();
  }  
  // ADC time check
  adcTS2 = adcCh3.getTimeStamp();  // final time for mulitiple times of ADC
  timeDifference = adcTS2-adcTS1;    // elapsed time
  
  // clear interrupt flag
  ulstatus = timerA2PWM.intStatus(TIMER_BASE);
  timerA2PWM.intClear(TIMER_BASE, ulstatus);
  
// calcuate moving-avegrage of volt_array[20]
 adcVoltAverage =  movingAverage(adcVoltMovingAverage1Array, adcVoltArray[20], MOVING_AVEGRAGE_1);

  // P1 and P2 count
/*  if (adcVoltAverage > P1Threshold)
      p1Count++;
  if (adcVoltAverage > P2Threshold)
      p2Count++; */
  
  // 1 second flag
  isr_cnt ++; 
if(isr_cnt == (int)(1000 / (PWMPeriod))) {
  flagFor1sec = true;
  isr_cnt = 0;
 // p1CountAverage = (int)movingAverage(p1CountArray, (double)p1Count, MOVING_AVEGRAGE_30);
 // p2CountAverage = (int)movingAverage(p1CountArray, (double)p2Count, MOVING_AVEGRAGE_30);
  
  // get data
  Temperature = tempHumiditySensor.getTemp(); // get temperature
  Humidity = tempHumiditySensor.getHumi();  // get humidity
  AdcVoltage = adcVoltAverage; // get ADC voltage
  HDC1000Configuration = tempHumiditySensor.readConfig();
  
  // store data for transmission ( temp. and humidity is multiplied by 10)
  storeDataForTransmission(temperatureArray,(int)Temperature*10);
  storeDataForTransmission(humidityArray,(int)Humidity*10);
  storeDataForTransmission(adcVoltageArray,(int)(AdcVoltage*100));
  
//  p1Count=0;                                              // reset p1 count
//  p2Count=0;
  tick1Second++;                                      // increment 1 sec tick
  (tick1Second%60) ?  :tick1Minute++ ;   // increment 1 min tick
   (tick1Minute%60) ?   : tick1Hour++ ;   // increment 1 min tick
  }
// isr loop time check
  t2 = millis();
  t3=t2-t1;
}

void pinISR()
{
//  static char data = 0;
//  int val;
//  data ^=1;  // toggle output
//  digitalWrite(8,data); // pulse output
//  val = analogRead(A3); // adc  
// if (isr_cnt == 500)
//  digitalWrite(5,0); 
}

// moving average of 10 samples
// moving average with pointer
double movingAverage(double* arrayPtr, double stream, const int movingAverageNum)
{
  static double voltSum;
  double voltMovingAvg;
  char k,m;

  for (  k=0; k <movingAverageNum-1; k++)
  *(arrayPtr+k) = *(arrayPtr+k+1);
  *(arrayPtr + movingAverageNum-1) = stream;

  // calculate moving average
  for ( m=0; m<movingAverageNum; m++)
  voltSum += *(arrayPtr + m);
  voltMovingAvg = voltSum /movingAverageNum ;
  voltSum =0;  // reset dummy2*/

  return voltMovingAvg;
}

/************ WIFI CONNECTION ***************/
// this method makes a HTTP connection to the server:
void httpRequest(char **dataStringArray) {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  // server address:
char server[] = "script.google.com";
String data; // data to post

  // if there's a successful connection:
  for (char k=0; k<NoOfDataToEncode; k++)
   {
      client.stop();
    if (client.sslConnect(server, 443)) {
    Serial.print("POSTING DATA TO GOOGLE SHEET...");
    
      data = dataStringArray[k];
      if (k < NoOfDataToEncode -1)  // test for 
      {
        // send the HTTP POST request:
        client.println("POST /macros/s/AKfycbwZG8yJLEXqOXkpyPQygB6CFu5T5LkQKVFJOCUxNg3ZI8suQ20/exec HTTP/1.1");
        client.println("Host: script.google.com");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(data.length());
        client.println();  // required but i don't know exactly why?
        client.print(data);
        client.println();
        client.println("Connection: close");
          }
   
      Serial.print("posted data: ");Serial.println(data);
      delay(50);
      // FREE DYNAMIC MEMORY  
      free(dataStringArray[k]);
    }
    else 
    {
      // if you couldn't make a connection:
      Serial.println("connection failed");
    }
    sleep(1000); // delay for posting data to GOOGLE
}
    
    // TURN OFF FAN
    digitalWrite(DC_FAN,0);
    
    // GO INTO LOW POWER MODE
  //  sleep(postingInterval - sleepInterval);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
}


void printWifiStatus() 
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setupWifi(char* ssid, char *password)
{
  // your network name also called SSID
//char *ssid = "NetweeN";
// your network password
//char *password = "car391133";
 // attempt to connect to Wifi network:
 const int delayTime_ms = 500;
 const char connectionTryMaxCount = 5;
 static int connectionTrialCount;
    Serial.print("Attempting to connect to Network named: ");
    // print the network name (SSID);
    Serial.println(ssid); 
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, password);
    while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
      Serial.print("re-connection tried :");
      Serial.println(connectionTrialCount);
      delay(delayTime_ms);
      connectionTrialCount++;
      if(connectionTrialCount > connectionTryMaxCount)
      {
        timerA2PWM.softReset(10000L);
        connectionTrialCount = 0;
      }
      
    }
  
    Serial.println("\nYou're connected to the network");
    Serial.println("Waiting for an ip address");
  
    while (WiFi.localIP() == INADDR_NONE) {
    // print dots while we wait for an ip addresss
      Serial.print(".");
      delay(300);
      }

    Serial.println("\nIP Address obtained");
    // We are connected and have an IP address.
    // Print the WiFi status.
    printWifiStatus();
}

/*****************************************************************/
/******************  DEPRECATED **********************************/
/****************************************************************/

double movingAverage10(double stream)
{
  const int arrayIndex = 10;
  static unsigned long cnt, i;
  static double dummy[arrayIndex];
  static double dummy2;
  double volt_avg;
  cnt++;
  for ( char k=0; k <arrayIndex-1; k++)
  dummy[k] = dummy[k+1];
  dummy[arrayIndex-1] = stream; 
  
  // calculate average
  for (char m=0; m<arrayIndex; m++)
  dummy2 += dummy[m];
  volt_avg = dummy2 /arrayIndex ;
  dummy2 =0;  // reset dummy2
  return volt_avg;
}

// moving average of 30 seconds
unsigned long movingAverageOfP1CountFor30Sec(unsigned long stream)
{
  const int arrayIndex = 30;
  static unsigned long cnt, i;
  static unsigned long dummy[arrayIndex];
  static unsigned long dummy2;
  unsigned long pCount_avg;
  cnt++;
  for ( char k=0; k <arrayIndex-1; k++)
  dummy[k] = dummy[k+1];
  dummy[arrayIndex-1] = stream; 
  
  // calculate average
  for (char m=0; m<arrayIndex; m++)
  dummy2 += dummy[m];
  pCount_avg = dummy2 /arrayIndex ;
  dummy2 =0;  // reset dummy2
  return pCount_avg;
}

unsigned long movingAverageOfP2CountFor30Sec(unsigned long stream)
{
  const int arrayIndex = 30;
  static unsigned long cnt;
  static unsigned long dummy[arrayIndex];
  static unsigned long dummy2;
  unsigned long pCount_avg;
  cnt++;
  for ( char k=0; k <arrayIndex-1; k++)
  dummy[k] = dummy[k+1];
  dummy[arrayIndex-1] = stream; 
  
  // calculate average
  for (char m=0; m<arrayIndex; m++)
  dummy2 += dummy[m];
  pCount_avg = dummy2 /arrayIndex ;
  dummy2 =0;  // reset dummy2
  return pCount_avg;
}


