//This is a Function Demonstration Program for SensorDuinoV2.0 
//Board and Program Designed and Tested by Zhengyao William Wang
//2024/11/01 Version 1.02

#include "SensorDuinoV2.h"
#include "alert.h" // Out of range alert icon
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>

////////////WiFi Config////////////

const char* serverURL = "http://192.168.116.250:8080/sensor-data";

typedef struct struct_message { // Message Container
  float temperature;
  float humidity;
  float pressure;
  float gas;
  float CH2O;
} struct_message;

struct_message SensorDuino_readings;

BH1750 On_Board_BH1750;
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

uint32_t runTime = -99999;       // time for next update
int reading = 0; // Value to be displayed
int d = 0; // Variable used for the sinewave test waveform
bool range_error = 0;
int8_t ramp = 1;

void setup() {

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, SensorDuino_TXD, SensorDuino_RXD);

  Wire.begin();
  On_Board_BH1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_addr);  

  pinMode(MQ2_Switch_Pin, OUTPUT);
  pinMode(ZE08_Switch_Pin, OUTPUT);

  pinMode(MQ2_Read_Pin,INPUT);
  pinMode(Battery_Monitor_Pin,INPUT);

  pinMode(LED_Pin, OUTPUT);
  pinMode(Buzzer_Pin, OUTPUT);

  ////Screen Initialization////
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(0xCCFF33);tft.setTextSize(1);tft.setTextFont(7);
         
  ////Connect to the WiFi////
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.print("Connecting to ");              
  Serial.print(ssid); Serial.println(" ...");  
  
  int i = 0;                                   
  while (WiFi.status() != WL_CONNECTED) {      
    delay(1000);                                                
    Serial.print(i++); Serial.print(' ');      
  }                                            
                                               
  Serial.println("");                          
  Serial.println("Connection established!");   
  Serial.print("IP address:    ");             
  Serial.println(WiFi.localIP());   
}

