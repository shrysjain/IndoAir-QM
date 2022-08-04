#include "Zanshin_BME680.h"  // Include Zanduino's BME680 Sensor library for BME68x control
#include "bsec.h" // Include Bosch's BSEC Fusion library for extra BME68x control
#include <SparkFun_Qwiic_OLED.h> // Include SparkFun's breakout OLED library with the QWIIC connect system

// Global variables
int bsecIAQ;
String AQissue;
String AQissueFixes;

const uint32_t SERIAL_SPEED{115200};  // Set the baud rate for Serial I/O

// Create an instance of the BME680 class
BME680_Class BME680;

//Forward function declaration with default value for sea level
float altitude(const int32_t press, const float seaLevel = 1013.25);
float altitude(const int32_t press, const float seaLevel) {
  static float Altitude;
  Altitude =
      44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into meters
  return (Altitude);
}  

// Declare helper functions for BSEC library
void checkIaqSensorStatus(void);
void errLeds(void);

// Create an object of the BSEC class
Bsec iaqSensor;

// BSEC Variables
String output;

// Create an object of the QWIICMicroOLED class
QwiicMicroOLED myOLED;

void setup() {
  Serial.begin(SERIAL_SPEED);  // Start serial port at Baud rate
#ifdef __AVR_ATmega32U4__      // If this is a 32U4 processor, then wait 3 seconds to init USB port
  delay(3000);
#endif
  Serial.print(F("Starting air quality monitoring program\n"));
  Serial.print(F("- Initializing BME688\n"));
  while (!BME680.begin(I2C_STANDARD_MODE)) {  // Start BME680 using I2C, use first device found
    Serial.print(F("-  Unable to find BME680. Trying again in 5 seconds.\n"));
    delay(5000);
  }  // of loop until device is located
  Serial.print(F("- Setting 16x oversampling for all sensors\n"));
  BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
  BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
  BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
  Serial.print(F("- Setting IIR filter to a value of 4 samples\n"));
  BME680.setIIRFilter(IIR4);  // Use enumerated type values
  Serial.print(F("- Setting gas measurement to 320\xC2\xB0\x43 for 150ms\n"));  // "�C" symbols
  BME680.setGas(320, 150);  // 320�c for 150 milliseconds

  Wire.begin();

  iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  output = "\nBSEC Library Version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[2] {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
  };

  iaqSensor.updateSubscription(sensorList, 2, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // Print the header
  output = "IAQ, breath VOC equivalent";
  Serial.println(output);

  if (myOLED.begin() == false) {
    Serial.println("OLED Initialization Failed - Freezing");
    while (true);
  }
  
  Serial.println("OLED Initialization Success");
  Serial.println();

  write("Loading");

}  // of method setup()

