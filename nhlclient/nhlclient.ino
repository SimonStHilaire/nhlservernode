#include "Arduino.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Tone32.h>

//AP
#include "EEPROM.h"
#include <WiFi.h>

//Visual
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/Picopixel.h>
#include "Adafruit_LEDBackpack.h"

#define DEBUG_PRINT false

#define ONE_MINUTE (60 * 1000)
#define ONE_HOUR (3600 * 1000)
#define PRE_GAME_HOURS 10
#define PRE_GAME_MILLIS (PRE_GAME_HOURS * ONE_HOUR)

#define SCROLL_DELAY 100

#define BUZZER_PIN 12

#define RESET_PIN 13
#define NEXT_TEAM_PIN 13
#define PREVIOUS_TEAM_PIN 14

#define FREQUENCY 2000
#define BUZZER_CHANNEL 0
#define BUZZER_REZ 8

#define MTL_GOAL_SONG_SIZE 6
int MTL_GOAL_SONG[MTL_GOAL_SONG_SIZE][2] = {{NOTE_C4,150}, {NOTE_E4, 150}, {NOTE_G4, 150}, {NOTE_C5, 300}, {NOTE_G4, 150}, {NOTE_C5, 500}};
int CurrentMtlGoalNote = MTL_GOAL_SONG_SIZE;
unsigned long LastMtlGoalNoteTime = 0;

#define VS_GOAL_SONG_SIZE 4
int VS_GOAL_SONG[VS_GOAL_SONG_SIZE][2] = {{NOTE_C5,225}, {NOTE_B4, 225}, {NOTE_AS4, 225}, {NOTE_A4, 1200}};
int CurrentVsGoalNote = VS_GOAL_SONG_SIZE;
unsigned long LastVsGoalNoteTime = 0;

#define GAMESTART_SONG_SIZE 6
int GAMESTART_SONG[GAMESTART_SONG_SIZE][2] = {{NOTE_C5,300}, {NOTE_C5, 300}, {NOTE_A4, 300}, {NOTE_G4, 300}, {NOTE_C5, 300}, {NOTE_C5, 300}};
int CurrentGameStartNote = GAMESTART_SONG_SIZE;
unsigned long LastGameStartNoteTime = 0;

#define MTL_WIN_SONG_SIZE 14
int MTL_WIN_SONG[MTL_WIN_SONG_SIZE][2] = {{NOTE_C4,150}, {NOTE_D4, 150}, {NOTE_F4, 300}, {NOTE_F4, 600}, {NOTE_F4, 150}, {NOTE_G4, 150}, {NOTE_GS4, 300}, {NOTE_GS4, 600}, {NOTE_C5, 300}, {NOTE_AS4, 300}, {NOTE_G4, 600}, {NOTE_G4, 150}, {NOTE_DS4, 150}, {NOTE_F4, 600}};

#define VS_WIN_SONG_SIZE 11
int VS_WIN_SONG[VS_WIN_SONG_SIZE][2] = {{NOTE_C4,600}, {NOTE_C4, 450}, {NOTE_C4, 300}, {NOTE_C4, 600}, {NOTE_DS4, 450}, {NOTE_D4, 300}, {NOTE_D4, 450}, {NOTE_C4, 300}, {NOTE_C4, 450}, {NOTE_B3,300}, {NOTE_C4, 600}};

Adafruit_8x8matrix PeriodDisplay = Adafruit_8x8matrix();
Adafruit_8x8matrix MtlDisplay = Adafruit_8x8matrix();
Adafruit_8x8matrix VsDisplay = Adafruit_8x8matrix();

String ssid     = "";
String password = "";

String ap_ssid     = "MTL_SCORE";
String ap_password = "12345678";

const char* URL = "https://wl2qdbard4.execute-api.ca-central-1.amazonaws.com/dev/getscore?scores&team=";  // Server URL

int port = 80;

int TeamsIds[31] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 52, 53, 54};
int TEAM = 8; //MTL
int CurrentTeamIndex = 7; //MTL

bool CanNext = false;
bool CanPrev = false;

unsigned long LastTeamChangeTime = 0;

//AP
WiFiServer server(80);
String header;

