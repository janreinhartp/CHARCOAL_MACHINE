#include <Arduino.h>
#include "control.h"
#include <EEPROMex.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
byte enterChar[] = {
    B10000,
    B10000,
    B10100,
    B10110,
    B11111,
    B00110,
    B00100,
    B00000};

byte fastChar[] = {
    B00110,
    B01110,
    B00110,
    B00110,
    B01111,
    B00000,
    B00100,
    B01110};
byte slowChar[] = {
    B00011,
    B00111,
    B00011,
    B11011,
    B11011,
    B00000,
    B00100,
    B01110};

static const int buttonPin = 2;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 4;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 3;
int buttonStatePrevious3 = HIGH;

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

unsigned long previousMillisTest;
const int intervalTest = 1000;
unsigned long currentMillisTest;

// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 3;
const int NUM_SETTING_ITEMS = 5;
const int NUM_TESTMACHINE_ITEMS = 4;
const int MAX_ITEM_LENGTH = 20; // maximum characters for the item name

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool settingFlag, settingEditFlag, testMenuFlag, TestMachineFlag = false;
bool RunAutoFlag = false;
int runAutoStatus = 0;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"RUN AUTO", "ENTER TO RUN AUTO"},
    {"TEST MACHINE", "ENTER TO TEST"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"FORWARD", "SEC"},
    {"REVERSE", "SEC"},
    {"INTERVAL", "SEC"},
    {"COOKING TIME", "MIN"},
    {"SAVE", "Test"}};

String testmachine_items[NUM_TESTMACHINE_ITEMS][2] = { // array with item names
    {"FORWARD", "MIXER"},
    {"REVERSE", "MIXER"},
    {"BUZZER", "MIXER"},
    {"EXIT", " "}};

