// Test program for the MD_YX5300 library
//
// Menu driven interface using the Serial Monitor to test individual functions.
//
// Dependencies
// MD_cmdProcessor library found at https://github.com/MajicDesigns/MD_cmdProcessor
//

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
//#include <Streaming.h>      // https://github.com/janelia-arduino/Streaming

//#include <Adafruit_NeoPixel.h>
#include <MD_YX5300.h>
#include <MD_cmdProcessor.h>
#include <SoftwareSerial.h>





// Connections for soft serial interface to the YX5300 module.  Note these are "D" (data) pin numbers, not physical pin numbers
const uint8_t MP3_RX = 0;    // connect to TX of MP3 Player module
const uint8_t MP3_TX = 2;    // connect to RX of MP3 Player module

#define ConsoleBaudRate 115200
#define USE_SOFTWARESERIAL 1   ///< Set to 1 to use SoftwareSerial library, 0 for native serial port
#define Console   Serial   // command processor input/output stream
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define CMD(s) { Console.print(F("\n>")); Console.print(F(s)); Console.print(F(" ")); }

//DS3232RTC myRTC;

// TODO:  Move to Display include file.  Neopixel color for showing hour and 1/2 hours on the ring.  Adjust for Neopixel strip


// Chimecounter is used to increment chimes up until the current hour count
int chimeCounter = 0;

// Start with an invalid "old" set of time values.   0:00 would be a valid time (midnight)

int oldHours = -1;
int oldMinutes = -1;
int oldSeconds = -1;

//  Test value for chime loop
int Hours = 12;
int Minutes = 30;
int Seconds = -1;

//  We will check that there are readable audio files
int filesOnDisk = 0;  



// Create a soft serial connection to the MP3 module
SoftwareSerial  softserial(MP3_RX, MP3_TX);  // MP3 player serial stream for MP3 comms
MD_YX5300 mp3(softserial); //Create the  YX5300 object for managing audio

bool bUseCallback = true; // use callbacks
bool bUseSynch = false;   // not synchronous

// MP3 module indicates when a track is done by sending a token over serial.  We'll watch for that on a serial interrupt.  Assume at start nothing is playi9ng, because
// we haven't yet asked for a play
bool isTrackFinished = true; // When we start to play a track, we'll make it false.  A response/interrupt from the MP3 module will make it true again 
int numberOfTracks = 0;  //  We'll read to see how many tracks are on the SD card
bool trackStatusChange = false;
bool fileCountStatusChange = false;
bool trackError = false;


char outputString[20];

void cbResponse(const MD_YX5300::cbData *status)
// Used to process device responses either as a library callback function
// or called locally when not in callback mode.
{
 
  Console.print ("Callback:");

//Console.print ("Callback data:");
//Console.println(status->data);
  switch (status->code)
  {
    case MD_YX5300::STS_OK:         Console.print(F("STS_OK"));         break;
    case MD_YX5300::STS_TIMEOUT:    Console.print(F("STS_TIMEOUT"));    break;
    case MD_YX5300::STS_VERSION:    Console.print(F("STS_VERSION"));    break;
    case MD_YX5300::STS_CHECKSUM:   Console.print(F("STS_CHECKSUM"));    break;
    case MD_YX5300::STS_TF_INSERT:  Console.print(F("STS_TF_INSERT"));  break;
    case MD_YX5300::STS_TF_REMOVE:  Console.print(F("STS_TF_REMOVE"));  break;
    case MD_YX5300::STS_ERR_FILE: {
      // Seems to happen randomly.  Retry the track after a delay  
      trackError = true;
      Console.print(F("STS_ERR_FILE"));   break;
    } break;
    case MD_YX5300::STS_ACK_OK:       Console.print(F("STS_ACK_OK")); break;
    case MD_YX5300::STS_FILE_END:   {
        Console.print(F("STS_FILE_END"));
        trackStatusChange = true;
        isTrackFinished = true;  //  global to say track is finished, perhaps play next track or finish
      } break;
    case MD_YX5300::STS_INIT:       Console.print(F("STS_INIT"));       break;
    case MD_YX5300::STS_STATUS:     Console.print(F("STS_STATUS"));     break;
    case MD_YX5300::STS_EQUALIZER:  Console.print(F("STS_EQUALIZER"));  break;
    case MD_YX5300::STS_VOLUME:     Console.print(F("STS_VOLUME"));     break;

    case MD_YX5300::STS_TOT_FILES: {  
      //Console.print(F("STS_TOT_FILES: "));  
      //Console.print(status->data);
      numberOfTracks = status->data;
      fileCountStatusChange = true;
    } break;
    case MD_YX5300::STS_PLAYING:    Console.print(F("STS_PLAYING"));    break;
    case MD_YX5300::STS_FLDR_FILES: Console.print(F("STS_FLDR_FILES")); break;
    case MD_YX5300::STS_TOT_FLDR:   Console.print(F("STS_TOT_FLDR"));   break;
    default: Console.print(F("STS_??? 0x")); Console.print(status->code, HEX); break;
  }
  Console.print("\n");

  //Console.print(F(", 0x"));
  //Console.print(status->data, HEX);
}

