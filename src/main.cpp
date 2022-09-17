// Test program for the MD_YX5300 library
//
// Menu driven interface using the Serial Monitor to test individual functions.
//
// Dependencies
// MD_cmdProcessor library found at https://github.com/MajicDesigns/MD_cmdProcessor
//
#include <Adafruit_NeoPixel.h>
#include <MD_YX5300.h>
#include <MD_cmdProcessor.h>
#include <SoftwareSerial.h>

#define ConsoleBaudRate 115200
#define NeoPixelPin D5
// Connections for soft serial interface to the YX5300 module.  Note these are "D" (data) pin numbers, not physical pin numbers
const uint8_t MP3_RX = 0;    // connect to TX of MP3 Player module
const uint8_t MP3_TX = 2;    // connect to RX of MP3 Player module

#define NumberOfNeoPixels 24  //  This is for the 24-LED neopixel ring.  Adjust for what you have for Neopixels
#define USE_SOFTWARESERIAL 1   ///< Set to 1 to use SoftwareSerial library, 0 for native serial port
#define Console   Serial   // command processor input/output stream
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define CMD(s) { Console.print(F("\n>")); Console.print(F(s)); Console.print(F(" ")); }



// Neopixel color for showing hour and 1/2 hours on the ring.  Adjust for Neopixel strip
int HourLedColor[] = {50, 50, 50, 100};  // TODO: Should pre-define some static colors

int neoPixelArray[NumberOfNeoPixels][4];  // RGBS array so we can set all NeoPixel and write all at once.


// Chimecounter is used to increment chimes up until the current hour count
int chimeCounter = 0;

// Start with an invalid "old" set of time values.   0:00 would be a valid time (midnight)

int oldHours = -1;
int oldMinutes = -1;
int oldSeconds = -1;

//  Test value for chime loop
int Hours = 4;
int Minutes = 30;
int Seconds = -1;


// RGB strip has connected to NeoPixelPin.  There are other, more real-time modes, but we're saving our interrupts and timers for now.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NumberOfNeoPixels, NeoPixelPin, NEO_GRB + NEO_KHZ800);

// Create a soft serial connection to the MP3 module
SoftwareSerial  softserial(MP3_RX, MP3_TX);  // MP3 player serial stream for MP3 comms
MD_YX5300 mp3(softserial); //Create the  YX5300 object for managing audio

bool bUseCallback = true; // use callbacks
bool bUseSynch = false;   // not synchronous

// MP3 module indicates when a track is done by sending a token over serial.  We'll watch for that on a serial interrupt.  Assume at start nothing is playi9ng, because
// we haven't yet asked for a play
bool isTrackFinished = true;


void readRTC() {
  ;  
  // Read the real-time clock, put results in Hours, Minutes, Seconds
  // This gets called during the main loop.
  // Results determine if chime or update Neopixel ring.
  // TODO: Stub for RTC module or NTP code.
  // RTP module will also need a "set values" routine
}

//simple function which takes values for the red, green and blue led and also
//a delay
void setColor(int led, int redValue, int greenValue, int blueValue, int delayValue)
{
  pixels.setPixelColor(led, pixels.Color(redValue, greenValue, blueValue));
  pixels.show();
  //delay(delayValue);
}

void showTimeOnNeopixelRing() {
  //  Update the  neopixel ring
  //  Values in Hours + minutes determine how many LEDs are lit, and how bright
  //  0, 2, 4, and other even-numbered LEDs represent an hour and are fully lit
  //  Odd-numberred LEDs represent the half-hour, and my be less fully lit or a different color
  //  This means noon and midnight would be dark....maybe have to re-think that.
  int i = 0;

  while ( i <= Hours ) {
    setColor(((i - 1) * 2), HourLedColor[0], HourLedColor[1], HourLedColor[2], HourLedColor[3]);
    i++;
  }
  //if (Minutes >= 30)  // Dim light for each half hour.  Might be able to do this with just luminance.
  //  setColor((((Hours - 1) * 2) + 1), (HourLedColor[0] / 2), (HourLedColor[1] / 2), (HourLedColor[2] / 2), (HourLedColor[3] / 2));
}

void turnPixelsSolid(int red, int green, int blue) {
  int led;
  for (led = 0; led <= NumberOfNeoPixels; led++)
  {
    setColor(led, red, green, blue, 10);
  }
}