void loop() {

  digitalWrite(ZE08_Switch_Pin, HIGH); //turn on MQ2 and ZE08 all the time for testing, will not use in normal mode cauz it consumes too much power
  digitalWrite(MQ2_Switch_Pin, HIGH);
  
  delay(200);
  Serial.println("/////////////////////Data Received!/////////////////////");
////////////////////////////////////////////////////////////////////////////////////////
  // Part 1, obtain temperature and humidity data from SHT30
  // Due to received values are in String datatype, datatype conversion required
  float SHT30_ReadTempHumidity_RECEIVED[3];
  String SHT30_ReadTempHumidity_RAW = SHT30_ReadTempHumidity(SHT30_addr);
  byte index = 0;

  char data[SHT30_ReadTempHumidity_RAW.length()];
  SHT30_ReadTempHumidity_RAW.toCharArray(data, SHT30_ReadTempHumidity_RAW.length());

  char* SHT30_ELEMENT = strtok(data,",");
  while (SHT30_ELEMENT != NULL)
  {
    SHT30_ReadTempHumidity_RECEIVED[index] = atof(SHT30_ELEMENT);
    index++;
    SHT30_ELEMENT = strtok(NULL,",");
  }

  double SHT30_Temp_C = SHT30_ReadTempHumidity_RECEIVED[0];
  double SHT30_Temp_F = SHT30_ReadTempHumidity_RECEIVED[1];
  double SHT30_Humidity_Level = SHT30_ReadTempHumidity_RECEIVED[2];

  delay(100);
////////////////////////////////////////////////////////////////////////////////////////
  //Part 2, obtain luminosity reading from BH1750
  double BH1750_Luminosity_lux = On_Board_BH1750.readLightLevel();
  
  delay(100);
////////////////////////////////////////////////////////////////////////////////////////
  //Part 3, obtain pressure reading in kPa from HP203N
  double HP203N_Pressure_kPa = HP203N_ReadPressure(HP203N_addr,4096);
  
  delay(100);
////////////////////////////////////////////////////////////////////////////////////////
  //Part 4, obtain smoke and combustile gas reading from MQ-2B in percentage
  double MQ2_Gas_Level = MQ2_ReadGas(MQ2_Switch_Pin,MQ2_Read_Pin)/4096*100;
  
  delay(100);
////////////////////////////////////////////////////////////////////////////////////////
  //Part 5, obtain formaldehyde (CH2O) reading from ZE08 in mg/m3
  double ZE08_CH2O_mgm3 = ZE08_ReadFormaldehyde(ZE08_Switch_Pin);
  
  delay(100);
////////////////////////////////////////////////////////////////////////////////////////
  //Part 6, Activate the alarms if abnormalty detected

  if (SHT30_Temp_C > 40 || MQ2_Gas_Level > 40)
  {
    LED_Dimming(LED_Pin, 4000, 0, 8, 1); //PWM freq = 4000, duty cycle = 1/255*100%
    Buzzer_Dimming(Buzzer_Pin, 250, 1, 8, 50); //PWM freq = 300, duty cycle = 50/255*100%
  }
  else if (SHT30_Humidity_Level < 50 || ZE08_CH2O_mgm3 > 1)
  {
    LED_Dimming(LED_Pin, 4000, 0, 8, 1); //PWM freq = 4000, duty cycle = 1/255*100%
    Buzzer_Dimming(Buzzer_Pin, 250, 1, 8, 0); //PWM freq = 300, duty cycle = 50/255*100%
  }
  else 
  {
    LED_Dimming(LED_Pin, 4000, 0, 8, 0); //PWM freq = 4000, duty cycle = 1/255*100%
    Buzzer_Dimming(Buzzer_Pin, 300, 1, 8, 0); //PWM freq = 300, duty cycle = 20/255*100%
  }

////////////////////////////////////////////////////////////////////////////////////////
  //Print readings to Serial Port
  Serial.print("\nAmbient Light Strength = ");Serial.print(BH1750_Luminosity_lux);Serial.println(" lux");delay(10);
  Serial.print("\nTemperature Celsius    = ");Serial.print(SHT30_Temp_C);Serial.println(" C");delay(10);
  Serial.print("\nTemperature Fahrenheit = ");Serial.print(SHT30_Temp_F);Serial.println(" F");delay(10);
  Serial.print("\nRelative Humidity      = ");Serial.print(SHT30_Humidity_Level);Serial.println(" %");delay(10);
  Serial.print("\nAtmospheric Pressure   = ");Serial.print(HP203N_Pressure_kPa);Serial.println(" kPa");delay(10);
  Serial.print("\nSmoke   = ");Serial.print(MQ2_Gas_Level);Serial.println(" %");delay(10);
  Serial.print("\nCH2O   = ");Serial.print(ZE08_CH2O_mgm3,6);Serial.println("mg/m3");delay(10);
  Serial.println();

  //Print readings to LCD Screen
  tft.setTextFont(4);
  
  tft.setTextColor(TFT_WHITE,TFT_BLACK);

  tft.setCursor(0,80);
  tft.print(String(BH1750_Luminosity_lux)+"LUX");

  tft.setCursor(0,130);
  tft.print(String(SHT30_Temp_C)+"C");

  tft.setCursor(0,180);
  tft.print(String(SHT30_Temp_F)+"F");

  tft.setCursor(0,230);
  tft.print(String(SHT30_Humidity_Level)+"%");

  tft.setCursor(0,280);
  tft.print(String(HP203N_Pressure_kPa)+"kPa");

  tft.setCursor(0,380);
  tft.print(String(ZE08_CH2O_mgm3,6)+"mg/m3");

  tft.setCursor(0,330);
  if (MQ2_Gas_Level>40)
    tft.setTextColor(TFT_WHITE,TFT_RED);
  tft.print(String(MQ2_Gas_Level)+"%GAS");

  ringMeter(SHT30_Temp_C,-20,50,       100,70,50,"Celsius",BLUE2RED); // Draw analogue meter
  ringMeter(SHT30_Temp_F,1,122,      200,70,50,"Fahrenheit",BLUE2RED); // Draw analogue meter
  ringMeter(SHT30_Humidity_Level,1,100,      100,200,50,"Humidity",RED2RED); // Draw analogue meter
  ringMeter(HP203N_Pressure_kPa,98,102,     200,200,50,"Pressure",RED2GREEN); // Draw analogue meter

  // Send readings to Laptop via HTTP Post
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"temperature\":" + String(SHT30_Temp_C) + ",";
    jsonData += "\"humidity\":" + String(SHT30_Humidity_Level) + ",";
    jsonData += "\"pressure\":" + String(HP203N_Pressure_kPa) + ",";
    jsonData += "\"light\":" + String(BH1750_Luminosity_lux) + ",";
    jsonData += "\"gas\":" + String(MQ2_Gas_Level) + ",";
    jsonData += "\"CH2O\":" + String(ZE08_CH2O_mgm3);
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error occurred while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(3000);
}