void setCallbackMode(bool b)
{
  bUseCallback = b;
  CMD("Callback");
  Console.print(b ? F("ON") : F("OFF"));
  mp3.setCallback(b ? cbResponse : nullptr);
}

void setSynchMode(bool b)
{
  bUseSynch = b;
  CMD("Synchronous");
  Console.print(b ? F("ON") : F("OFF"));
  mp3.setSynchronous(b);
}

char * getNum(char *cp, uint32_t &v, uint8_t base = 10)
{
  char* rp;

  v = strtoul(cp, &rp, base);
  return (rp);
}



void initMp3Module() {
  // YX5300 Serial interface
  softserial.begin(MD_YX5300::SERIAL_BPS);
  mp3.begin();
  setCallbackMode(bUseCallback);
  setSynchMode(bUseSynch);
}

void printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    Serial.print(':');
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}



void setup()
{
  Console.begin(ConsoleBaudRate);

  Serial.begin(115200);
  initMp3Module();
  /*
  Serial << F( "\n" __FILE__ "\n" __DATE__ " " __TIME__ "\n" );

  myRTC.begin();
  delay(500);
  
  setSyncProvider(myRTC.get);   // the function to get the time from the RTC
  if(timeStatus() != timeSet)
        Serial.println("Unable to sync with the RTC");
  else
        Serial.println("RTC has set the system time");
   */
  ///
 //// initClockDisplay();
  // Send a request to check if there are files on the SD card
  // Result will be in filesOnDisk
  ////mp3.queryFilesCount();
  ////delay(1000);


}

void loop()
{

  // In the loop we'll be:
  //  1.  Checking for variables changed by callbacks.
  //      these variables are:
  //         filesOnDisk - Returned after query to list the number of files
  //         isTrackFinished -  Did the MP3 module complete playing a track
  //         FUTURE:  When web message is received, e.g. from sensor, play appropriate track
  //  2.  Periodically updating the RTC
  //  3.  Checking if it's time to chime, e.g. XX:00 on the clock
  //  4.  Periodically update any neopixel/7-segment or other display

  // (Asynchronously) Check for status changes in the MP3 module.  Could be Track Finished, File Count, etc

  Console.print("L");
  mp3.check();

  // Make sure we have a track to play
  if (fileCountStatusChange == true)
  {
    Console.print("Number of tracks:");
    Console.print(numberOfTracks);
    Console.print("\n");
    fileCountStatusChange = false;
    delay(500);
  }

  // Can only be one type of response at a time.

  else if (isTrackFinished == true)
  { // Set to true initially, and on completion of each track
    // Console.print ("isTrackFinished:True\n");

    // Do we need to play additional chimes?
    if (chimeCounter < Hours)
    {
      chimeCounter++;
      Console.print("CHIME ");
      Console.println(chimeCounter);
      Console.print("\n");
      // delay (200);
      Console.print("STARTING TRACK 1,1\n");
      mp3.playSpecific(1, 1); // Send serial request to play track. In loop, monitor isTrackFinished, which will be set by callback
      isTrackFinished = false;
      // delay (200);
    }
    delay (100);
  }
  else if (trackError == true) {
    // Whoops.  Had a problem playing a track.  Retry after a delay
      Console.println("OMG.  I hit a track error.  Retrying....");
      trackError = false;
      mp3.playSpecific(1, 1);
      delay (500);

  }
    delay(100);
}