int parametersTimer[NUM_SETTING_ITEMS] = {1, 1, 1, 1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {60, 60, 300, 600};

bool refreshScreen = true;
unsigned long previousMillis = 0;
const long interval = 1000;

int FWDTIMEADD = 20;
int RWDTIMEADD = 30;
int INTTIMEADD = 40;
int COOKTIMEADD = 50;

Control FORWARD(A0, 100, 100);
Control REVERSE(A1, 100, 100);
Control BUZZER(A2, 100, 100);
Control INTERVALFWDRWD(100, 100, 100);
Control COOKING(100, 100, 100);

char *secondsToHHMMSS(int total_seconds)
{
  int hours, minutes, seconds;

  hours = total_seconds / 3600;         // Divide by number of seconds in an hour
  total_seconds = total_seconds % 3600; // Get the remaining seconds
  minutes = total_seconds / 60;         // Divide by number of seconds in a minute
  seconds = total_seconds % 60;         // Get the remaining seconds

  // Format the output string
  static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
  sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
  return hhmmss_str;
}

void saveSettings()
{
  EEPROM.writeInt(FWDTIMEADD, parametersTimer[0]);
  EEPROM.writeInt(RWDTIMEADD, parametersTimer[1]);
  EEPROM.writeInt(INTTIMEADD, parametersTimer[2]);
  EEPROM.writeInt(COOKTIMEADD, parametersTimer[3]);
}

void loadSettings()
{
  parametersTimer[0] = EEPROM.readInt(FWDTIMEADD);
  parametersTimer[1] = EEPROM.readInt(RWDTIMEADD);
  parametersTimer[2] = EEPROM.readInt(INTTIMEADD);
  parametersTimer[3] = EEPROM.readInt(COOKTIMEADD);
}

void setTimers()
{
  FORWARD.setTimer(secondsToHHMMSS(parametersTimer[0]));
  REVERSE.setTimer(secondsToHHMMSS(parametersTimer[1]));
  INTERVALFWDRWD.setTimer(secondsToHHMMSS(parametersTimer[2]));
  COOKING.setTimer(secondsToHHMMSS(parametersTimer[3] * 60));
  BUZZER.setTimer(secondsToHHMMSS(30));
}

void stopAll()
{
  RunAutoFlag = false;
  testMenuFlag = false;
  TestMachineFlag = false;
  runAutoStatus = 0;
  FORWARD.stop();
  REVERSE.stop();
  BUZZER.stop();
  INTERVALFWDRWD.stop();
  COOKING.stop();
}

void RunAuto()
{
  COOKING.run();
  if (COOKING.isTimerCompleted() == true)
  {
    BUZZER.relayOn();
  }
  else
  {
    switch (runAutoStatus)
    {
    case 1:
      FORWARD.run();
      if (FORWARD.isTimerCompleted() == true)
      {
        INTERVALFWDRWD.start();
        runAutoStatus = 2;
      }
      break;
    case 2:
      INTERVALFWDRWD.run();
      if (INTERVALFWDRWD.isTimerCompleted() == true)
      {
        REVERSE.start();
        runAutoStatus = 3;
      }
      break;
    case 3:
      REVERSE.run();
      if (REVERSE.isTimerCompleted() == true)
      {
        INTERVALFWDRWD.start();
        runAutoStatus = 4;
      }
      break;
    case 4:
      INTERVALFWDRWD.run();
      if (INTERVALFWDRWD.isTimerCompleted() == true)
      {
        FORWARD.start();
        runAutoStatus = 1;
      }
      break;
    default:
      break;
    }
  }
}

void RunTestMachine()
{
  Serial.println("Test Run");
  FORWARD.run();
  REVERSE.run();
  BUZZER.run();
}

// Function for reading the button state
void readButtonUpState()
{
  if (currentMillis - previousButtonMillis > intervalButton)
  {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
    {
      buttonLongPressUpMillis = currentMillis;
      buttonStatePrevious = LOW;
    }
    buttonPressDuration = currentMillis - buttonLongPressUpMillis;
    if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
    {
      buttonStateLongPressUp = true;
    }
    if (buttonStateLongPressUp == true)
    {
      // Insert Fast Scroll Up
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
          {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
          }
          else
          {
            parametersTimer[currentSettingScreen] += 1;
          }
        }
        else
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            currentSettingScreen = 0;
          }
          else
          {
            currentSettingScreen++;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
        {
          currentTestMenuScreen = 0;
        }
        else
        {
          currentTestMenuScreen++;
        }
      }
      else
      {
        if (currentMainScreen == NUM_MAIN_ITEMS - 1)
        {
          currentMainScreen = 0;
        }
        else
        {
          currentMainScreen++;
        }
      }
      refreshScreen = true;
    }

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
            {
              parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
            }
            else
            {
              parametersTimer[currentSettingScreen] += 1;
            }
          }
          else
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              currentSettingScreen = 0;
            }
            else
            {
              currentSettingScreen++;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentTestMenuScreen = 0;
          }
          else
          {
            currentTestMenuScreen++;
          }
        }
        else
        {
          if (currentMainScreen == NUM_MAIN_ITEMS - 1)
          {
            currentMainScreen = 0;
          }
          else
          {
            currentMainScreen++;
          }
        }
        refreshScreen = true;
      }
    }
    previousButtonMillis = currentMillis;
  }
}

void readButtonDownState()
{
  if (currentMillis2 - previousButtonMillis2 > intervalButton2)
  {
    int buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
    {
      buttonLongPressDownMillis = currentMillis2;
      buttonStatePrevious2 = LOW;
    }
    buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
    if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
    {
      buttonStateLongPressDown = true;
    }
    if (buttonStateLongPressDown == true)
    {
      Serial.println("test Fast");
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] <= 0)
          {
            parametersTimer[currentSettingScreen] = 0;
          }
          else
          {
            parametersTimer[currentSettingScreen] -= 1;
          }
        }
        else
        {
          if (currentSettingScreen == 0)
          {
            currentSettingScreen = NUM_SETTING_ITEMS - 1;
          }
          else
          {
            currentSettingScreen--;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == 0)
        {
          currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
        }
        else
        {
          currentTestMenuScreen--;
        }
      }
      else
      {
        if (currentMainScreen == 0)
        {
          currentMainScreen = NUM_MAIN_ITEMS - 1;
        }
        else
        {
          currentMainScreen--;
        }
      }
      refreshScreen = true;
    }

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        Serial.println("test Slow");
        // Short Scroll Down
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] <= 0)
            {
              parametersTimer[currentSettingScreen] = 0;
            }
            else
            {
              parametersTimer[currentSettingScreen] -= 1;
            }
          }
          else
          {
            if (currentSettingScreen == 0)
            {
              currentSettingScreen = NUM_SETTING_ITEMS - 1;
            }
            else
            {
              currentSettingScreen--;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == 0)
          {
            currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
          }
          else
          {
            currentTestMenuScreen--;
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            currentMainScreen = NUM_MAIN_ITEMS - 1;
          }
          else
          {
            currentMainScreen--;
          }
        }
        refreshScreen = true;
      }
    }
    previousButtonMillis2 = currentMillis2;
  }
}

