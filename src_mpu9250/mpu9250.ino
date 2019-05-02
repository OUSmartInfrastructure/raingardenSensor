#include <math.h>
#include "constants.h"
#include "quaternionFilters.h"
#include "MPU9250.h"

// Author: Julio Arauz
// License: MIT license
// email: arauz@ohio.edu
// Copyright: 2018
// Changes log: V2.1 -  a. Added device id lookup to decide what webhook is used when publishing set of angles to cloud
//                      b. Angles are reported between 0 and 90 degrees
//                      c. Changed THRESHOLDANGLE constant to report when above a certain angle, not below
// Changes log: V2.2 -  a. Modified code to work with MPU9250


// Global variables used to read angles from mpu6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

// Angle reading timing control
unsigned long readAngleIntervalms = TIMEBETWEENANGLEREADSEC * 1000;         // Output interval in ms to print angle to screen in dblLvl2
int flgReadInterval = 0;                                                    // Aux variable used for printing debuggin in loop
unsigned long angleLastReadTime;


// Structure for returning read angles
struct Angles{
    float angleX;
    float angleY;
    float angleZ;
};


// Battery monitoring
FuelGauge fuel;

// MPU9250 sensor unit
MPU9250 mpu9250;

// Timers
// Timer1 is used for reporting battery voltage
Timer timer1( (unsigned int) (HOURSVOLTAGEREPORTING*3600*1000), FlagReportVoltage);

// Global vars to act after a timer expires as the callback should not
// deal with publishing to the cloud
bool voltageHrsTimerFlag = FALSE;

// Global variables for loop
int samplesRead=0;                            // Used to count how many samples have been read before computing the current average
float sumSampleAngle =0;
int currentAverageIndex=0;                  // Used to keep track which average index is used in the averageAngles array
float currentAverage;
float averageAngles[NUMANGLESAVERAGE];
String averageAnglesTimestamp[NUMANGLESAVERAGE];
bool angleExceeded = FALSE;                 // Flag variable change to true in loop when one of the angels exceeds THRESHOLDANGLE, if so report angles and switch if off again
String devID;                               // Used to retrieve the device ID of the Electron running the code and then used to see which webhook name must be used
                                            // when publishing a list of angles to the cloud

void mpu9250_Self_Test()
{
  // Read the WHO_AM_I register, this is a good test of communication
  //byte c = mpu9250.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  byte c = mpu9250.readByte(0x68, WHO_AM_I_MPU9250);
  if (DBGLVL1 == TRUE) Serial.print("MPU9250 "); Serial.print("I AM "); Serial.print(c, HEX);
  if (DBGLVL1 == TRUE) Serial.print(" I should be "); Serial.println(0x71, HEX);


  if (c == 0x71) // WHO_AM_I should always be 0x68
  {
    if (DBGLVL1 == TRUE) Serial.println("MPU9250 is online...");

    // Start by performing self test and reporting values
    mpu9250.MPU9250SelfTest(mpu9250.SelfTest);

    // Calibrate gyro and accelerometers, load biases in bias registers
    mpu9250.calibrateMPU9250(mpu9250.gyroBias, mpu9250.accelBias);

    mpu9250.initMPU9250();
    // Initialize device for active mode read of acclerometer, gyroscope, and
    // temperature
    if (DBGLVL1 == TRUE) Serial.println("MPU9250 initialized for active data mode....");

    // Read the WHO_AM_I register of the magnetometer, this is a good test of
    // communication
    byte d = mpu9250.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
    if (DBGLVL1 == TRUE) Serial.print("AK8963 "); Serial.print("I AM "); Serial.print(d, HEX);
    if (DBGLVL1 == TRUE) Serial.print(" I should be "); Serial.println(0x48, HEX);

    // Get magnetometer calibration from AK8963 ROM
    mpu9250.initAK8963(mpu9250.magCalibration);
    // Initialize device for active mode read of magnetometer
    if (DBGLVL1 == TRUE) Serial.println("AK8963 initialized for active data mode....");
    if (DBGLVL2)
    {
      //  Serial.println("Calibration values: ");
      Serial.print("X-Axis sensitivity adjustment value ");
      Serial.println(mpu9250.magCalibration[0], 2);
      Serial.print("Y-Axis sensitivity adjustment value ");
      Serial.println(mpu9250.magCalibration[1], 2);
      Serial.print("Z-Axis sensitivity adjustment value ");
      Serial.println(mpu9250.magCalibration[2], 2);
    }

  } // if (c == 0x71)
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
  }
}

