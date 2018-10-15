/////////////////////////////////////////////////////////////
/*
   The FIRMWARE for the Ghost Bros Walkie Talkie Prop
   Watch the Build Video HERE -
   It uses GPS to determine your location and cue an audio file specific to that location.
   All audio files must be in the format of 8.3, (8 letters or numbers dot 3 letters or numbers - ex. AAAAA001.MP3)

   Created by Sean Hodgins 10/03/2018

   You can get the audio files from... INSERT WEBSITE
   The SD Card MUST contain the following files:
   intro000.mp3
   place001.mp3 through place013.mp3
   gpsstart.mp3
   gpsfound.mp3

  You can have unlimited (as SD Card permits) "random" sayings. You must declare the amount in the definition "randomTalkNum".
  Random talking can occur when the user is not close to a designated location.
  The random talk files will need to be named as:
  random00.mp3 - random99.mp3
  Do not skip any numbers.
*/


#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

#define numLocations 11

#define randomTalkNum 2
int randomTalkPlayed[randomTalkNum] = {0};

//static const
double Loc_Lat[11] = {49.2843991, 49.283803, 49.283784, 49.283575, 49.283151, 49.283020, 49.283246, 49.283368, 49.283331, 49.283481, 49.283738};
//static const
double Loc_Lon[11] = { -123.1088747, -123.106501, -123.106359,  -123.106587, -123.105319, -123.104172, -123.104099, -123.103920, -123.104192, -123.104020, -123.105401};

char* trackLocation[] = {"place001.mp3", "place002.mp3", "place003.mp3", "place004.mp3", "place005.mp3", "place006.mp3", "place007.mp3", "place008.mp3", "place009.mp3", "place010.mp3", "place011.mp3"};

int distanceTo = 0;
int minimumDist = 50;

int CurrDest = 0;
int LastDest = 0;

#define LEDY 3
#define LEDR 4

#define Serial SerialUSB

#define BUTTTOP 9
#define BUTTBOT 10
#define BUTTENC 11

#define ENC1 13
#define ENC2 12
int encoderValue = 290;
int lastEncoded = 0;
int newA = 0;
int sum = 0;


#define XDCS A3
#define MP3CS A4
#define SDCS A5

#define PIN 1

int volume = 50;

// These are the pins used for the Audio
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS       A4     // VS1053 chip select pin (output)
#define VS1053_DCS     A3     // VS1053 Data/command select pin (output)
#define CARDCS          A5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     5     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


int inLocation = 0;
int currLocation = 0;
int notification = 0;
TinyGPSPlus gps;


int leastdistance = 0;

void setup()
{

  Loc_Lat[2] = 45.5540529;
  Loc_Lon[2] = -75.2820992;
  Serial.begin(115200);
  Serial1.begin(9600);
  //while (!Serial);
  pinMode(BUTTENC, INPUT_PULLUP);
  pinMode(BUTTTOP, INPUT);
  pinMode(BUTTBOT, INPUT);
  pinMode(LEDY, OUTPUT);
  pinMode(LEDR, OUTPUT);


  pinMode(ENC1, INPUT);
  pinMode(ENC2, INPUT);
  attachInterrupt(ENC1, updateEncoder, CHANGE);
  attachInterrupt(ENC2, updateEncoder, CHANGE);
  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1) {
      digitalWrite(LEDR, HIGH);
      delay(500);
      digitalWrite(LEDY, HIGH);
      delay(500);
      digitalWrite(LEDR, LOW);
      delay(500);
      digitalWrite(LEDY, LOW);
      delay(500);
    }
  }
  Serial.println(F("VS1053 found"));
  musicPlayer.setVolume(volume, 0);
  musicPlayer.sineTest(0x44, 200);    // Make a tone to indicate VS1053 is working
  digitalWrite(LEDY, HIGH);
  digitalWrite(LEDR, HIGH);
  delay(500);
  digitalWrite(LEDY, LOW);
  digitalWrite(LEDR, LOW);


  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1) {
      digitalWrite(LEDY, HIGH);
      delay(500);
      digitalWrite(LEDR, HIGH);
      delay(500);
      digitalWrite(LEDY, LOW);
      delay(500);
      digitalWrite(LEDR, LOW);
      delay(500);
    }
  }
  Serial.println("SD OK!");

  musicPlayer.setVolume(volume, 0);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int




  attachInterrupt(BUTTTOP, PLAY, FALLING);
  attachInterrupt(BUTTBOT, PAUSE, FALLING);
  attachInterrupt(BUTTENC, STOP, FALLING);

  delay(100);

  Serial.println(F("Searching for GPS"));
  musicPlayer.playFullFile("intro000.mp3");

  while (Serial1.available() > 0)
    gps.encode(Serial1.read());
  while (gps.location.isUpdated() == 0) {
    while (Serial1.available() > 0)
      gps.encode(Serial1.read());
    delay(100);
    Serial.println("Waiting for GPS");
    if (musicPlayer.stopped()) {
      //musicPlayer.startPlayingFile("waiting.mp3");
      //delay(100);
    }
  }

  musicPlayer.stopPlaying();
  Serial.println(F("GPS Found"));


}