const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFmTCCBIGgAwIBAgIQDh+h475hcAblUszPHZYxzjANBgkqhkiG9w0BAQsFADBG\n" \
"MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRUwEwYDVQQLEwxTZXJ2ZXIg\n" \
"Q0EgMUIxDzANBgNVBAMTBkFtYXpvbjAeFw0xOTA4MDkwMDAwMDBaFw0yMDA5MDkx\n" \
"MjAwMDBaMDMxMTAvBgNVBAMMKCouZXhlY3V0ZS1hcGkuY2EtY2VudHJhbC0xLmFt\n" \
"YXpvbmF3cy5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC0XrTZ\n" \
"t4Ogq32TOIIzZAqr8jqutRMV/mFfw3acKeeJcR/NQLj60XBzi8wCvw8hh99RkJYh\n" \
"cfXUK3FtRRbY3LBjmnT6BLHpuqrnITKAybAH9EmNLi+iPTMHszrJ/hM0ZZPOsBTe\n" \
"cdVhJSyrU+nZ/CCYEgiEPRso8J5pzguohrXqlYLxd5wWWKFc5SkDaCo7jQ01+Ejz\n" \
"XSFacqqbUo9zGrdwZCgMuVn5wY5+sIwdOVh1uIdn5pJH7cfKmTo1VJzNmhgFuNa4\n" \
"U3RnGKjA/A54nAZ2buShZGw+aUEfhl1uqvrkOxGpZT94+Csjbm+FbFjLxm76PHSD\n" \
"OYSADtZb80fQ/+J1AgMBAAGjggKUMIICkDAfBgNVHSMEGDAWgBRZpGYGUqB7lZI8\n" \
"o5QHJ5Z0W/k90DAdBgNVHQ4EFgQUou4wHEf9l4l+ifxv6Sy7az+jLfswMwYDVR0R\n" \
"BCwwKoIoKi5leGVjdXRlLWFwaS5jYS1jZW50cmFsLTEuYW1hem9uYXdzLmNvbTAO\n" \
"BgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMDsG\n" \
"A1UdHwQ0MDIwMKAuoCyGKmh0dHA6Ly9jcmwuc2NhMWIuYW1hem9udHJ1c3QuY29t\n" \
"L3NjYTFiLmNybDAgBgNVHSAEGTAXMAsGCWCGSAGG/WwBAjAIBgZngQwBAgEwdQYI\n" \
"KwYBBQUHAQEEaTBnMC0GCCsGAQUFBzABhiFodHRwOi8vb2NzcC5zY2ExYi5hbWF6\n" \
"b250cnVzdC5jb20wNgYIKwYBBQUHMAKGKmh0dHA6Ly9jcnQuc2NhMWIuYW1hem9u\n" \
"dHJ1c3QuY29tL3NjYTFiLmNydDAMBgNVHRMBAf8EAjAAMIIBBAYKKwYBBAHWeQIE\n" \
"AgSB9QSB8gDwAHYAu9nfvB+KcbWTlCOXqpJ7RzhXlQqrUugakJZkNo4e0YUAAAFs\n" \
"d42X4wAABAMARzBFAiEAkD4GdjvUKZWOw9MOpVK5k02GdGnB23Y39LB+150FSF0C\n" \
"IHRH5htUkYF7HFjzf7IzM5jnS1UbfKoSs8DovtIfSQPHAHYAh3W/51l8+IxDmV+9\n" \
"827/Vo1HVjb/SrVgwbTq/16ggw8AAAFsd42YHQAABAMARzBFAiEAjGAekmDkSjuy\n" \
"mzjgGHcdrc/D9GmzzQXJdCnQ9wymjKsCIHHSyfzWV7dEOL1BKukZWsUSn40RAsFi\n" \
"rM/nSix7+YQuMA0GCSqGSIb3DQEBCwUAA4IBAQCq57Js3zuiwzUvV/SA1YbpUKDa\n" \
"U04dhDSPEhb5m4J+mjapTOFBzbvhIZKz8hgdNbmiu6V7eg94QBCzEJ1trKi/VHyU\n" \
"RufV5JWPesTjvUiX6QFGvuhQ1XzJ3rj0O8Xa73+Ns8GV8S2+5mQ7B6pnOorBubRT\n" \
"wwLtVmlehCczabnwpeqjyoeMIBOPd2EV5Y6S0gkyTO2sZ575lLAM17yEL1E1NHeD\n" \
"/xLO67IPin6Ip3qEV/7Zq+nFobbNh1fZne/oGtDpuZ23Q12J/f0RoDZHZDvhJir5\n" \
"TufNlFAxvNKoPXfgBQuyCUEZSGgGf4WymkgSeiWJg+p6DqlMIJ4/pVCPAETF\n" \
"-----END CERTIFICATE-----\n";


unsigned long LastGetDataMillis = 0;

char Message[256];

int error = 22;//Not connected to network
unsigned long waitTime = 0;
int mtl = 0;
int vs = 0;
int period = 1;

int PreviousMtl = 0;
int PreviousVs = 0;

unsigned long ErrorCount = 0;

const long ErrorUpdateDelay = ONE_MINUTE;
const long LiveUpdateDelay = 10000;

unsigned long LastAnimationTime = 0;
int CurrentAnimationFrame = 0;
#define ANIMATION_DELAY 60

unsigned long LastScrollTime = 0;
int CurrentScroll = 0;