void loop() {
  int breezeIAQI;
  static int temp, humidity, pressure, gas;  // Assign variables for BME readings
  
  BME680.getSensorData(temp, humidity, pressure, gas);  // Get readings

  // Serial.println("Temperature: " + String((temp/100) - 3) + "° C"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
  // Serial.println("Relative Humidity: " + String(humidity/1000) + "%"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
  
  int tempIAQI = (temp/100) - 3;  
  int humiIAQI = humidity/1000;

  // Round the humidity to 10 for use with the chart 
  int humiLastDigit = humiIAQI % 10;
  if (humiLastDigit > 4) {
    humiIAQI = humiIAQI + (10 - humiLastDigit);
  } else if (humiLastDigit < 5) {
    humiIAQI = humiIAQI - humiLastDigit;
  }
  
  // Cross values against a chart by Breeze Technologies (DE) for an IAQI rating of 1 - 6, a number < 4 is good IAQI rating, otherwise poor IAQI rating
  if (tempIAQI < 18 || tempIAQI > 25 || humiIAQI < 40 || humiIAQI > 90 || tempIAQI == 18 && humiIAQI == 40 || tempIAQI == 19 && humiIAQI == 40 || tempIAQI == 18 && humiIAQI == 50 || tempIAQI == 18 && humiIAQI == 90|| tempIAQI == 23 && humiIAQI == 90 || tempIAQI == 24 && humiIAQI == 80 || tempIAQI == 24 && humiIAQI == 90 || tempIAQI == 25 && humiIAQI == 70 || tempIAQI == 25 && humiIAQI == 80) {
    // Serial.println("IAQI is Poor"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    breezeIAQI = 0;
  } else {
    // Serial.println("IAQI is Good"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    breezeIAQI = 1;
  }

  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available
    output = "Time: " + String(time_trigger);
    output += ", IaQ: " + String(iaqSensor.iaq);
    output += ", b-VOC: " + String(iaqSensor.breathVocEquivalent);
    // Serial.println(output); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP

    if (iaqSensor.iaq < 25) {
      bsecIAQ = 0; // Amazing IAQ
      // Serial.println("IAQ (Not Temp/Humi Dependant) is Amazing"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    } else if (iaqSensor.iaq > 24 && iaqSensor.iaq < 251) {
      bsecIAQ = 1; // Good IAQ
      //Serial.println("IAQ (Not Temp/Humi Dependant) is Good"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    } else if (iaqSensor.iaq > 250 && iaqSensor.iaq < 501) {
      bsecIAQ = 2; // Poor IAQ
      //Serial.println("IAQ (Not Temp/Humi Dependant) is Poor"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    } else {
      bsecIAQ = 3; // Error 
      //Serial.println("IAQ (Not Temp/Humi Dependant) is: ERROR"); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
    }
  } else {
    checkIaqSensorStatus();
  }

  if (breezeIAQI == 1) {
    if (bsecIAQ == 0 || bsecIAQ == 1) {
      doubleWrite("AQ is Very", "Good, Safe");
    } else if (bsecIAQ == 2) {
      doubleWrite("AQ is Good", "Some VOCs");
    } else {
      doubleWrite("Error in", "Reading AQ");
    }
  } else if (breezeIAQI == 0) {
    if (bsecIAQ == 0 || bsecIAQ == 1) {
      doubleWrite("AQ is Poor,", "No VOCs");
    } else if (bsecIAQ == 2) {
      doubleWrite("AQ is", "Very Poor");
    } else {
      doubleWrite("Error in", "Reading AQ");
    }
  } else {
    doubleWrite("Error in", "Reading AQ");
  }

  // Detecting issue in AQ
  if (tempIAQI < 18 && humiIAQI > 94) {
    AQissue = "Too Cold and Too Humid";
    AQissueFixes = "Use a heater or air conditioner to heaten the area and bring down the humidity";
  } else if (tempIAQI < 18) {
    AQissue = "Too Cold";
    AQissueFixes = "Use a heater or turn off AC/fans/anything cooling the area";
  } else if (tempIAQI > 25 && humiIAQI > 94) {
    AQissue = "Too Hot and Too Humid";
    AQissueFixes = "Use an air conditioner to dry out the air and circulate cold air";
  } else if (tempIAQI > 25) {
    AQissue = "Too Hot";
    AQissueFixes = "Use AC/fans to circulate more cold air";
  } else if (humiIAQI > 94) {
    AQissue = "Too Humid";
    AQissueFixes = "Run a heater or air conditioner which will dry out the air in your area";
  } else if (humiIAQI < 35) {
    AQissue = "Not Humid Enough";
    AQissueFixes = "Use a vaporizer, steam machine, or humidifier to raise humidity and moisture";
  } else if (tempIAQI == 25 && humiIAQI > 64 || tempIAQI == 24 && humiIAQI > 75) {
    AQissue = "Too Hot and Too Humid";
    AQissueFixes = "Use an air conditioner to dry out the air and circulate cold air";
  } else if (tempIAQI == 23 && humiIAQI > 85) {
    AQissue = "Too Humid";
    AQissueFixes = "Run a heater or air conditioner which will dry out the air in your area";
  } else if (tempIAQI == 18 && humiIAQI < 55) {
    AQissue = "Too Cold and Not Humid Enough";
    AQissueFixes = "Use humidifers/heaters or try to tackle the issues one-by-one";
  } else if (tempIAQI == 18 && humiIAQI > 84) {
    AQissue = "Too Cold and Too Humid";
    AQissueFixes = "Use a heater to heat up the air and dry it out at the same time";
  } else if (tempIAQI == 19 && humiIAQI == 40) {
    AQissue = "Not Humid Enough";
    AQissueFixes = "Use a vaporizer, steam machine, or humidifier to raise humidity and moisture";
  } else {
    AQissue = "No Issue with Air Quality";
    AQissueFixes = "No issue to fix";
  }

  // Serial.println("Air Quality Issue: " + String(AQissue)); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP
  // Serial.println("AQ Issue Fixes: " + AQissueFixes); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMP

  Serial.println(String(AQissue));
  
  // Serial.println(); COMMENTED OUT OF SERIAL MONITOR DATA PRINT DUMB
  delay(10000);
}  // of method loop()

void write(String str) {
  int intStringWidth, intStringHeight;
  
  myOLED.erase();

  intStringWidth = (myOLED.getWidth() - myOLED.getStringWidth(str)) / 2;
  intStringHeight = (myOLED.getHeight() - myOLED.getStringHeight(str)) / 2;

  myOLED.text(intStringWidth, intStringHeight, str, 1);
  myOLED.display();
} // of method write()

void doubleWrite(String one, String two) {
  int x0, y0, x1, y1;

  myOLED.erase();

  myOLED.text(0, 16, one, 1);
  myOLED.text(0, 26, two, 1);

  myOLED.display();
} // of method doubleWrite()

void checkIaqSensorStatus(void) {
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
} // of method checkIaqSensorStatus()

void errLeds(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
} // of method errLeds()
