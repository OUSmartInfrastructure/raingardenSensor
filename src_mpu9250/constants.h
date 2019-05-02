// Timing controls
#define NUMANGLESAVERAGE            5               // Number of angle averages that will be stored every
#define HOURSVOLTAGEREPORTING       4              // Number of hours between voltage reports to serial or cloud m2x (set to 0 for no reporting)
#define THRESHOLDANGLE              7.0               // Angle in degrees above which a report of an array of averages is reported
#define TIMEBETWEENANGLEREADSEC     30            // Number of seconds between consecutive angle reads, if set to every 30 seconds and 12 samples 5 averages covers 30 minutes
#define CONSECUTIVEANGFORAVERAGE    12              // Number of consecutive samples to store before computing one of the NUMANGLESAVERAGE averages

#define MPUADDR                     0x68            // i2c address of mpu6050
#define MPUSETUPREGISTER            0x6B            // i2c address of register during setup
#define MPUWRITEREGISTER            0x3B            // i2c register address to read angles
#define MINVAL                      265             // from external sample code to read mpu6050 angle correctly
#define MAXVAL                      402             // from external sample code to read mpu6050 angle correctly

#define SERIALTXRATE                9600            // bps rate for USB serial communications

#define TZONE                       -5              // Set time zone

#define DELAYBETWEENANGLEPUBLISHING 450             // Delay in ms between angles are published to the cloud

// Output control
#define OUTSERIAL                   TRUE            // Print output to the serial USB port
#define OUTCLOUD                    TRUE           // Print output to the cloud service (m2x)

// Integration webhook name. itsElectron01 uses l01, m02; itsElectron02 uses l02, m02 ;used in function ReportAnglesWithTimeStamps
#define WH1DEVID1                   "d1wh1"
#define WH2DEVID1                   "d1wh2"
#define WH1DEVID2                   "d2wh1"
#define WH2DEVID2                   "d2wh2"
#define WH1DEVID3                   "d3wh1"
#define WH2DEVID3                   "d3wh2"

#define WHVDEVID1                   "d1whv"
#define WHVDEVID2                   "d2whv"
#define WHVDEVID3                   "d3whv"

// Debug control
#define DBGLVL1                     TRUE            // Print setup information during initialization
#define DBGLVL2                     TRUE            // Prints sensor data to screen, e.g. angle to screen
#define DBGLVL3                     TRUE            // Prints timer and function in/out information

// MATH constants
#define PI                          3.141592
#define RADTODEG                    (180/3.141592)

// Number of electrons in project
#define NUMELECTRONS                2

// Device ids of electrons in project
#define DEVID1                      "3d001XXXX951343334363138"
#define DEVID2                      "38004XXXXb51343334363138"
#define DEVID3                      "30005XXXX951353338363036" //test photon