// Include all other necessary functions like HP203N_ReadPressure, SHT30_ReadTempHumidity, MQ2_ReadGas, etc.


float HP203N_ReadPressure(int HP203N_ADDR,int HP203N_SAMPLE_RATE)
{
  Wire.beginTransmission(HP203N_ADDR);

  /*Sample rate: 0x00:4096, 0x01:2048, 0x08:1024, 0x03:512*/
  int HP203N_SAMPLE_RATE_HEX;
  switch(HP203N_SAMPLE_RATE)
  {
    case 4096:
      HP203N_SAMPLE_RATE_HEX = 0x00;
    break;

    case 2048:
      HP203N_SAMPLE_RATE_HEX = 0x01;
    break;

    case 1024:
      HP203N_SAMPLE_RATE_HEX = 0x08;
    break;

    case 512:
      HP203N_SAMPLE_RATE_HEX = 0x03;
    break;
  }

  Wire.write(0x40|HP203N_SAMPLE_RATE_HEX);
  Wire.endTransmission(HP203N_ADDR);

  delay(100);

  Wire.beginTransmission(HP203N_ADDR);
  Wire.write(0x30);//read both temp & pres, 0x30 for pres only, 0x32 for temp only
  Wire.endTransmission(HP203N_ADDR);

  Wire.requestFrom(HP203N_ADDR,6);//for only pres or only temp, use 3 bytes

  float HP203N_PRES;
  if(Wire.available())
  {
    long BYTE_READ;
    BYTE_READ = Wire.read();
    BYTE_READ <<= 8;
    BYTE_READ |= Wire.read();
    BYTE_READ <<= 8;
    BYTE_READ |= Wire.read();
    
    HP203N_PRES = float(BYTE_READ)/1000.00;
    
  }
  return HP203N_PRES;
}

String SHT30_ReadTempHumidity(int SHT30_ADDR)
{
  unsigned int data[6]; 

  Wire.beginTransmission(SHT30_ADDR);
  //Measurement command 0x2C26
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();

  delay(50);
  

  Wire.requestFrom(SHT30_ADDR, 6);

  if (Wire.available() == 6)
  {
    data[0] = Wire.read(); //temp high byte
    data[1] = Wire.read(); //temp low byte
    data[2] = Wire.read(); //temp crc
    data[3] = Wire.read(); //humidity high byte
    data[4] = Wire.read(); //humidity low byte
    data[5] = Wire.read(); //humidity crc
  }

  //convert from bytes to degree Celsius, Fahrenheit, and humidity
  float cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
  float fTemp = (cTemp * 1.8) + 32;
  float humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

  String SHT30_ReadTempHumidity = (String(cTemp)+","+String(fTemp)+","+String(humidity));

  return SHT30_ReadTempHumidity;
}

float MQ2_ReadGas(int MQ2_SWICH_PIN, int MQ2_READ_PIN)
{
    //digitalWrite(MQ2_SWITCH_PIN,HIGH);
    //output range of sensor: 2.5V-3.3V (4.0V originally, 3.6V due to on-board Voltage Divider)
    //for ESP Boards, the ADC range is 0-3.3V:0-4095
    int ADC_READ = analogRead(MQ2_READ_PIN);

    return ADC_READ;

}

float ZE08_ReadFormaldehyde(int ZE08_SWITCH_PIN)
{
  byte byte_received[9] = {0};
  
  delay(150);
  
  ZE08_switchToQandAMode();

  delay(200);
  
  ZE08_querySensor(); // Send a query command to the sensor
  // Serial.println("Command Sent\n");

  if (Serial2.available() >= 9) 
  {
  // Assuming the sensor sends back 9 bytes of data as in the active mode
    
    for (int i = 0; i < 9; i++) 
    {
      // Read each byte of the response
      byte_received[i] = Serial2.read();
      // Note: The delay here might not be necessary since Serial2.read() is a blocking call
    }
  }
    
    // Calculate the concentration from the received data
    float CH2O_concentration = byte_received[2] * 256 + byte_received[3]; // Combining the high and low byte
    float CH2O_concentration_mgm3 = CH2O_concentration * 1.25 / 1000; // Convert to mg/m3
    float CH2O_concentration_ppm = CH2O_concentration / 1000.0; // Convert to ppm
    
    return CH2O_concentration_mgm3;
}

void LED_ON_OFF(int LED_pin,bool LED_State)
{

    if(LED_State)
    {
      digitalWrite(LED_pin, HIGH);
    }
    else 
    {
      digitalWrite(LED_pin,LOW);
    }  

}