void mpu9250_calc_acc_gs()
{
  // Now we'll calculate the acceleration value into actual g's
  // This depends on scale being set
  mpu9250.ax = (float)mpu9250.accelCount[0]*mpu9250.aRes; // - accelBias[0];
  mpu9250.ay = (float)mpu9250.accelCount[1]*mpu9250.aRes; // - accelBias[1];
  mpu9250.az = (float)mpu9250.accelCount[2]*mpu9250.aRes; // - accelBias[2];

  mpu9250.readGyroData(mpu9250.gyroCount);  // Read the x/y/z adc values
  mpu9250.getGres();
}


void mpu9250_update_quaternions()
{
  // Must be called before updating quaternions!
  mpu9250.updateTime();

  // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
  // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
  // (+ up) of accelerometer and gyro! We have to make some allowance for this
  // orientationmismatch in feeding the output to the quaternion filter. For the
  // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
  // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
  // modified to allow any convenient orientation convention. This is ok by
  // aircraft orientation standards! Pass gyro rate as rad/s
//  MadgwickQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
  MahonyQuaternionUpdate(mpu9250.ax, mpu9250.ay, mpu9250.az, mpu9250.gx*DEG_TO_RAD,
                         mpu9250.gy*DEG_TO_RAD, mpu9250.gz*DEG_TO_RAD, mpu9250.my,
                         mpu9250.mx, mpu9250.mz, mpu9250.deltat);
}

void setup(){
    // Initialise I2C communication as MASTER
    Wire.begin();

    // Debug
    Serial.begin(SERIALTXRATE);

    delay(300);


	  mpu9250_Self_Test();

    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Serial port configured for %d bps.", SERIALTXRATE);
    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Starting setup...");
    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Setting time zone to: %d ...", int(TZONE));
    Time.zone(TZONE);

    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Turning on Daylight Savings Time");
    Time.beginDST();


    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Current time and date are: %s", Time.timeStr().c_str());

    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Configuring wire I2C to interface with device at address: 0x%x.", MPUADDR);

    devID = System.deviceID();
    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Device id is: %s", devID.c_str());

    Wire.begin();
    Wire.beginTransmission(MPUADDR);
    Wire.write(MPUSETUPREGISTER);
    Wire.write(0);
    Wire.endTransmission(true);

    if (DBGLVL1 == TRUE){
        if (HOURSVOLTAGEREPORTING >0) Serial.printlnf("[DBG1 setup] Timer1 for battery voltage reporting every %f hours, or %d ms is ON", (float) HOURSVOLTAGEREPORTING, (int) (HOURSVOLTAGEREPORTING*3600*1000));
        else Serial.printlnf("[DBG1 setup] Timer1 for battery reporting is OFF");
    }

    if (HOURSVOLTAGEREPORTING > 0) timer1.start();

    if (DBGLVL1 == TRUE) Serial.printlnf("[DBG1 setup] Finished setup... continuing...");
}

// Read angles from mpu6050
struct Angles ReadAngles() {