enum State
{
  Initial,
  Configuring,
  Initialization,
  PreGame,
  InGame,
  PostGame
};

State CurrentState = Initial;

//Forwards
void delayFor(long milliseconds);
void UpdateVisual();
bool GetData();
void TurnOffDisplays();
void Connect();
bool IsConnected();
bool ReadCredentials(String& ssid, String& psk);
String GetGetParam(String prefix, String suffix);
void HandleClient();
void SetCredentials(String ssid, String psk);
void PlayMtlWin();
void PlayVsWin();
String GetNameFromTeamId(int id);
void DisplayTeamAbv(String teamAbv);

void setup()
{
  if(DEBUG_PRINT) Serial.begin(115200);
  delay(100);

  pinMode(RESET_PIN, INPUT);
  pinMode(PREVIOUS_TEAM_PIN, INPUT);

  PeriodDisplay.begin(0x70);
  MtlDisplay.begin(0x71);
  VsDisplay.begin(0x74); //Note, the A2 and A2 soldering pin are inverted

  TurnOffDisplays();

  ledcSetup(BUZZER_CHANNEL, FREQUENCY, BUZZER_REZ);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  if (!EEPROM.begin(65) && DEBUG_PRINT)
  {
    Serial.println("Failed to initialise EEPROM");
  }

  TEAM = ReadTeamId();

  if(!ReadCredentials(ssid, password) || digitalRead(RESET_PIN) == LOW)
  {
    SetState(Configuring);
  }
  else
  {
    SetState(Initialization);
  }
}

void SetState(State newState)
{
  if(newState == CurrentState)
  {
    return;
  }
  
  if(DEBUG_PRINT)
  {
    Serial.print(("New state: "));
    Serial.println(newState);
  }
 
  switch(newState)
  {
    case Configuring:
    {
      WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());

      IPAddress IP = WiFi.softAPIP();
      
      if(DEBUG_PRINT)
      {
        Serial.print("AP IP address: ");
        Serial.println(IP);
      }

      server.begin();

      PlayStartGame();

      TurnOffDisplays();
      
      sprintf(Message,"Configuration\0");
     
      break;
    }
    case Initialization:
    {
      GetData();
        
      break;
    }
    case PreGame:
    {
      LastScrollTime = millis();
      CurrentScroll = 17;

      LastAnimationTime = millis();
      CurrentAnimationFrame = 0;
             
      if(DEBUG_PRINT)
      {
        int waitHours = waitTime/ONE_HOUR;
        int waitMinutes = (waitTime - (waitHours*ONE_HOUR)) / ONE_MINUTE;

        if(DEBUG_PRINT)
        {
          Serial.print("Next game in: ");
          Serial.print(waitHours);
          Serial.print(":");
          Serial.println(waitMinutes);
        }
      }

      UpdateRemainingTime();
      
      break;
    }
    case InGame:
    {   
      UpdateRemainingTime();

      GetData();
      UpdateVisual();

      delay(10);

      PlayStartPeriod();

      break;
    }

    case PostGame:
    {
      if(CurrentState == InGame)
      {
        PeriodDisplay.clear();
       
        PeriodDisplay.setCursor(1,1);
        PeriodDisplay.setTextSize(1);
        PeriodDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
        PeriodDisplay.setTextColor(LED_ON);
        PeriodDisplay.print("F");
        PeriodDisplay.writeDisplay();

        if(PreviousMtl > PreviousVs)
        {
          PlayMtlWin();
        }
        else
        {
          PlayVsWin();
        }
      }
      break;
    }
  }

  CurrentState = newState;
}

void DrawPeriodClock(int frame)
{
  switch(CurrentAnimationFrame)
    {
      case 0:
      {
        PeriodDisplay.drawLine(3,3, 3,0, LED_ON);
        break;
      }

      case 1:
      {
        PeriodDisplay.drawLine(3,3, 5,1, LED_ON);
        break;
      }

      case 2:
      {
        PeriodDisplay.drawLine(3,3, 5,3, LED_ON);
        break;
      }

      case 3:
      {
        PeriodDisplay.drawLine(3,3, 5,5, LED_ON);
        break;
      }

      case 4:
      {
        PeriodDisplay.drawLine(3,3, 3,5, LED_ON);
        break;
      }

      case 5:
      {
        PeriodDisplay.drawLine(3,3, 1,5, LED_ON);
        break;
      }

      case 6:
      {
        PeriodDisplay.drawLine(3,3, 1,3, LED_ON);
        break;
      }

      case 7:
      {
        PeriodDisplay.drawLine(3,3, 1,1, LED_ON);
        break;
      }
    }
}

