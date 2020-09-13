#include <EEPROM.h>

#define BUFFERSIZE 127

#define FIRST_RUN 0
#define READY 1
#define GET_INPUT 2
#define VALIDATION 3


long int timerArray[6];
long int minTimer = 99999, maxTimer = 0;
int cntTimer = 0;

uint8_t inBuffer[BUFFERSIZE];
int inLength;

int fsrReading;

const int fsr = 0;

const int redLED = 27;
const int yellowLED = 35;
const int greenLED = 43;

const int buzzer = 45;

const int button = 49;

int ante = 0;

char code[6] = " ";
int cntCode = 0, copyCntCode;
int cntChar = 0;

int timer = 0;
int finalTimer = 0;

char testCode[6] = " ";

bool pauseTime = false;
int timerPause = 0;
int timerFinal = 0;

int activated = 0;

int ok = 0;

boolean hasCode = false;
boolean hasFinished = false;

int STATE = FIRST_RUN;

static char checkBlue[2] = " ";
static char prevBlue = 0;

static char checkSystem[2] = " ";
static char prevCheckSystem = 0;

static char inputReset[2] = " ";

char temp[2] = " ";

char firstAdd = 2;

int validInputs = 0, invalidInputs = 0;

int sendConf = 0;

int checkBlueCnt = 0;


void setup()
{

  Serial.begin(9600);

  Serial1.begin(9600);

  pinMode(fsr, INPUT);

  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  pinMode(button, INPUT);

  pinMode(buzzer, OUTPUT);


  digitalWrite(redLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, LOW);

  noTone(buzzer);

  
  EEPROM_readCode(40, checkSystem);


  checkBlue[0] = 48;
  STATE = FIRST_RUN;

}