	// On interrupt, check if data ready interrupt
    if (mpu9250.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
    {
      mpu9250.readAccelData(mpu9250.accelCount);  // Read the x/y/z adc values
      mpu9250.getAres();

      mpu9250_calc_acc_gs();
    }// if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

    mpu9250_update_quaternions();
  	delay(1000);

    /*int16_t tmpX = 1000*mpu9250.ax;
    int16_t tmpY = 1000*mpu9250.ay;
    int16_t tmpZ = 1000*mpu9250.az;

  	//int xAng = map(tmpX,MINVAL,MAXVAL,-90,90);*/
    int xAng = map((int16_t) (1000*mpu9250.ax),MINVAL,MAXVAL,-90,90);
    int yAng = map((int16_t) (1000*mpu9250.ay),MINVAL,MAXVAL,-90,90);
    int zAng = map((int16_t) (1000*mpu9250.az),MINVAL,MAXVAL,-90,90);

    float x= ( (float) RADTODEG ) * (atan2(-yAng, -zAng) + ((float) PI) );
    float y= ( (float) RADTODEG ) * (atan2(-xAng, -zAng) + ((float) PI) );
    float z= ( (float) RADTODEG ) * (atan2(-yAng, -xAng) + ((float) PI) );

    /*float x = 1000*mpu9250.ax;
    float y = 1000*mpu9250.ay;
    float z = 1000*mpu9250.az;*/

    // We are only intereted in angle y adjust it so that it is between 0 and 90 degress
    y = y - 270;
    if (y < 0) y = y * (-1);
    if ((int) y==45) y=0;

    Angles a = {x, y , z};

    Wire.endTransmission();
    return a;
}

// Flag that the voltage report timer has expired
void FlagReportVoltage() {
    if (DBGLVL3 == TRUE) Serial.printlnf("[DBG3 FlagReportVoltage] Timer for reporting voltage expired, turning flag on");

    voltageHrsTimerFlag = TRUE;
}



// report voltage to screen or cloud
void ReportVoltage() {

    if (DBGLVL3 == TRUE) Serial.printlnf("[DBG3 ReportVoltage] Voltage will be reported... flag for timer will be turned off...");

    voltageHrsTimerFlag = FALSE;

    if (OUTSERIAL) {
        Serial.printlnf("[ReportVoltage] (OUTSERIAL ON) Voltage [V]: %f", fuel.getVCell());
        Serial.printlnf("[ReportVoltage] (OUTSERIAL ON) State of charge (%): %f", fuel.getSoC());
    }

    if (OUTCLOUD){
        float data = fuel.getVCell();
        String strData = String(data);
        if (DBGLVL3) {
            Serial.printlnf("[DBG3 ReportVoltage] (OUTCLOUD ON) length: %d", strData.length());
            Serial.print("[DBG3 ReportVoltage] (OUTCLOUD ON) Voltage read: ");
            Serial.print(strData);
            Serial.printlnf(" [V]");
        }

        if (OUTSERIAL) {
            Serial.print("[ReportVoltage] (OUTSERIAL ON) Publishing value ");
            Serial.print(strData);
            Serial.printlnf(" to webhook v");
        }

        time_t time = Time.now();
        String dataToPublish = String::format("{ \"ts0\": \"%s\", \"v0\": %.1f }", Time.format(time, TIME_FORMAT_ISO8601_FULL).c_str(), strData);

        Particle.publish(WHVDEVID1, dataToPublish, PRIVATE);

    }


}

void ReportAngle(){
    int i;

    if (OUTSERIAL) {
        Serial.printlnf("[ReportAngle] (OUTSERIAL ON) At least one of the angles was above the threshold of %f", (float) THRESHOLDANGLE);
        Serial.println("[ReportAngle] (OUTSERIAL ON) Angles are: ");
        for (i=0; i<NUMANGLESAVERAGE; i++){
            Serial.printf("%f ", averageAngles[i]);
        }
        Serial.printlnf("...");
        Serial.println("[ReportAngle] (OUTSERIAL ON) Timestamps are: ");
        for (i=0; i<NUMANGLESAVERAGE; i++){
            Serial.print(averageAnglesTimestamp[i]);
            Serial.print(" ");
        }
        Serial.printlnf("...");
    }

    if (OUTCLOUD){
        for (i=0; i<NUMANGLESAVERAGE; i++){
            float data = averageAngles[i];
            String strData = String(data, 2);
            if (DBGLVL3) {
                Serial.print("[DBG3 ReportAngle] Data to be published is: ");
                Serial.println(strData);
            }

            if (OUTSERIAL) Serial.printlnf("[ReportAngle] (OUTSERIAL ON) Publishing value %d to webhook s.", i);
            Particle.publish("s", strData, PRIVATE);
            delay(DELAYBETWEENANGLEPUBLISHING);
        }
    }

} // end reportangle function

void ReportAnglesWithTimeStamps(){
    int i;

    if (OUTSERIAL) {
        Serial.printlnf("[ReportAnglesWithtimeStamps] (OUTSERIAL ON) At least one of the angles was above the threshold of %f", (float) THRESHOLDANGLE);
        Serial.println("[ReportAnglesWithtimeStamps] (OUTSERIAL ON) Angles are: ");
        for (i=0; i<NUMANGLESAVERAGE; i++){
            Serial.printf("%f ", averageAngles[i]);
        }
        Serial.printlnf("...");
        Serial.println("[ReportAnglesWithtimeStamps] (OUTSERIAL ON) Timestamps are: ");
        for (i=0; i<NUMANGLESAVERAGE; i++){
            Serial.print(averageAnglesTimestamp[i]);
            Serial.print(" ");
        }
        Serial.printlnf("...");
    }

    if (OUTCLOUD){
        // Construct JSON string to be sent as event
        // Particle Publish can only take a string that is up to 255 in length so multiple publishes are used
        String dataToPublish;

        float ang0 = averageAngles[0];
        float ang1 = averageAngles[1];
        float ang2 = averageAngles[2];
        float ang3 = averageAngles[3];
        float ang4 = averageAngles[4];


        // Before publishing obtain the names of the Webhooks to use
        String firstWebhook = FirstWebhookName(devID);
        String secondWebhook = SecondWebhookName(devID);

        if (DBGLVL3) Serial.printlnf("[DBG3 ReportAnglesWithTimeStamps] First webhook name to publish angles is %s", firstWebhook.c_str());
        if (DBGLVL3) Serial.printlnf("[DBG3 ReportAnglesWithTimeStamps] Second webhook name to publish angles is %s", secondWebhook.c_str());



        dataToPublish = String::format("{ \"ts0\": \"%s\", \"an0\": %.2f, \"ts1\": \"%s\", \"an1\": %.2f, \"ts2\": \"%s\", \"an2\": %.2f }", averageAnglesTimestamp[0].c_str(), ang0, averageAnglesTimestamp[1].c_str(), ang1, averageAnglesTimestamp[2].c_str(), ang2);


        if (DBGLVL3) {
            Serial.print("[DBG3 ReportAnglesWithtimeStamps] First set of data to be published is: ");
            Serial.println(dataToPublish);
            Serial.printlnf("DBG3 Length of first set of data to be published is: %d", dataToPublish.length());
        }

        if (OUTSERIAL) {
            Serial.print("[ReportAnglesWithtimeStamps] (OUTSERIAL ON) Publishing values and timestamps to webhook: ");
            Serial.println(firstWebhook);
        }
        Particle.publish(firstWebhook, dataToPublish, PRIVATE); // Publish first three out of five readings
        delay(DELAYBETWEENANGLEPUBLISHING);


        dataToPublish = String::format("{ \"ts3\": \"%s\", \"an3\": %.2f, \"ts4\": \"%s\", \"an4\": %.2f }", averageAnglesTimestamp[3].c_str(), ang3, averageAnglesTimestamp[4].c_str(), ang4);

        if (DBGLVL3) {
            Serial.print("[DBG3 ReportAnglesWithtimeStamps] Second set of data to be published is: ");
            Serial.println(dataToPublish);
            Serial.printlnf("DBG3 Length of second set of data to be published is: %d", dataToPublish.length());
        }

        if (OUTSERIAL) {
            Serial.print("[ReportAnglesWithtimeStamps] (OUTSERIAL ON) Publishing values and timestamps to webhook: ");
            Serial.println(secondWebhook);
        }

        Particle.publish(secondWebhook, dataToPublish, PRIVATE); // Publish last two of the five readings

    } // end if outcloud

} // end report angles with timestamps


// Check the first and second webhook names to use depending on the device id
char* FirstWebhookName(String deviceID){
    if (deviceID.equals(DEVID1)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 FirstWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH1DEVID1);
    }
    else if (deviceID.equals(DEVID2)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 FirstWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH1DEVID2);
    }
    else if (deviceID.equals(DEVID3)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 FirstWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH1DEVID3);
    }
    else {
        if (DBGLVL3) Serial.printlnf("Did not find match for device ID %s, using first webhook as default", deviceID.c_str());
        return (WH1DEVID1);
    }
}

