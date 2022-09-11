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
#define NumberOfNeoPixels 24  //  This is for the 24-LED neopixel ring.  Adjust for what you have for Neopixels
#define USE_SOFTWARESERIAL 1   ///< Set to 1 to use SoftwareSerial library, 0 for native serial port
#define Console   Serial   // command processor input/output stream
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define CMD(s) { Console.print(F("\n>")); Console.print(F(s)); Console.print(F(" ")); }

// Neopixel color for showing hour and 1/2 hours on the ring.  Adjust for Neopixel strip
int HourLedColor[] = {50, 50, 50, 100};  // Should pre-define some static colors

int neoPixelArray[NumberOfNeoPixels][4];  // RGBS array so we can set all NeoPixel and write all at once.
int chimeCounter = 0;

int oldHours = -1;
int oldMinutes = -1;
int oldSeconds = -1;

int Hours = 4;
int Minutes = 30;
int Seconds = -1;


// RGB strip has connected to NeoPixelPin.  There are other, more real-time modes, but we're saving our interrupts and timers for now.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NumberOfNeoPixels, NeoPixelPin, NEO_GRB + NEO_KHZ800);


// Connections for soft serial interface to the YX5300 module.  Note these are "D" (data) pin numbers, not physical pin numbers
const uint8_t MP3_RX = 0;    // connect to TX of MP3 Player module
const uint8_t MP3_TX = 2;    // connect to RX of MP3 Player module

//const uint8_t MP3_RX = 2;    // connect to TX of MP3 Player module
//const uint8_t MP3_TX = 0;    // connect to RX of MP3 Player module

SoftwareSerial  softserial(MP3_RX, MP3_TX);  // MP3 player serial stream for MP3 comms
MD_YX5300 mp3(softserial); //Create the  YX5300 object for managing audio
bool bUseCallback = true; // use callbacks
bool bUseSynch = false;   // not synchronous

// MP3 indicates when a track is done by sending a token over serial.  We'll watch for that on an interrupt.  Assume at start nothing is playi9ng.
bool isTrackFinished = true;