void LED_Dimming(int LED_pin, double LED_freq, int LED_channel, int LED_resolution, int LED_dutyCycle)
{
  //set up PWM channel
  ledcSetup(LED_channel, LED_freq, LED_resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(LED_pin, LED_channel);

  ledcWrite(LED_channel, LED_dutyCycle);
}

void Buzzer_Dimming(int Buzzer_pin, double Buzzer_freq, int Buzzer_channel, int Buzzer_resolution, int Buzzer_dutyCycle)
{
  //set up PWM channel
  ledcSetup(Buzzer_channel, Buzzer_freq, Buzzer_resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(Buzzer_pin, Buzzer_channel);

  ledcWrite(Buzzer_channel, Buzzer_dutyCycle);
}

int ringMeter(int value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring

  int w = r / 3;    // Width of outer ring is 1/4 of radius
  
  int angle = 150;  // Half the sweep angle of meter (300 degrees)

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Variable to save "value" text colour from scheme and set default
  int colour = TFT_BLUE;
 
  // Draw colour blocks every inc degrees
  for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = TFT_RED; break; // Fixed colour
        case 1: colour = TFT_GREEN; break; // Fixed colour
        case 2: colour = TFT_BLUE; break; // Fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
        default: colour = TFT_BLUE; break; // Fixed colour
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      //text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }
  // Convert value to a string
  char buf[10];
  byte len = 3; if (value > 999) len = 5;
  dtostrf(value, len, 0, buf);
  buf[len] = ' '; buf[len+1] = 0; // Add blanking space and terminator, helps to centre text too!
  // Set the text colour to default
  tft.setTextSize(1);

  if (value<vmin || value>vmax) {
    drawAlert(x,y+40,50,1);
  }
  else {
    drawAlert(x,y+40,50,0);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  tft.setTextColor(colour, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  // Print value, if the meter is large then use big font 8, othewise use 4
  if (r > 84) {
    tft.setTextPadding(55*3); // Allow for 3 digits each 55 pixels wide
    tft.drawString(buf, x, y, 8); // Value in middle
  }
  else {
    tft.setTextPadding(3 * 14); // Allow for 3 digits each 14 pixels wide
    tft.drawString(buf, x, y, 4); // Value in middle
  }
  tft.setTextSize(1);
  tft.setTextPadding(0);
  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (r > 84) tft.drawString(units, x, y + 60, 4); // Units display
  else tft.drawString(units, x, y + 15, 2); // Units display

  // Calculate and return right hand side x coordinate
  return x + r;
} 

void drawAlert(int x, int y , int side, bool draw)
{
  if (draw) {//&& !range_error
    drawIcon(alert, x - alertWidth/2, y - alertHeight/2, alertWidth, alertHeight);
    // range_error = 1;
  }
  else if (!draw) {
    tft.fillRect(x - alertWidth/2, y - alertHeight/2, alertWidth, alertHeight, TFT_BLACK);
    // range_error = 0;
  }
}

unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

void drawIcon(const unsigned short* icon, int16_t x, int16_t y, int8_t width, int8_t height) {

  uint16_t  pix_buffer[BUFF_SIZE];   // Pixel buffer (16 bits per pixel)

  tft.startWrite();

  // Set up a window the right size to stream pixels into
  tft.setAddrWindow(x, y, width, height);

  // Work out the number whole buffers to send
  uint16_t nb = ((uint16_t)height * width) / BUFF_SIZE;

  // Fill and send "nb" buffers to TFT
  for (int i = 0; i < nb; i++) {
    for (int j = 0; j < BUFF_SIZE; j++) 
    {
      pix_buffer[j] = pgm_read_word(&icon[i * BUFF_SIZE + j]);
    }
    tft.pushColors(pix_buffer, BUFF_SIZE);
  }

  // Work out number of pixels not yet sent
  uint16_t np = ((uint16_t)height * width) % BUFF_SIZE;

  // Send any partial buffer left over
  // if (np) {
  //   for (int i = 0; i < np; i++) pix_buffer[i] = pgm_read_word(&icon[nb * BUFF_SIZE + i]);
  //   tft.pushColors(pix_buffer, np);
  // }

  tft.endWrite();
}  

void ZE08_switchToQandAMode() {
  byte switchCommand[] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
  Serial2.write(switchCommand,sizeof(switchCommand));
}

void ZE08_querySensor() {
  byte queryCommand[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  Serial2.write(queryCommand,sizeof(queryCommand));
}