char* SecondWebhookName(String deviceID){
    if (deviceID.equals(DEVID1)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID1);
    }
    else if (deviceID.equals(DEVID2)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID2);
    }
    else if (deviceID.equals(DEVID3)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID3);
    }
    else{
        if (DBGLVL3) Serial.printlnf("Did not find match for device ID %s, using first webhook as default", deviceID.c_str());
        return (WH2DEVID1);
    }
}
char* VoltageWebhookName(String deviceID){
    if (deviceID.equals(DEVID1)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 VoltageWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WHVDEVID1);
    }
    else if (deviceID.equals(DEVID2)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 VoltageWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WHVDEVID2);
    }
    else if (deviceID.equals(DEVID3)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 VoltageWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WHVDEVID3);
    }
    else{
        if (DBGLVL3) Serial.printlnf("Did not find match for device ID %s, using first voltageb webhook as default", deviceID.c_str());
        return (WHVDEVID1);
    }
}

char* SecondWebhookName(String deviceID){
    if (deviceID.equals(DEVID1)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID1);
    }
    else if (deviceID.equals(DEVID2)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID2);
    }
    else if (deviceID.equals(DEVID3)) {
        if (DBGLVL3) Serial.printlnf("[DBG3 SecondWebhookName] Found match for device ID %s", deviceID.c_str());
        return (WH2DEVID3);
    }
    else{
        if (DBGLVL3) Serial.printlnf("Did not find match for device ID %s, using first webhook as default", deviceID.c_str());
        return (WH2DEVID1);
    }
}