void readRTC() {
  ;  // Read the real-time clock, put results in Hours, Minutes, Seconds
  // This gets called during the main loop.
  // Results determine if chime or update Neopixel ring.
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

  Console.print(F(", 0x"));
  Console.print(status->data, HEX);
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

// Command processor handlers
void handlerHelp(char* param);

void handlerP_bang(char* param) {
  CMD("Play Start");
  mp3.playStart();
  cbResponse(mp3.getStatus());
}
void handlerPP(char* param)     {
  CMD("Play Pause");
  mp3.playPause();
  cbResponse(mp3.getStatus());
}
void handlerPZ(char* param)     {
  CMD("Play Stop");
  mp3.playStop();
  cbResponse(mp3.getStatus());
}
void handlerP_gt(char* param)   {
  CMD("Play Next");
  mp3.playNext();
  cbResponse(mp3.getStatus());
}
void handlerP_lt(char* param)   {
  CMD("Play Prev");
  mp3.playPrev();
  cbResponse(mp3.getStatus());
}

void handlerP(char* param)
{
  uint32_t t;

  getNum(param, t);
  CMD("Play Track");
  Console.print(t);
  mp3.playTrack(t);
  cbResponse(mp3.getStatus());
}

void handlerPT(char* param)
{
  uint32_t fldr, file;

  param = getNum(param, fldr);
  getNum(param, file);
  CMD("Play Specific Fldr");
  Console.print(fldr);
  Console.print(F(", "));
  Console.print(file);
  mp3.playSpecific(fldr, file);
  cbResponse(mp3.getStatus());
}

void handlerPF(char* param)
{
  uint32_t fldr;

  getNum(param, fldr);
  CMD("Play Folder");
  Console.print(fldr);
  mp3.playFolderRepeat(fldr);
  cbResponse(mp3.getStatus());
}

void handlerPX(char* param)
{
  uint32_t fldr;

  getNum(param, fldr);
  CMD("Play Shuffle Folder");
  Console.print(fldr);
  mp3.playFolderShuffle(fldr);
  cbResponse(mp3.getStatus());
}

void handlerPR(char* param)
{
  uint32_t file;

  getNum(param, file);
  CMD("Play File repeat");
  Console.print(file);
  mp3.playTrackRepeat(file);
  cbResponse(mp3.getStatus());
}


void handlerVM(char *param)
{
  uint32_t cmd;

  getNum(param, cmd);
  CMD("Volume Enable");
  Console.print(cmd);
  mp3.volumeMute(cmd != 0);
  cbResponse(mp3.getStatus());
}

void handlerV(char *param)
{
  uint32_t v;

  getNum(param, v);
  CMD("Volume");
  Console.print(v);
  mp3.volume(v);
  cbResponse(mp3.getStatus());
}

void handlerV_plus(char* param)  {
  CMD("Volume Up");
  mp3.volumeInc();
  cbResponse(mp3.getStatus());
}
void handlerV_minus(char* param) {
  CMD("Volume Down");
  mp3.volumeDec();
  cbResponse(mp3.getStatus());
}

void handlerQE(char* param) {
  CMD("Query Equalizer");
  mp3.queryEqualizer();
  cbResponse(mp3.getStatus());
}
void handlerQF(char* param) {
  CMD("Query File");
  mp3.queryFile();
  cbResponse(mp3.getStatus());
}
void handlerQS(char* param) {
  CMD("Query Status");
  mp3.queryStatus();
  cbResponse(mp3.getStatus());
}
void handlerQV(char* param) {
  CMD("Query Volume");
  mp3.queryVolume();
  cbResponse(mp3.getStatus());
}
void handlerQX(char* param) {
  CMD("Query Folder Count");
  mp3.queryFolderCount();
  cbResponse(mp3.getStatus());
}
void handlerQY(char* param) {
  CMD("Query Tracks Count");
  mp3.queryFilesCount();
  cbResponse(mp3.getStatus());
}

void handlerQZ(char* param)
{
  uint32_t fldr;

  getNum(param, fldr);
  CMD("Query Folder Files Count");
  Console.print(fldr);
  mp3.queryFolderFiles(fldr);
  cbResponse(mp3.getStatus());
}

void handlerS(char *param) {
  CMD("Sleep");
  mp3.sleep();
  cbResponse(mp3.getStatus());
}
void handlerW(char *param) {
  CMD("Wake up");
  mp3.wakeUp();
  cbResponse(mp3.getStatus());
}
void handlerZ(char *param) {
  CMD("Reset");
  mp3.reset();
  cbResponse(mp3.getStatus());
}

void handlerE(char *param)
{
  uint32_t e;

  getNum(param, e);
  CMD("Equalizer");
  Console.print(e);
  mp3.equalizer(e);
  cbResponse(mp3.getStatus());
}

void handlerX(char *param)
{
  uint32_t cmd;

  getNum(param, cmd);
  CMD("Shuffle");
  Console.print(cmd);
  mp3.shuffle(cmd != 0);
  cbResponse(mp3.getStatus());
}

void handlerR(char* param)
{
  uint32_t cmd;

  getNum(param, cmd);
  CMD("Repeat");
  Console.print(cmd);
  mp3.repeat(cmd != 0);
  cbResponse(mp3.getStatus());
}

void handlerY(char *param)
{
  uint32_t cmd;

  getNum(param, cmd);
  setSynchMode(cmd != 0);
}

void handlerC(char * param)
{
  uint32_t cmd;

  getNum(param, cmd);
  setCallbackMode(cmd != 0);
}

const MD_cmdProcessor::cmdItem_t PROGMEM cmdTable[] =
{
  { "?",  handlerHelp,    "",     "Help", 0 },
  { "h",  handlerHelp,    "",     "Help", 0 },
  { "p!", handlerP_bang,  "",     "Play", 1 },
  { "p",  handlerP,       "n",    "Play file index n (0-255)", 1 },
  { "pp", handlerPP,      "",     "Play Pause", 1 },
  { "pz", handlerPZ,      "",     "Play Stop", 1 },
  { "p>", handlerP_gt,    "",     "Play Next", 1 },
  { "p<", handlerP_lt,    "",     "Play Previous", 1 },
  { "pt", handlerPT,      "f n",  "Play Track folder f, file n", 1 },
  { "pf", handlerPF,      "f",    "Play loop folder f", 1 },
  { "px", handlerPX,      "f",    "Play shuffle folder f", 1 },
  { "pr", handlerPR,      "n",    "Play loop file index n", 1 },
  { "v+", handlerV_plus,  "",     "Volume up", 2 },
  { "v-", handlerV_minus, "",     "Volume down", 2 },
  { "v",  handlerV,       "x",    "Volume set x (0-30)", 2 },
  { "vm", handlerVM,      "b",     "Volume Mute on (b=1), off (0)", 2 },
  { "qe", handlerQE,      "",     "Query equalizer", 3 },
  { "qf", handlerQF,      "",     "Query current file", 3 },
  { "qs", handlerQS,      "",     "Query status", 3 },
  { "qv", handlerQV,      "",     "Query volume", 3 },
  { "qx", handlerQX,      "",     "Query folder count", 3 },
  { "qy", handlerQY,      "",     "Query total file count", 3 },
  { "qz", handlerQZ,      "f",    "Query files count in folder f", 3 },
  { "s",  handlerS,       "",     "Sleep", 4 },
  { "w",  handlerW,       "",     "Wake up", 4 },
  { "e",  handlerE,       "n",    "Equalizer type n", 5 },
  { "x",  handlerX,       "b",    "Play Shuffle on (b=1), off (0)", 5 },
  { "r",  handlerR,       "b",    "Play Repeat on (b=1), off (0)", 5 },
  { "z",  handlerZ,       "",     "Reset", 5 },
  { "y",  handlerY,       "b",    "Synchronous mode on (b=1), off (0)", 6 },
  { "c",  handlerC,       "b",    "Callback mode on (b=1), off (0)", 6 },
};

MD_cmdProcessor CP(Console, cmdTable, ARRAY_SIZE(cmdTable));


// handler functions
void handlerHelp(char* param)
{
  Console.print(F("\n[MD_YX5300 Test]\nSet Serial line ending to newline."));
  CP.help();
  Console.print(F("\n"));
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
  // Other person's cod for async CLI into the code.
  CP.begin();
  CP.help();

  pixels.begin(); //  Initializes the NeoPixel library.
  turnPixelsSolid(10,0,10);
  pixels.show();
}





int ringUpdateDelayCounter = 0;
int ringUpdateDelayTrigger = 200;   // Update the display once evry 20 times through loop
void loop()
{
  //CP.run();
  Console.print (" >");
  Console.println (ringUpdateDelayCounter);
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
      //delay (1000);
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