void loop()
{
  if (musicPlayer.playingMusic == 1) {
    digitalWrite(LEDY, HIGH);
  }
  if (musicPlayer.stopped()) {
    digitalWrite(LEDY, LOW);
  }
  while (Serial1.available() > 0)
    gps.encode(Serial1.read());

  if (musicPlayer.stopped()) {
    if (gps.location.isValid())
    {
      int locationsum = 0;
      leastdistance = 5000000;
      for (int x = 0; x < numLocations; x++) {
        double distanceToLoc = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), Loc_Lat[x], Loc_Lon[x]);
        int MetersTo = distanceToLoc;
        if (MetersTo < leastdistance){
          leastdistance = MetersTo;
        }
        if (MetersTo < 50) {

          locationsum++;
          currLocation = x;
          if (notification == 0) {
            notification = 1;
            Serial.print("In Location :");
            Serial.println(x);
            Serial.print("Distance: ");
            Serial.println(MetersTo);
          }
        }

      }
      if (locationsum > 0) {
        inLocation = 1;
        if (notification == 1) {
          digitalWrite(LEDR, HIGH);
          musicPlayer.sineTest(0x44, 200);
          notification = 2;
        }
      }
      else {
        inLocation = 0;
        digitalWrite(LEDR, LOW);
      }
    }
  }
  delay(100);
  while (musicPlayer.paused()) {
    for (int y = 255; y > 0; y--) {
      analogWrite(LEDY, y);
      delay(5);
    }
    for (int y = 0; y < 255; y++) {
      analogWrite(LEDY, y);
      delay(5);
    }
    pinMode(LEDY, OUTPUT);
  }

}



void DestChange() {

  int buttstate = digitalRead(4);
  while (buttstate == 0) {
    buttstate = digitalRead(4);
  }
  CurrDest = CurrDest + 1;
  if (CurrDest > 3) {
    CurrDest = 0;
  }
  if (LastDest != CurrDest) {

    LastDest = CurrDest;
  }
}

void PLAY() {
  int butt = digitalRead(BUTTTOP);
  while (butt == 0) {
    butt = digitalRead(BUTTTOP);
  }
  Serial.print("Distance: ");
  Serial.println(leastdistance);
  if (musicPlayer.stopped()) {
    if (inLocation == 1) {
      
      musicPlayer.startPlayingFile(trackLocation[currLocation]);
      notification = 0;
      digitalWrite(LEDY, 255);
    }
  }
}

void STOP() {
  int butt = digitalRead(BUTTENC);
  while (butt == 0) {
    butt = digitalRead(BUTTENC);
  }
  musicPlayer.stopPlaying();
  digitalWrite(LEDY, 0);
}

void PAUSE() {
  int butt = digitalRead(BUTTBOT);
  while (butt == 0) {
    butt = digitalRead(BUTTBOT);
  }
  if (! musicPlayer.paused()) {
    Serial.println("Paused");
    musicPlayer.pausePlaying(true);
  } else {
    Serial.println("Resumed");
    musicPlayer.pausePlaying(false);
  }

}

void VChange() {
  if (! musicPlayer.stopped()) {
    if (! musicPlayer.paused()) {
      musicPlayer.pausePlaying(true);
      Serial.print("Volume: ");
      Serial.println(volume);
      if (volume > 0 && volume < 60) {
        musicPlayer.setVolume(volume, 0);
      }
      musicPlayer.pausePlaying(false);
    }
  }
}


void updateEncoder() {
  int MSB = digitalRead(ENC1); //MSB = most significant bit
  int LSB = digitalRead(ENC2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b0010 || sum == 0b1101 || sum == 0b1011 || sum == 0b0100) {
    if (encoderValue > 0) {
      encoderValue--;
    }
  }
  if (sum == 0b1110 || sum == 0b1000 || sum == 0b0001 || sum == 0b0111) {
    if (encoderValue < 300) {
      encoderValue++;
    }
  }
  //Serial.println(encoderValue);
  volume = encoderValue / 5;

  VChange();
  lastEncoded = encoded; //store this value for next time
}