void loop()
{

  
  if (Serial1.available()) {
    inLength =  0;
    while (Serial1.available()) {
      delay(50);
      inBuffer[ inLength] = Serial1.read();
      inLength++;
      if (inLength >= BUFFERSIZE)
        break;
    }

    if (inLength == 1) {

      temp[0] = (inBuffer[0]);
      temp[1] = '\0';

      if(temp[0] == 48 || temp[0] == 49){
        checkBlue[0] = (inBuffer[0]);
        checkBlue[1] = '\0';
        checkBlueCnt = 0;
      }else if(temp[0] == 50 || temp[0] == 51){
        checkSystem[0] = (inBuffer[0]);
        checkSystem[1] = '\0';
      }else{
        inputReset[0] = (inBuffer[0]);
        inputReset[1] = '\0';
      }
     
    }
    else {
      for (int i = 0 ; i < inLength ; i++)
      {
        if (inLength == 4 || inLength == 5) {
          code[cntCode] = (inBuffer[i]);
          cntCode++;
          if (inLength == 4) {
            if (cntCode == 4) {
              code[cntCode] = '\0';
              hasCode = true;
            }
          } else if (inLength == 5) {
            if (cntCode == 5) {
              code[cntCode] = '\0';
              hasCode = true;
            }
          }
        }
      }
    }

  } else if (checkBlue[0] == 49) { //daca au trecut doua secunde de cand nu a mai fost seriala disponibila si eu am tot informatia ca bluetooth e conectat, inseamna ca am iesit brusc din aplicatie
    delay(50);
    checkBlueCnt += 50;

    if (checkBlueCnt > 2000) {
      checkBlue[0] = 48;
      checkBlueCnt = 0;

    }
  }

  if (hasCode == true) {

    copyCntCode = cntCode;
    writeString(10, code);
    STATE = READY;

    hasCode = false;
    cntCode = 0;
  }



  if (checkSystem[0] == 50) {

    if(prevCheckSystem != 1){
      STATE = FIRST_RUN;
      prevCheckSystem = 1;
      EEPROM_writeCode(40, 50);
    }

    int buttonState = digitalRead(button);

    if (buttonState == HIGH && ante == LOW) {
      STATE = READY;
    }

    if(inputReset[0] == 52){
      STATE = READY;
      memset(inputReset, 0, sizeof inputReset);
    }

    ante = buttonState;

    if (checkBlue[0] == 49 && prevBlue != 50) { //cazul in care tocmai s-a aprins bluetooth

      String inString;
      inString += validInputs;
      inString += ',';
      inString += invalidInputs;
      inString += ',';

      if (validInputs != 0 || invalidInputs != 0) {

        for (int i = 0; i < inString.length(); i++) {

          Serial1.print(inString[i]);


        }

        validInputs = 0;
        invalidInputs = 0;
        inString = " ";



        EEPROM_writeCode(20, validInputs);
        EEPROM_writeCode(30, invalidInputs);


      }

      prevBlue = 50;

    } else if (checkBlue[0] == 49 && sendConf != 0) { //cazul in care bluetooth e deja aprins si s-a trimis ceva


      if (sendConf == 1) {
        Serial1.print("one");
      } else if (sendConf == 2) {
        Serial1.print("zero");
      }
      sendConf = 0;
    } else if (checkBlue[0] == 48 && prevBlue == 50) { //cazul in care bluetooth a fost stins
      prevBlue = 51;
    }



    switch (STATE) {

      case FIRST_RUN:

        EEPROM_readCode(20, validInputs);

        EEPROM_readCode(30, invalidInputs);


        EEPROM.get( 10, firstAdd );

        if (firstAdd == 48 || firstAdd == 49 && copyCntCode == 0) {
          int len = 0;
          unsigned char k;
          k = EEPROM.read(10);
          while (k != '\0') {
            k = EEPROM.read(10 + len);
            code[len] = k;
            len++;
          }
          code[len] = '\0';
          len--;
          copyCntCode = len;
        }

        STATE = READY;

        break;



      case READY :

        digitalWrite(greenLED, HIGH);
        digitalWrite(yellowLED, LOW);
        digitalWrite(redLED, LOW);
        noTone(buzzer);
        cntTimer = 0;
        pauseTime = false;
        cntChar = 0;
        STATE = GET_INPUT;
        break;

      case GET_INPUT:

        if (pauseTime == false) {

          if (finalTimer > 50) {

            
            if (finalTimer > maxTimer) {
              maxTimer = finalTimer;
            }
            if (finalTimer < minTimer) {
              minTimer = finalTimer;
            }

            timerArray[cntTimer] = finalTimer;
            cntTimer++;
            finalTimer = 0;
            pauseTime = true;
          }

          if (cntChar == copyCntCode) {
            if (maxTimer < (minTimer * 2)) {
              memset(testCode, 0, sizeof testCode);
              maxTimer = 0;
              minTimer = 99999;
              cntTimer = 0;
              hasFinished = true;

              digitalWrite(yellowLED, LOW);
              STATE = VALIDATION;
              break;
            } else {
              for (int i = 0; i < cntChar; i++) {
                int shortTouch = minTimer - timerArray[i];
                if (timerArray[i] > minTimer) {
                  shortTouch *= -1;
                }
                int longTouch = maxTimer - timerArray[i];
                if (timerArray[i] > maxTimer) {
                  longTouch *= -1;
                }

                if (shortTouch < longTouch) {
                  testCode[i] = 48;
                } else if (longTouch < shortTouch) {
                  testCode[i] = 49;
                } else if (longTouch == shortTouch) {
                  testCode[i] = 49;
                }

              }

              maxTimer = 0;
              minTimer = 99999;
              cntTimer = 0;
              hasFinished = true;
              digitalWrite(yellowLED, LOW);
              STATE = VALIDATION;
            }
          }


        }

        fsrReading = analogRead(fsr);

        if (fsrReading > 200) {
          activated = 1;
          if (cntTimer == 0) {
              digitalWrite(greenLED, LOW);
              digitalWrite(yellowLED, HIGH);
            }

        } else {
          activated = 0;
        }


        if (activated == 1) {
          if (checkBlueCnt == 0)
            delay(50);
          timer += 50;
        } else {
          finalTimer = timer;
          timer = 0;
        }

        if (pauseTime == true) {
          if (checkBlueCnt == 0)
            delay(50);
          timerPause += 50;
          if (timerPause == 1000) {
            timerPause = 0;
            pauseTime = false;
            cntChar++;
          }
        }

        break;

      case VALIDATION:

        if (hasFinished == true) {
          for (int i = 0; i < strlen(code); i++) {
            if (code[i] == testCode[i]) {
              ok = 0;
            } else {
              ok = 1;
              break;
            }
          }
          if (ok == 0) {
            if (checkBlue[0] == 49) {
              sendConf = 1;
            } else if (checkBlue[0] == 48) {
              validInputs++;
              EEPROM_writeCode(20, validInputs);
            }
          } else {
            if (checkBlue[0] == 49) {
              sendConf = 2;
            } else if (checkBlue[0] == 48) {
              invalidInputs++;
              EEPROM_writeCode(30, invalidInputs);
            }
          }
        }

        hasFinished = false;

        if (ok == 1) {
          digitalWrite(redLED, HIGH);
          if (timerFinal == 3000) {
            digitalWrite(redLED, LOW);
            timerFinal = 0;
            STATE = READY;
          } else {
            if (checkBlueCnt == 0)
              delay(50);
            timerFinal += 50;
          }
        } else {
          tone(buzzer, 3000);
          if (timerFinal == 3000) {
            noTone(buzzer);
            memset(testCode, 0, sizeof testCode);
            timerFinal = 0;
            cntChar = 0;
            digitalWrite(greenLED, HIGH);
            STATE = READY;
          } else {
            if (checkBlueCnt == 0)
              delay(50);
            timerFinal += 50;
          }

        }

        break;

    }
  } else {

    if(prevCheckSystem != 2){
       digitalWrite(greenLED, LOW);
       digitalWrite(yellowLED, LOW);
       digitalWrite(redLED, LOW);

       noTone(buzzer);

       EEPROM_writeCode(40, 51);
       
       prevCheckSystem = 2;
    }
    
   
  }







}


template <class T> int EEPROM_writeCode(int ee, const T& value)
{
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readCode(int ee, T& value)
{
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}


void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0');
}