void readButtonEnterState()
{
  if (currentMillis3 - previousButtonMillis3 > intervalButton3)
  {
    int buttonState3 = digitalRead(buttonPin3);
    if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
    {
      buttonLongPressEnterMillis = currentMillis3;
      buttonStatePrevious3 = LOW;
    }
    buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
    if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
    {
      buttonStateLongPressEnter = true;
    }
    if (buttonStateLongPressEnter == true)
    {
      // Insert Fast Scroll Enter
      Serial.println("Long Press Enter");
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        // Short Scroll Enter
        if (currentMainScreen == 0 && settingFlag == true)
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            settingFlag = false;
            saveSettings();
            loadSettings();
            currentSettingScreen = 0;
            setTimers();
          }
          else
          {
            if (settingEditFlag == true)
            {
              settingEditFlag = false;
            }
            else
            {
              settingEditFlag = true;
            }
          }
        }
        else if (currentMainScreen == 1 && RunAutoFlag == true)
        {
          RunAutoFlag = false;
          runAutoStatus = 0;
          stopAll();
        }
        else if (currentMainScreen == 2 && testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentMainScreen = 0;
            currentTestMenuScreen = 0;
            testMenuFlag = false;
            TestMachineFlag = false;
            stopAll();
          }
          else if (currentTestMenuScreen == 0)
          {
            if (FORWARD.isTimerCompleted() == true)
            {
              if (REVERSE.isTimerCompleted() == false)
              {
                REVERSE.stop();
              }
              FORWARD.start();
            }
            else
            {
              FORWARD.stop();
            }
          }
          else if (currentTestMenuScreen == 1)
          {
            if (REVERSE.isTimerCompleted() == true)
            {
              if (FORWARD.isTimerCompleted() == false)
              {
                FORWARD.stop();
              }
              REVERSE.start();
            }
            else
            {
              REVERSE.stop();
            }
          }
          else if (currentTestMenuScreen == 2)
          {
            if (BUZZER.isTimerCompleted() == true)
            {
              BUZZER.start();
            }
            else
            {
              BUZZER.stop();
            }
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            settingFlag = true;
          }
          else if (currentMainScreen == 1)
          {
            RunAutoFlag = true;
            runAutoStatus = 1;
            COOKING.start();
            FORWARD.start();
            lcd.clear();
          }
          else if (currentMainScreen == 2)
          {
            testMenuFlag = true;
            TestMachineFlag = true;
          }
        }
      }
    }
    previousButtonMillis3 = currentMillis3;
  }
}

void InputReadandFeedback()
{
  currentMillis = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  readButtonEnterState();
  readButtonUpState();
  readButtonDownState();
}

void setupLCD()
{
  lcd.init();
  lcd.clear();
  lcd.createChar(0, enterChar);
  lcd.createChar(1, fastChar);
  lcd.createChar(2, slowChar);
  lcd.backlight();
  refreshScreen = true;
}

void printMainMenu(String MenuItem, String Action)
{
  lcd.clear();
  lcd.print(MenuItem);
  lcd.setCursor(0, 3);
  lcd.write(0);
  lcd.setCursor(2, 3);
  lcd.print(Action);
  refreshScreen = false;
}

void printRunAutoScreen(String TimeRemain, String Process, String TimeRemaining)
{
  lcd.setCursor(0,0);
  lcd.print("CHARCOAL MACHINE");
  lcd.setCursor(0,1);
  lcd.print("TIME: ");
  lcd.print(TimeRemain);
  lcd.setCursor(0, 2);
  lcd.print(Process);
  lcd.setCursor(0, 3);
  lcd.print(TimeRemaining);
  refreshScreen = false;
}
void printRunAutoScreenUp(String SettingTitle, String Process, String TimeRemaining)
{
  lcd.clear();
  lcd.print(Process);
  lcd.setCursor(0, 1);
  lcd.print(TimeRemaining);
  refreshScreen = false;
}