void UpdateWaitAnimation()
{
  if((millis() - LastAnimationTime) > ANIMATION_DELAY)
  {
    CurrentAnimationFrame++;

    if(CurrentAnimationFrame > 7)
      CurrentAnimationFrame = 0;
    
    PeriodDisplay.setFont(NULL);
    PeriodDisplay.setTextSize(1);
    PeriodDisplay.setTextWrap(false);
    PeriodDisplay.clear();
    PeriodDisplay.drawCircle(3,3, 3, LED_ON);

    DrawPeriodClock(CurrentAnimationFrame);
    
    PeriodDisplay.writeDisplay();
    
    LastAnimationTime = millis();
  }
}

void UpdateRemainingTime()
{
  unsigned long remaining = waitTime - (millis() - LastGetDataMillis);
  
  int waitHours = (waitTime == 0) ? 0 : remaining/ONE_HOUR;
  int waitMinutes = (waitTime == 0) ? 0 : ((remaining - (waitHours*ONE_HOUR)) / ONE_MINUTE) + 1;

  if(waitMinutes == 60)
  {
    waitHours += 1;
    waitMinutes = 0;
  }

  MtlDisplay.setFont(&Picopixel);
  MtlDisplay.setTextSize(1);
  MtlDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  MtlDisplay.setTextColor(LED_ON);
  MtlDisplay.clear();
  MtlDisplay.setCursor(1,6);
  if(waitHours < 10) MtlDisplay.print("0");
  MtlDisplay.print(waitHours);  
  MtlDisplay.writeDisplay();

  VsDisplay.setFont(&Picopixel);
  VsDisplay.setTextSize(1);
  VsDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  VsDisplay.setTextColor(LED_ON);
  VsDisplay.clear();
  VsDisplay.setCursor(1,6);
  if(waitMinutes < 10) VsDisplay.print("0");
  VsDisplay.print(waitMinutes);
  VsDisplay.writeDisplay();
}

void UpdateWaitMessage()
{
  if((millis() - LastScrollTime) > SCROLL_DELAY)
  {
    CurrentScroll--;
    
    int scrollLimit = strlen(Message) * 6;
  
    if(CurrentScroll < -scrollLimit)
    {     
      CurrentScroll = 17;
    }

    MtlDisplay.setFont(NULL);
    MtlDisplay.setTextSize(1);
    MtlDisplay.setTextWrap(false);
    MtlDisplay.setTextColor(LED_ON);
    MtlDisplay.clear();
    MtlDisplay.setCursor(CurrentScroll,1);
    MtlDisplay.print(Message);      
    MtlDisplay.writeDisplay();

    VsDisplay.setFont(NULL);
    VsDisplay.setTextSize(1);
    VsDisplay.setTextWrap(false);
    VsDisplay.setTextColor(LED_ON);
    VsDisplay.clear();
    VsDisplay.setCursor(CurrentScroll - 8,1);
    VsDisplay.print(Message);    
    VsDisplay.writeDisplay();
  
    LastScrollTime = millis();
  }
}

