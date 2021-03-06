/*   
NODE MCU Web server with two Sensors over I2C and aREST API
BMP180 altitude and temo
ClosedCube_HDC1080 humidity and temp
https://www.sparkfun.com/products/11824
Like most pressure sensors, the BMP180 measures absolute pressure.
Since absolute pressure varies with altitude, you can use the pressure
to determine your altitude.

Hardware connections:
- (GND) to GND
+ (VDD) to 3.3V
(WARNING: do not connect + to 5V or the sensor will be damaged!)

You will also need to connect the I2C pins (SCL and SDA) to your
Arduino. The pins are different on different Arduinos:

Any Arduino pins labeled:  SDA  SCL
Uno, Redboard, Pro:        A4   A5
Mega2560, Due:             20   21
Leonardo:                   2    3
NodeMCU V1(ESP12E ):       D2   D1

Leave the IO (VDDIO) pin unconnected. This pin is for connecting
the BMP180 to systems with lower logic levels such as 1.8V


*/

// Your sketch must #include this library, and the Wire library.
// Install them! 
#include <SFE_BMP180.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "ESP8266WiFi.h"
#include <aREST.h>

// WiFi parameters
const char* ssid = "ARRIS-3F62";
const char* password = "BP28CD304627";
const char* id = "2";
const char* Name = "sensor_module2";  


// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// You will need to create an SFE_BMP180 object, here called "pressure":

SFE_BMP180 bmp180;
ClosedCube_HDC1080 hdc1080;

// Create aREST instance
aREST rest = aREST();

//Variables para BPM180
double baseline; // baseline pressure
double T;

//Variables de control y tiempo
int ledState = LOW;     
unsigned long previousMillis = 0;
const long interval = 500;

//variables Hdc1080
double temp;
double rh;


// Variables to be exposed to the API
int temperature;
int humidity;

void setup()
{
  //LED
  pinMode(LED_BUILTIN, OUTPUT);

  //Serial
  Serial.begin(115200);
  Serial.println("REBOOT");

  // Initialize the Pressure sensor (it is important to get calibration values stored on the device).
  if (bmp180.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail (disconnected?)\n\n");
    while(1); // Pause forever.
  }

  // Get the baseline pressure: 
  baseline = getPressure();
  Serial.print("baseline pressure: ");
  Serial.print(baseline);
  Serial.println(" mb");  

// Start hdc1080 sensor
  hdc1080.begin(0x40);
//Read info 
  Serial.print("Manufacturer ID=0x");
  Serial.println(hdc1080.readManufacturerId(), HEX); // 0x5449 ID of Texas Instruments
  Serial.print("Device ID=0x");
  Serial.println(hdc1080.readDeviceId(), HEX); // 0x1050 ID of the device


  //REST API
  // Init variables and expose them to REST API
  rest.variable("temperature",&temperature);
  rest.variable("humidity",&humidity);
    
  // Give name and ID to device
  rest.set_id(id);
  rest.set_name(Name);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
  
  // Print the IP address
 Serial.println(WiFi.localIP());
  
}



void loop()
{
  //Blink Led to show it's working in the loop
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;   
    if (ledState == LOW)
      ledState = HIGH;  // Note that this switches the LED *off*
    else
      ledState = LOW;   // Note that this switches the LED *on*
    digitalWrite(LED_BUILTIN, ledState);
 }

// Data from bmp180:
 //Get Pressure    and temp
  double a,P;
  // Get a new pressure reading 
  P = getPressure();
  // Calc the relative altitude difference between
  // the new reading and the baseline reading:
  a = bmp180.altitude(P,baseline);

  //Print 
//  Serial.print("relative altitude: ");
//  if (a >= 0.0) Serial.print(" "); // add a space for positive numbers
//  Serial.print(a,1);
//  Serial.print(" meters, ");
//  if (a >= 0.0) Serial.print(" "); // add a space for positive numbers
//  Serial.print(a*3.28084,0);
//    Serial.print(" feet");
//    Serial.print(" Temp: ");
//    Serial.print(T);
//    Serial.println(" degree ");


// Get temp and humidity from 1080
  temp = hdc1080.readTemperature();
  rh = hdc1080.readHumidity();

//Print
//  Serial.print("T=");
//  Serial.print(temp);
//  Serial.print("C, RH=");
//  Serial.print(rh);
//  Serial.println("%");


 // Pass readings to API 
  temperature =  temp;
  humidity =  rh;

  
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);




}


double getPressure()
{
  char status;
  double P,p0,a;

  // You must first get a temperature measurement to perform a pressure reading. 
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = bmp180.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:

    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.

    status = bmp180.getTemperature(T);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = bmp180.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = bmp180.getPressure(P,T);
        if (status != 0)
        {
          return(P);
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");
}