void loop(){

    Angles ang;                                                 // Structure used to hold all angle values, called by ReadAngles
    int i;
    time_t time;

    // Check if any of the timers have expired
    if ( voltageHrsTimerFlag == TRUE ) ReportVoltage();

    // Read angles every TIMEBETWEENANGLEREADSEC seconds
    if (flgReadInterval == 0) {
        // if angle has not been read yet
        ang = ReadAngles();                                     // read angle and store into data structure ang
        samplesRead++;
        angleLastReadTime = millis();
        flgReadInterval = 1;                                     // mark angle as read for the first time
        if (DBGLVL3) Serial.printlnf("[DBG3 loop] First time Angle read: %f at %ld", ang.angleY, angleLastReadTime);
        }
    else {
        // if readAngleIntervalms have elapsed since last reeading
        if ((unsigned long) (millis() - angleLastReadTime) > readAngleIntervalms) {
            ang = ReadAngles();
            samplesRead++;
            angleLastReadTime = millis();
            if (DBGLVL3) Serial.printlnf("[DBG3 loop] Angle read: %f at %ld", ang.angleY, angleLastReadTime);
        }
    }

    // Store CONSECUTIVEANGFORAVERAGE samples and compute average
    if (samplesRead <= CONSECUTIVEANGFORAVERAGE) {
        sumSampleAngle += ang.angleY;
    }
    else {
        averageAngles[currentAverageIndex] = sumSampleAngle / CONSECUTIVEANGFORAVERAGE;
        time = Time.now();
        averageAnglesTimestamp[currentAverageIndex++] = Time.format(time, TIME_FORMAT_ISO8601_FULL);
        if (DBGLVL3) Serial.printlnf("[DBG3 loop] %d samples have been collected with an average of %f this is average number %d of %d", CONSECUTIVEANGFORAVERAGE, averageAngles[currentAverageIndex - 1], currentAverageIndex, NUMANGLESAVERAGE);

        samplesRead = 1;
        sumSampleAngle = ang.angleY;

    }

    if (currentAverageIndex == NUMANGLESAVERAGE){
        currentAverageIndex = 0;
        if (DBGLVL2) Serial.printlnf("[DBG3 loop] Checking if any of the averages went above %f, if so report all %d angles.", (float) THRESHOLDANGLE, NUMANGLESAVERAGE);
        for (i=0; i<NUMANGLESAVERAGE; i++){
            if (averageAngles[i] > (float) THRESHOLDANGLE) {
                angleExceeded = TRUE;
                break;
            }
            else {
                angleExceeded = FALSE;
            }
        }

        if (DBGLVL3) Serial.printlnf("[DB3 loop] Checked angles... One of them was above %f is (TRUE/FALSE) %d", (float) THRESHOLDANGLE, angleExceeded);

        if (angleExceeded) {
            // ReportAngle();
            ReportAnglesWithTimeStamps();
            angleExceeded = FALSE;
        }

    }

} // end loop function