void PlayMtlWin()
{
  for(int j = 0; j < 2; ++j)
  {
    for(int i = 0; i < MTL_WIN_SONG_SIZE; ++i)
    {
      ledcWriteTone(BUZZER_CHANNEL, MTL_WIN_SONG[i][0]);
      delay(MTL_WIN_SONG[i][1]);
    }

    if(j == 0)
    {
      ledcWriteTone(BUZZER_CHANNEL, 0);
      delay(300);
    }
  }

  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void PlayVsWin()
{
  for(int i = 0; i <VS_WIN_SONG_SIZE; ++i)
  {
    ledcWriteTone(BUZZER_CHANNEL, VS_WIN_SONG[i][0]);
    delay(VS_WIN_SONG[i][1]);
  }

  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void PlayMtlGoal()
{
  for(int i = 0; i < MTL_GOAL_SONG_SIZE; ++i)
  {
    ledcWriteTone(BUZZER_CHANNEL, MTL_GOAL_SONG[i][0]);
    delay(MTL_GOAL_SONG[i][1]);
  }

  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void PlayVsGoal()
{
  for(int i = 0; i < VS_GOAL_SONG_SIZE; ++i)
  {
    ledcWriteTone(BUZZER_CHANNEL, VS_GOAL_SONG[i][0]);
    delay(VS_GOAL_SONG[i][1]);
  }

  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void PlayStartGame()
{
  for(int i = 0; i < GAMESTART_SONG_SIZE; ++i)
  {
    ledcWriteTone(BUZZER_CHANNEL, GAMESTART_SONG[i][0]);
    delay(GAMESTART_SONG[i][1]);
  }

  ledcWriteTone(BUZZER_CHANNEL, 0);
}

void PlayStartPeriod()
{
  PlayStartGame();
  delay(600);
  PlayStartGame();
}

void PlayMtlGoalNote(int noteIndex)//Async: call this once with 0 to start song
{
  CurrentMtlGoalNote = noteIndex;
  
  if(CurrentMtlGoalNote >= MTL_GOAL_SONG_SIZE)
  {
    ledcWriteTone(BUZZER_CHANNEL, 0);
    return;
  }
  
  ledcWriteTone(BUZZER_CHANNEL, MTL_GOAL_SONG[CurrentMtlGoalNote][0]);
  LastMtlGoalNoteTime = millis();
}

void PlayMtlGoalUpdate()//Async: call this every frame
{
  if(CurrentMtlGoalNote >= MTL_GOAL_SONG_SIZE)
  {
    return;
  }
  
  if(millis() - LastMtlGoalNoteTime > MTL_GOAL_SONG[CurrentMtlGoalNote][1])
  {
    PlayMtlGoalNote(CurrentMtlGoalNote++);
  }
}

void PlayVsGoalNote(int noteIndex)//Async: call this once with 0 to start song
{
  CurrentVsGoalNote = noteIndex;
  
  if(CurrentVsGoalNote >= VS_GOAL_SONG_SIZE)
  {
    ledcWriteTone(BUZZER_CHANNEL, 0);
    return;
  }
  
  ledcWriteTone(BUZZER_CHANNEL,VS_GOAL_SONG[CurrentVsGoalNote][0]);
  LastVsGoalNoteTime = millis();
}

void PlayVsGoalUpdate()//Async: call this every frame
{
  if(CurrentVsGoalNote >= VS_GOAL_SONG_SIZE)
  {
    return;
  }
  
  if(millis() - LastVsGoalNoteTime > VS_GOAL_SONG[CurrentVsGoalNote][1])
  {
    PlayVsGoalNote(CurrentVsGoalNote++);
  }
}

void loop()
{
  switch(CurrentState)
  {
    case Configuring:
    {
      HandleClient();

      if(LastTeamChangeTime == 0)
      {
        if(strlen(Message) > 0)
          UpdateWaitMessage();
      }
      else if((millis() - LastTeamChangeTime) >= 5000)
      {
        LastTeamChangeTime = 0;
        TEAM = TeamsIds[CurrentTeamIndex];
        WriteTeamId(TEAM);

        TurnOffDisplays();
        
        sprintf(Message,"Configuration\0");
      }
              
      if(CanNext && (digitalRead(NEXT_TEAM_PIN) == LOW) && ((millis() - LastTeamChangeTime) > 250))
      {
        CurrentTeamIndex++;

        if(CurrentTeamIndex >= 31)
          CurrentTeamIndex = 0;
          
        LastTeamChangeTime = millis();
        CanNext = false;
        DisplayTeamAbv();
        
      }
      else if(CanPrev && (digitalRead(PREVIOUS_TEAM_PIN) == LOW) && ((millis() - LastTeamChangeTime) > 250))
      {
        CurrentTeamIndex--;

        if(CurrentTeamIndex < 0)
          CurrentTeamIndex = 30;
          
        LastTeamChangeTime = millis();
        CanPrev = false;
        DisplayTeamAbv();
        
      }

      if(!CanNext && digitalRead(NEXT_TEAM_PIN) == HIGH)
      {
        CanNext = true;
        delay(100);
      }

       if(!CanPrev && digitalRead(PREVIOUS_TEAM_PIN) == HIGH)
       {
          CanPrev = true;
       }
      
      break;
    }
    
    case Initialization:
    {
      if(error != 0 && (millis() - LastGetDataMillis > ErrorUpdateDelay))
      {
        GetData();

        if(error != 0)
        {
          sprintf(Message,"Erreur %i", error);
        }
      }

      if(strlen(Message) > 0)
        UpdateWaitMessage();

      if(error != 0)
      {
        break;
      }
  
      if(waitTime == 0)
      {
        SetState(InGame);
      }
      else
      {
        SetState(PreGame);
      }
      break;
    }

    case PreGame:
    {
      if((millis() < LastGetDataMillis) || (millis() - LastGetDataMillis) > ONE_MINUTE)
      {
        GetData();
        if(error != 0)
        {
          SetState(Initialization);
        }
        else 
        {
          UpdateRemainingTime();
          
          if(waitTime == 0)
          {
            SetState(InGame);
            break;
          }
        }
      }

      if(strlen(Message) > 0)
        UpdateWaitMessage();        

       UpdateWaitAnimation();

      break;
    }

    case InGame:
    {
      if((millis() - LastGetDataMillis) > LiveUpdateDelay)
      {
        GetData();
      }

      if(error != 0)
      {
        SetState(Initialization);
      }
      else
      {
        if(waitTime == 0)
        {
          UpdateVisual();
        }
        else if((millis() - LastGetDataMillis) >= (waitTime - ONE_HOUR))
        {
          SetState(PreGame);
        }
        else
        {
          SetState(PostGame);
        }
      }

      break;
    }

    case PostGame:
    {
      delay(ONE_HOUR);//1 hour
      SetState(PreGame);
      break;
    }
  }
}

bool IsConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void Connect()
{
  while(WiFi.status() != WL_CONNECTED)
  {
    if(DEBUG_PRINT) Serial.print("Attempting to connect to SSID: ");
    if(DEBUG_PRINT) Serial.println(ssid);

    DisplayTeamAbv();
  
    WiFi.disconnect(false);
    WiFi.begin(ssid.c_str(), password.c_str());
  
    //TODO: do in loop so we can display a message or use yield?
    for(int i = 0; i < 6; ++i)
    {
      if(DEBUG_PRINT) Serial.print(".");
      // wait 1 second for re-trying
      delayFor(1000);

      if(WiFi.status() == WL_CONNECTED)
      {
        if(DEBUG_PRINT) Serial.println("");
        if(DEBUG_PRINT) Serial.print("Connected to ");
        if(DEBUG_PRINT) Serial.println(ssid);
        
        break;
      }
      else
      {
        if(i%2 == 0)
        {
          PeriodDisplay.clear();
          PeriodDisplay.drawLine(3,0, 3,7, LED_ON);
          PeriodDisplay.drawLine(4,0, 4,7, LED_ON);
          PeriodDisplay.writeDisplay();
        }
        else
        {
          PeriodDisplay.clear();
          PeriodDisplay.drawLine(0,3, 7,3, LED_ON);
          PeriodDisplay.drawLine(0,4, 7,4, LED_ON);
          PeriodDisplay.writeDisplay();
        }
      }
    }
  }
}

bool GetData()
{
  LastGetDataMillis = millis();

  if(!IsConnected())
  {
    error = 22;
    Serial.println("Aucune connection reseau");
    sprintf(Message,"Aucune connection reseau\0");
    Connect();
    Message[0] = '\0';
  }
 
  HTTPClient http;

  String url = URL + String(TEAM);
  http.begin(url, root_ca);
  int httpCode = http.GET();

  String response;

  if (httpCode == 200)
  {
      response = http.getString();
      //if(DEBUG_PRINT) Serial.println(response);
  }
  else
  {
    error = 44;
    if(DEBUG_PRINT) Serial.println("Erreur serveur");
    if(DEBUG_PRINT) Serial.println(httpCode);
    sprintf(Message,"Erreur serveur\0");
    return false;
  }

  http.end(); //Free the resources
    
  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = 150;//90 + buffer
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError deserializeError = deserializeJson(doc, response);

 
  if (deserializeError)
  {
    error = 33;
    if(DEBUG_PRINT) Serial.print(F("deserializeJson() failed: "));
    if(DEBUG_PRINT) Serial.println(deserializeError.c_str());
    sprintf(Message,"Erreur de format de fichier\0");
    return false;
  }

  int previousError = error;
  PreviousMtl = mtl;
  PreviousVs = vs;

  error = doc["e"];
  
  if(error == 0)
  {
    waitTime = doc["t"];
    mtl = doc["m"];
    vs = doc["v"];

    //Period
    int newPeriod = doc["p"];
    
    if(newPeriod > period)
    {
      PlayStartPeriod();
    }

    period = newPeriod;
    
    ErrorCount = 0;
     
    if(previousError != 0)
    {
      Message[0] = '\0';
    }
  }

  if(mtl > PreviousMtl)
  {
    PlayMtlGoal();
  }

  if(vs > PreviousVs)
  {
    PlayVsGoal();
  }

  if(error != 0)
  {
    ErrorCount++;

    if(ErrorCount > 3)
    {
      Serial.println("Displaying error");
      
      if(error == 999)
      {
        sprintf(Message,"Prochain match non planifie - %d\0", error);
      }
      else if(error == 484)
      {
        sprintf(Message,"Match reporte - %d\0", error);
      }
      else
      {
        sprintf(Message,"Erreur %d\0", error);
      }
    }
    else if(DEBUG_PRINT) 
    {
      Serial.print("ErrorCount: ");
      Serial.print(ErrorCount);
    }
  }

  //TODO: read message in Message

  if(DEBUG_PRINT) 
  {
    Serial.print("Error: ");
    Serial.print(error);
    Serial.print(" WaitTime: ");
    Serial.print(waitTime);
    Serial.print(" Period: ");
    Serial.print(period);
    Serial.print(" MTL: ");
    Serial.print(mtl);
    Serial.print(" VS: ");
    Serial.println(vs);
  }

  LastGetDataMillis = millis();

  return true;
}

void TurnOffDisplays()
{
  PeriodDisplay.clear();
  MtlDisplay.clear();
  VsDisplay.clear();

  PeriodDisplay.writeDisplay();
  MtlDisplay.writeDisplay();
  VsDisplay.writeDisplay();
}

void UpdateVisual()
{
  PeriodDisplay.clear();
  MtlDisplay.clear();
  VsDisplay.clear();

  PeriodDisplay.setFont(NULL);
  PeriodDisplay.setCursor(1,1);
  PeriodDisplay.setTextSize(1);
  PeriodDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  PeriodDisplay.setTextColor(LED_ON);
  PeriodDisplay.print(period, DEC);
  PeriodDisplay.writeDisplay();

  MtlDisplay.setTextSize(1);
  MtlDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  MtlDisplay.setTextColor(LED_ON);
  
  if(mtl < 10)
  {
    MtlDisplay.setFont(NULL);
    MtlDisplay.setCursor(1,1);
  }
  else
  {
    MtlDisplay.setFont(&Picopixel);
    MtlDisplay.setCursor(0,6);
  }

  MtlDisplay.print(mtl, DEC);
  MtlDisplay.writeDisplay();

  VsDisplay.setTextSize(1);
  VsDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  VsDisplay.setTextColor(LED_ON);

  if(vs < 10)
  {
    VsDisplay.setFont(NULL);
    VsDisplay.setCursor(1,1);
  }
  else
  {
    VsDisplay.setFont(&Picopixel);
    VsDisplay.setCursor(0,6);
  }
  
  VsDisplay.print(vs, DEC);
  VsDisplay.writeDisplay();
}

void delayFor(long milliseconds)
{
    long now = millis();

    while (millis() - now < milliseconds)
    {
        yield();
    }
}

bool ReadCredentials(String& ssid, String& psk)
{
  ssid = EEPROM.readString(0);
  psk = EEPROM.readString(32);

  if(ssid.length() > 0 && psk.length() > 0)
  {
    if(DEBUG_PRINT)
    {
      Serial.print("SSID: ");
      Serial.println(ssid);
      
      Serial.print("PSK: ");
      Serial.println(psk);
    }

    return true;
  }

  Serial.println("No credentials");
  
  return false;
}

int ReadTeamId()
{
  int teamid = EEPROM.read(64);

  if(teamid == 255)
    teamid = 8; //MTL by default

  for(int i = 0; i < 31; ++i)
  {
    if(TeamsIds[i] == teamid)
    {
      CurrentTeamIndex = i;
    }
  }

  return teamid;
}

void WriteTeamId(int teamId)
{
    EEPROM.write(64, teamId);
    EEPROM.commit();
}

String GetGetParam(String prefix, String suffix)
{
  int startIndex = header.indexOf(prefix);

  if (startIndex >= 0)
  {
    int endIndex = header.indexOf(suffix);

    if(endIndex > startIndex)
    {
      return header.substring(startIndex+prefix.length(), endIndex);
    }
  }

  return "";
}

void HandleClient()
{
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client)
  {
    String currentLine = "";
    
    while (client.connected())
    {
      if(strlen(Message) > 0)
        UpdateWaitMessage();

      if (client.available())
      {             
        char c = client.read();
        
        header += c;
        if (c == '\n') 
        {    
          if (currentLine.length() == 0)
          {
            if(header.indexOf("GET /config.php") >= 0)
            {
              String teamId = GetGetParam("team=", "&ssid");

              if(teamId.length() > 0)
              {
                int newTeam = teamId.toInt();
                WriteTeamId(newTeam);
              }
              
              String newssid = GetGetParam("&ssid=", "&psk");
  
              String newpsk = GetGetParam("&psk=", "&action");

              if(newssid.length() > 0 && newpsk.length() > 0)
              {
                SetCredentials(newssid, newpsk);
              }

              ESP.restart();
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                
            //Web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("</style></head>");

            client.println("<body><h1>Configuration MTL Score</h1>");

             client.println("<form method=\"GET\" action=\"config.php\" id=\"config\" />");

            client.println("Equipe: <select id=\"team\" name=\"team\" form=\"config\" />" \
              "<option value=\"1\" />New Jersey Devils</option />" \
              "<option value=\"2\" />New York Islanders</option />" \
              "<option value=\"3\" />New York Rangers</option />" \
              "<option value=\"4\" />Philadelphia Flyers</option />" \
              "<option value=\"5\" />Pittsburgh Penguins</option />" \
              "<option value=\"6\" />Boston Bruins</option />" \
              "<option value=\"7\" />Buffalo Sabres</option />" \
              "<option value=\"8\" />Montreal Canadiens</option />" \
              "<option value=\"9\" />Ottawa Senators</option />" \
              "<option value=\"10\" />Toronto Maple Leafs</option />" \
              "<option value=\"12\" />Carolina Hurricanes</option />" \
              "<option value=\"13\" />Florida Panthers</option />" \
              "<option value=\"14\" />Tampa Bay Lightning</option />" \
              "<option value=\"15\" />Washington Capitals</option />" \
              "<option value=\"16\" />Chicago Blackhawks</option />" \
              "<option value=\"17\" />Detroit Red Wings</option />" \
              "<option value=\"18\" />Nashville Predators</option />" \
              "<option value=\"19\" />St Louis Blues</option />" \
              "<option value=\"20\" />Calgary Flames</option />" \
              "<option value=\"21\" />Colorado Avalanche</option />" \
              "<option value=\"22\" />Edmonton Oilers</option />" \
              "<option value=\"23\" />Vancouver Canucks</option />" \
              "<option value=\"24\" />Anaheim Ducks</option />" \
              "<option value=\"25\" />Dallas Stars</option />" \
              "<option value=\"26\" />Los Angeles Kings</option />" \
              "<option value=\"28\" />San Jose Sharks</option />" \
              "<option value=\"29\" />Columbus Blue Jackets </option />" \
              "<option value=\"30\" />Minnesota Wild</option />" \
              "<option value=\"52\" />Winnipeg Jets</option />" \
              "<option value=\"53\" />Arizona Coyotes</option />" \
              "<option value=\"54\" />Vegas Golden Knights</option />" \
              "</select /><br />");
            client.println("Nom du reseau: <input type=\"text\" name=\"ssid\" /> <br />");
            client.println("Mot de passe: <input type=\"text\" name=\"psk\" /> <br /> <br />");
            
            client.println("<input type=\"submit\" name=\"action\" value=\"Configurer\" />");
            
            client.println("</form>");
            
            client.println("</body></html>");
            
            client.println();

            break;
          } 
          else 
          {
            currentLine = "";
          }
        } 
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }

    header = "";

    client.stop();
  }
}

void SetCredentials(String new_ssid, String new_psk)
{
    char ssid_c[32];
    
    sprintf(ssid_c, new_ssid.c_str());
    EEPROM.writeString(0, ssid_c);

    char psk_c[32];
    sprintf(psk_c, new_psk.c_str());
    EEPROM.writeString(32, psk_c);
    EEPROM.commit();
}

String GetNameFromTeamId(int id)
{
  switch(id)
  {
    case 1:
    {
      return "NJD";
    }
    case 2:
    {
      return "NYI";
    }
    case 3:
    {
      return "NYR";
    }
    case 4: 
    {
      return "PHI";
    }
    case 5:  
    {
      return "PIT";
    }
    case 6:  
    {
      return "BOS";
    }
    case 7:  
    {
      return "BUF";
    }
    case 8:  
    {
      return "MTL";
    }
    case 9:  
    {
      return "OTT";
    }
    case 10:  
    {
      return "TOR";
    }
    case 12:  
    {
      return "CAR";
    }
    case 13:  
    {
      return "FLA";
    }
    case 14:  
    {
      return "TBL";
    }
    case 15:  
    {
      return "WSH";
    }
    case 16:  
    {
      return "CHI";
    }
    case 17:  
    {
      return "DET";
    }
    case 18:  
    {
      return "NSH";
    }
    case 19:  
    {
      return "STL";
    }
    case 20:  
    {
      return "CGY";
    }
    case 21:  
    {
      return "COL";
    }
    case 22:  
    {
      return "EDM";
    }
    case 23:  
    {
      return "VAN";
    }
    case 24:  
    {
      return "ANA";
    }
    case 25:  
    {
      return "DAL";
    }
    case 26:  
    {
      return "LAK";
    }
    case 28:  
    {
      return "SJS";
    }
    case 29:  
    {
      return "CBJ";
    }
    case 30:  
    {
      return "MIN";
    }
    case 52:  
    {
      return "WPG";
    }
    case 53:  
    {
      return "ARI";
    }
    case 54:
    {
      return "VGK";
    }
  }

  return "ERR";
}

void DisplayTeamAbv()
{
  String teamAbv = GetNameFromTeamId(TeamsIds[CurrentTeamIndex]);
  
  PeriodDisplay.clear();
  PeriodDisplay.setCursor(1,1);
  PeriodDisplay.setTextSize(1);
  PeriodDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  PeriodDisplay.setTextColor(LED_ON);
  PeriodDisplay.print(teamAbv[0]);
  PeriodDisplay.writeDisplay();

  MtlDisplay.clear();
  MtlDisplay.setCursor(1,1);
  MtlDisplay.setTextSize(1);
  MtlDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  MtlDisplay.setTextColor(LED_ON);
  MtlDisplay.print(teamAbv[1]);
  MtlDisplay.writeDisplay();

  VsDisplay.clear();
  VsDisplay.setCursor(1,1);
  VsDisplay.setTextSize(1);
  VsDisplay.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  VsDisplay.setTextColor(LED_ON);
  VsDisplay.print(teamAbv[2]);
  VsDisplay.writeDisplay();
}