void cbResponse(const MD_YX5300::cbData *status)
// Used to process device responses either as a library callback function
// or called locally when not in callback mode.
{
  if (bUseSynch)
    Console.print(F("\nSync Status: "));
  else
    Console.print(F("\nCback status: "));

  switch (status->code)
  {
    case MD_YX5300::STS_OK:         Console.print(F("STS_OK"));         break;
    case MD_YX5300::STS_TIMEOUT:    Console.print(F("STS_TIMEOUT"));    break;
    case MD_YX5300::STS_VERSION:    Console.print(F("STS_VERSION"));    break;
    case MD_YX5300::STS_CHECKSUM:   Console.print(F("STS_CHECKSUM"));    break;
    case MD_YX5300::STS_TF_INSERT:  Console.print(F("STS_TF_INSERT"));  break;
    case MD_YX5300::STS_TF_REMOVE:  Console.print(F("STS_TF_REMOVE"));  break;
    case MD_YX5300::STS_ERR_FILE:   Console.print(F("STS_ERR_FILE"));   break;
    case MD_YX5300::STS_ACK_OK:     Console.print(F("STS_ACK_OK"));     break;
    case MD_YX5300::STS_FILE_END:   {
        Console.print(F("STS_FILE_END"));
        isTrackFinished = true;  // Tom modification to say track is finished, perhaps play next chime if needed
      } break;
    case MD_YX5300::STS_INIT:       Console.print(F("STS_INIT"));       break;
    case MD_YX5300::STS_STATUS:     Console.print(F("STS_STATUS"));     break;
    case MD_YX5300::STS_EQUALIZER:  Console.print(F("STS_EQUALIZER"));  break;
    case MD_YX5300::STS_VOLUME:     Console.print(F("STS_VOLUME"));     break;
    case MD_YX5300::STS_TOT_FILES:  Console.print(F("STS_TOT_FILES"));  break;
    case MD_YX5300::STS_PLAYING:    Console.print(F("STS_PLAYING"));    break;
    case MD_YX5300::STS_FLDR_FILES: Console.print(F("STS_FLDR_FILES")); break;
    case MD_YX5300::STS_TOT_FLDR:   Console.print(F("STS_TOT_FLDR"));   break;
    default: Console.print(F("STS_??? 0x")); Console.print(status->code, HEX); break;
  }

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


void setup()
{
  // YX5300 Serial interface
  softserial.begin(MD_YX5300::SERIAL_BPS);
  mp3.begin();
  setCallbackMode(bUseCallback);
  setSynchMode(bUseSynch);


  // Console interface from the IDE
  Console.begin(ConsoleBaudRate);


  pixels.begin(); //  Initializes the NeoPixel library.
  turnPixelsSolid(10,0,10);
  pixels.show();
}





int ringUpdateDelayCounter = 0;
int ringUpdateDelayTrigger = 200;   // Update the display once evry 20 times through loop
void loop()
{

  //Console.println (ringUpdateDelayCounter);
  ringUpdateDelayCounter++;
  if (ringUpdateDelayCounter >= ringUpdateDelayTrigger) {
    ringUpdateDelayCounter = 0;
    //Console.print (" >");
    //Console.println (ringUpdateDelayCounter);
    showTimeOnNeopixelRing();
  }

  while (chimeCounter < Hours) {  //used to be TimesToChime

    Console.print ("\b\b-=");
    Console.println (isTrackFinished);
    //delay(1000);
    if (isTrackFinished == true) {
      turnPixelsSolid(0, 0, 0);
      Console.print ("Beginning chime #");

      setColor(chimeCounter,20,20,20,100); //red

      Console.print (chimeCounter);
      Console.print ("\n");
      Console.print ("isTrackFinished is TRUE, so STARTING A PLAY.  First setting isTrackFinished to FALSE\n");
      isTrackFinished = false;
      delay (300);
      turnPixelsSolid(0, 0, 0);
      mp3.playSpecific(1, 1);
      //delay(100);
      chimeCounter++;
    }


    delay (100);
    mp3.check();
  }
  //turnPixelsSolid(0, 0, 0);
  //showTimeOnNeopixelRing();
}