void printSettingScreen(String SettingTitle, String Unit, double Value, bool EditFlag, bool SaveFlag)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);

  if (SaveFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print("ENTER TO SAVE ALL");
  }
  else
  {
    lcd.print(Value);
    lcd.print(" ");
    lcd.print(Unit);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    if (EditFlag == false)
    {
      lcd.print("ENTER TO EDIT");
    }
    else
    {
      lcd.print("ENTER TO SAVE");
    }
  }
  refreshScreen = false;
}

void printTestScreen(String TestMenuTitle, String Job, bool Status, bool ExitFlag)
{
  lcd.clear();
  lcd.print(TestMenuTitle);
  if (ExitFlag == false)
  {
    lcd.setCursor(0, 2);
    lcd.print(Job);
    lcd.print(" : ");
    if (Status == true)
    {
      lcd.print("ON");
    }
    else
    {
      lcd.print("OFF");
    }
  }

  if (ExitFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Exit Test");
  }
  else
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Run Test");
  }
  refreshScreen = false;
}

// void printRunAutoScreen(String SettingTitle, String Process, String TimeRemaining)
// {
//   lcd.setCursor(0, 2);
//   lcd.print(Process);
//   lcd.setCursor(0, 3);
//   lcd.print(TimeRemaining);
//   refreshScreen = false;
// }
void printScreen()
{
  if (settingFlag == true)
  {
    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
    }
    else
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
    }
  }
  else if (RunAutoFlag == true)
  {
    if (COOKING.isTimerCompleted() == false)
    {
      switch (runAutoStatus)
      {
      case 1:
        printRunAutoScreen(COOKING.getTimeRemaining(), "MIXER: FORWARD    ", FORWARD.getTimeRemaining());
        break;
      case 2:
        printRunAutoScreen(COOKING.getTimeRemaining(), "MIXER: OFF        ", INTERVALFWDRWD.getTimeRemaining());
        break;
      case 3:
        printRunAutoScreen(COOKING.getTimeRemaining(), "MIXER: REVERSE    ", REVERSE.getTimeRemaining());
        break;
      case 4:
        printRunAutoScreen(COOKING.getTimeRemaining(), "MIXER: OFF        ", INTERVALFWDRWD.getTimeRemaining());
        break;
      default:
        break;
      }
    }else{
      printRunAutoScreen("00:00:00", "FINISH COOKING", "00:00:00");
    }
  }
  else if (testMenuFlag == true)
  {
    switch (currentTestMenuScreen)
    {
    case 0:
      printTestScreen(testmachine_items[currentTestMenuScreen][0], FORWARD.getTimeRemaining(), !FORWARD.isTimerCompleted(), false);
      break;
    case 1:
      printTestScreen(testmachine_items[currentTestMenuScreen][0], REVERSE.getTimeRemaining(), !REVERSE.isTimerCompleted(), false);
      break;
    case 2:
      printTestScreen(testmachine_items[currentTestMenuScreen][0], BUZZER.getTimeRemaining(), !BUZZER.isTimerCompleted(), false);
      break;
    case 3:
      printTestScreen(testmachine_items[currentTestMenuScreen][0], " ", true, true);
      break;

    default:
      break;
    }
  }
  else
  {
    printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
  }
}

void setup()
{
  // saveSettings();
  Serial.begin(9600);
  setupLCD();
  loadSettings();
  setTimers();
  refreshScreen = true;
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  stopAll();
}

void loop()
{
  InputReadandFeedback();

  if (refreshScreen == true)
  {
    printScreen();
    refreshScreen = false;
  }

  if (TestMachineFlag == true)
  {
    RunTestMachine();
    currentMillisTest = millis();
    if (currentMillisTest - previousMillisTest >= intervalTest)
    {
      previousMillisTest = currentMillisTest;
      refreshScreen = true;
    }
  }

  if (RunAutoFlag == true)
  {
    RunAuto();
    currentMillisTest = millis();
    if (currentMillisTest - previousMillisTest >= intervalTest)
    {
      previousMillisTest = currentMillisTest;
      refreshScreen = true;
    }
  }
}