bool inputLockFlag = false; 

const int pinLatch595   = 11; 
const int pinCeas595    = 10; 
const int pinDate595    = 12; 

const int numarCifreAfisaj = 4; 
const int piniCifre[numarCifreAfisaj] = {4, 5, 6, 7}; 

const int pinJoystickX  = A0; 
const int pinJoystickY  = A1; 
const int pinJoystickSw = 8; 

const int joyLeftLimit = 300; 
const int joyRightLimit = 700; 
const unsigned long debounceMenuMs = 300; 

unsigned long lastMenuMoveTime = 0; 
unsigned long joyPressStartTime = 0; 
const unsigned long joyLongPressTime = 800; 

const int pinButonPauza = 2; 
const int pinBuzzer     = 9; 

struct intrareFont { 
  char caracter;
  byte model;
};

const intrareFont font7seg[] = { 
  {'0', 0b11111100}, 
  {'1', 0b01100000}, 
  {'2', 0b11011010},
  {'3', 0b11110010}, 
  {'4', 0b01100110}, 
  {'5', 0b10110110},
  {'6', 0b10111110}, 
  {'7', 0b11100000}, 
  {'8', 0b11111110},
  {'9', 0b11110110},
  {'A', 0b11101110}, 
  {'C', 0b10011100}, 
  {'E', 0b10011110},
  {'L', 0b00011100}, 
  {'O', 0b11111100}, 
  {'P', 0b11001110},
  {'R', 0b00001010}, 
  {'S', 0b10110110}, 
  {'t', 0b00011110},
  {'U', 0b01111100}, 
  {'y', 0b01110110}, 
  {'H', 0b01101110},
  {' ', 0b00000000}
};

const int fontCount = sizeof(font7seg) / sizeof(font7seg[0]); 

byte getCharPattern(char c) { 
  for (int i = 0; i < fontCount; i++) {
    if (font7seg[i].caracter == c) 
      return font7seg[i].model;
  }
  return 0;
}

const bool enableSerialFeedback = true; 

void printGameMessage(const String &msg) { 
  if (enableSerialFeedback) {
    Serial.println(msg);
  }
}

char displayChars[numarCifreAfisaj] = {' ', ' ', ' ', ' '}; 

enum blinkModeType { blinkModeNone, blinkModeFast, blinkModeSlow }; 
blinkModeType digitBlinkMode[numarCifreAfisaj] = {
  blinkModeNone, blinkModeNone, blinkModeNone, blinkModeNone
};

unsigned long lastMultiplexTime = 0; 
int currentDigitIndex = 0; 

const unsigned long multiplexIntervalMs = 2; 
const unsigned long blinkFastIntervalMs = 125; 
const unsigned long blinkSlowIntervalMs = 500; 

enum gameStateType { 
  gameStateIdleMenu,
  gameStateMenuAction,
  gameStateShowScoreMenu,
  gameStateShowSequence,
  gameStateInputPhase,
  gameStateCheckAnswer,
  gameStateResult,
  gameStatePaused
};

gameStateType gameState = gameStateIdleMenu; 

const int sequenceLength = 4; 
char sequenceChars[sequenceLength]; 
int inputDigits[sequenceLength]; 
bool digitLocked[sequenceLength] = {false, false, false, false}; 

int activeDigitIndex = 0; 
bool digitEditing = false; 

int currentScore = 0; 
int highScore = 0; 

unsigned long sequenceDisplayTimeMs = 16000; 
const unsigned long sequenceDisplayMinMs = 2000; 
const unsigned long sequenceDisplayStepMs = 4000; 

unsigned long stateStartTime = 0; 
bool resultSuccessFlag = false; 
bool gameInProgress = false; 

const int menuItemCount = 4; 
int menuIndex = 0; 

enum menuActionType { menuActionNone, menuActionPlay, menuActionScore, menuActionStop, menuActionHelp}; 
menuActionType pendingMenuAction = menuActionNone; 

int joystickXDirection = 0, joystickYDirection = 0; 
int joystickXDirectionLast = 0, joystickYDirectionLast = 0; 
bool joystickMoveLeftFlag, joystickMoveRightFlag, joystickMoveUpFlag, joystickMoveDownFlag; 
bool joystickShortPressFlag, joystickLongPressFlag; 

bool joystickSwLastState = HIGH; 
bool joystickSwPressed = false; 
unsigned long joystickSwPressTime = 0; 
bool joystickSwLongPressFired = false; 
const unsigned long joystickLongPressThresholdMs = 700; 

bool pauseButtonLastState = HIGH; 
unsigned long pauseButtonLastChangeTime = 0; 
const unsigned long pauseButtonDebounceMs = 200; 
bool pauseButtonPressedFlag = false; 

const int joystickLowThreshold  = 300; 
const int joystickHighThreshold = 700; 

bool buzzerActive = false; 
unsigned long buzzerEndTime = 0; 

void startBuzzerTone(int freq, unsigned long dur) { 
  tone(pinBuzzer, freq);
  buzzerActive = true;
  buzzerEndTime = millis() + dur;
}

void stopBuzzerTone() { 
  noTone(pinBuzzer); 
  buzzerActive = false; 
  } 

void updateBuzzer() { 
  if (buzzerActive && millis() > buzzerEndTime) 
  stopBuzzerTone(); 
} 

void playTickSound() { 
  startBuzzerTone(2000, 40); 
  } 

void playClickSound() { 
  startBuzzerTone(1500, 80); 
  } 

void playSuccessSound() { 
  startBuzzerTone(2200, 200); 
  } 

void playErrorSound() { 
  startBuzzerTone(400, 400); 
} 

void setSegments(byte pattern) { 
  digitalWrite(pinLatch595, LOW);
  shiftOut(pinDate595, pinCeas595, MSBFIRST, pattern);
  digitalWrite(pinLatch595, HIGH);
}

void enableDigit(int index) { 
  for (int i = 0; i < numarCifreAfisaj; i++) {
    digitalWrite(piniCifre[i], i == index ? LOW : HIGH);
  }
}

void updateDisplayMultiplex() { 
  unsigned long now = millis();

  if (now - lastMultiplexTime < multiplexIntervalMs) 
    return;

  lastMultiplexTime = now;

  currentDigitIndex = (currentDigitIndex + 1) % numarCifreAfisaj;

  bool visible = true;
  
  if (digitBlinkMode[currentDigitIndex] == blinkModeFast)
    visible = ((now / blinkFastIntervalMs) % 2) == 0;
  else 
    if (digitBlinkMode[currentDigitIndex] == blinkModeSlow)
      visible = ((now / blinkSlowIntervalMs) % 2) == 0;

  byte pattern = visible ? getCharPattern(displayChars[currentDigitIndex]) : 0;
  setSegments(pattern);
  enableDigit(currentDigitIndex);
}

void setDisplayText(const char *txt4) { 
  for (int i = 0; i < numarCifreAfisaj; i++) {
    displayChars[i] = txt4[i];
    digitBlinkMode[i] = blinkModeNone;
  }
}

void setDisplayNumber(int value) { 
  if (value < 0) value = 0;

  if (value > 9999) value = 9999;
    displayChars[0] = (value >= 1000) ? ('0' + (value / 1000) % 10) : ' ';
    displayChars[1] = (value >= 100)  ? ('0' + (value / 100) % 10)  : ' ';
    displayChars[2] = (value >= 10)   ? ('0' + (value / 10) % 10)   : ' ';
    displayChars[3] = '0' + (value % 10);

  for (int i = 0; i < numarCifreAfisaj; i++) 
    digitBlinkMode[i] = blinkModeNone;
}

void updateInputBlinkModes() { 
  for (int i = 0; i < sequenceLength; i++) {
    if (digitEditing && i == activeDigitIndex)
      digitBlinkMode[i] = blinkModeFast;
    else if (digitLocked[i])
      digitBlinkMode[i] = blinkModeSlow;
    else
      digitBlinkMode[i] = blinkModeNone;
  }
}

const unsigned long debounceMoveMs = 250; 
unsigned long lastMoveTime = 0; 

void updateJoystickDirection() { 
  int xValue = analogRead(pinJoystickX);
  int yValue = analogRead(pinJoystickY);
  unsigned long now = millis();

  joystickMoveLeftFlag = false;
  joystickMoveRightFlag = false;
  joystickMoveUpFlag = false;
  joystickMoveDownFlag = false;

  if (xValue < 300 && now - lastMoveTime > debounceMoveMs) {
    joystickMoveLeftFlag = true;
    lastMoveTime = now;
  }
  if (xValue > 700 && now - lastMoveTime > debounceMoveMs) {
    joystickMoveRightFlag = true;
    lastMoveTime = now;
  }

  if (yValue < 300 && now - lastMoveTime > debounceMoveMs) {
    joystickMoveUpFlag = true;
    lastMoveTime = now;
  }
  if (yValue > 700 && now - lastMoveTime > debounceMoveMs) {
    joystickMoveDownFlag = true;
    lastMoveTime = now;
  }
}

void updateJoystickButton() { 
  bool sw = digitalRead(pinJoystickSw); 
  unsigned long now = millis(); 

  if (sw != joystickSwLastState) { 
    joystickSwLastState = sw;

    if (sw == LOW) { 
      joystickSwPressed = true;
      joystickSwPressTime = now;
      joystickSwLongPressFired = false;
    } else { 
      if (joystickSwPressed && !joystickSwLongPressFired && now - joystickSwPressTime < joystickLongPressThresholdMs)
        joystickShortPressFlag = true; 
      joystickSwPressed = false;
    }
  }

  if (joystickSwPressed && !joystickSwLongPressFired && now - joystickSwPressTime >= joystickLongPressThresholdMs) {
    joystickLongPressFlag = true; 
    joystickSwLongPressFired = true;
  }
}

void updatePauseButton() { 
  bool st = digitalRead(pinButonPauza); 
  unsigned long now = millis(); 

  if (st != pauseButtonLastState && (now - pauseButtonLastChangeTime) > pauseButtonDebounceMs) {
    pauseButtonLastChangeTime = now; 
    pauseButtonLastState = st; 

    if (st == LOW) 
    pauseButtonPressedFlag = true; 
  }
}

void updateInputs() { 
  updateJoystickDirection(); 
  updateJoystickButton(); 
  updatePauseButton(); 
}

void clearInputFlags() { 
  joystickMoveLeftFlag = joystickMoveRightFlag = joystickMoveUpFlag = joystickMoveDownFlag = false;
  joystickShortPressFlag = joystickLongPressFlag = false;
  pauseButtonPressedFlag = false;
}

void startNewRound() { 
  for (int i = 0; i < sequenceLength; i++)
    sequenceChars[i] = '0' + random(0, 10); 

  setDisplayText(sequenceChars); 
  stateStartTime = millis(); 
  gameState = gameStateShowSequence; 
  gameInProgress = true; 
}

void startNewGame() { 
  pauseButtonPressedFlag = false; 
  pauseButtonLastState = digitalRead(pinButonPauza); 
  pauseButtonLastChangeTime = millis(); 

  currentScore = 0; 
  sequenceDisplayTimeMs = 16000; 
  startNewRound(); 
}

void enterInputPhase() { 
  for (int i = 0; i < sequenceLength; i++) {
    inputDigits[i] = 0;
    digitLocked[i] = false;
    displayChars[i] = '0';
  }
  activeDigitIndex = 0; 
  digitEditing = false; 
  updateInputBlinkModes(); 
  gameState = gameStateInputPhase; 
}

void goToIdleMenu() { 
  setDisplayText("PLAy"); 
  menuIndex = 0; 
  gameState = gameStateIdleMenu; 
  gameInProgress = false; 
}

void updateStateIdleMenu() { 
  if (menuIndex == 0) 
    setDisplayText("PLAy");
  else if (menuIndex == 1) 
    setDisplayText("SCOR");
  else if (menuIndex == 2) 
    setDisplayText("StOP");
  else 
    setDisplayText("HELP");

  int xVal = analogRead(pinJoystickX); 
  unsigned long now = millis();

  if (xVal < joyLeftLimit && now - lastMenuMoveTime > debounceMenuMs) {
    menuIndex--;
    if (menuIndex < 0) 
    menuIndex = menuItemCount - 1;
    playTickSound(); 
    lastMenuMoveTime = now;
  } else if (xVal > joyRightLimit && now - lastMenuMoveTime > debounceMenuMs) {
    menuIndex++;
    
    if (menuIndex >= menuItemCount) 
    menuIndex = 0;
    playTickSound();
    lastMenuMoveTime = now;
  }

  bool sw = digitalRead(pinJoystickSw); 
  if (sw == LOW) {
    if (joyPressStartTime == 0) 
      joyPressStartTime = now; 
  } else {
    if (joyPressStartTime != 0) {
      unsigned long pressLen = now - joyPressStartTime;
      joyPressStartTime = 0;

      if (pressLen >= joyLongPressTime) { 
        playClickSound();

        if (menuIndex == 0) {
          pendingMenuAction = menuActionPlay;
          setDisplayText("PLAy");

        } else if (menuIndex == 1) {
          pendingMenuAction = menuActionScore;
          setDisplayText("SCOR");

        } else if (menuIndex == 2) {
          pendingMenuAction = menuActionStop;
          setDisplayText("StOP");

        } else {
          pendingMenuAction = menuActionHelp;
          setDisplayText("HELP");
        }

        stateStartTime = millis();
        gameState = gameStateMenuAction;
      } else { 
        if (menuIndex == 1) {
          playClickSound();
          pendingMenuAction = menuActionScore;
          setDisplayText("SCOR");
          stateStartTime = millis();
          gameState = gameStateMenuAction;
        } else if (menuIndex == 3) {
          playClickSound();
          pendingMenuAction = menuActionHelp;
          setDisplayText("HELP");
          stateStartTime = millis();
          gameState = gameStateMenuAction;
        }
      }
    }
  }
}

void updateStateMenuAction() { 
  if (millis() - stateStartTime >= 800) {
    if (pendingMenuAction == menuActionPlay) {
      printGameMessage("→ Starting new game...");
      startNewGame();
    }
    else if (pendingMenuAction == menuActionScore) {
      printGameMessage("→ Showing high score...");
      setDisplayNumber(highScore);
      gameState = gameStateShowScoreMenu;
      stateStartTime = millis();
    }
    else if (pendingMenuAction == menuActionStop) {
      printGameMessage("→ Stopping game and returning to menu...");
      goToIdleMenu();
    }
    else if (pendingMenuAction == menuActionHelp) { 
      printGameMessage("");
      printGameMessage(" HOW TO PLAY:");
      printGameMessage("------------------------");
      printGameMessage(" Goal: Memorize the digits shown briefly.");
      printGameMessage("    Then re-enter them using the joystick.");
      printGameMessage("");
      printGameMessage("Controls:");
      printGameMessage(" - Move LEFT / RIGHT: select digit position");
      printGameMessage(" - Move UP / DOWN: change digit value");
      printGameMessage(" - Short press: toggle edit mode");
      printGameMessage(" - Long press: confirm all digits (submit)");
      printGameMessage("");
      printGameMessage(" Tip: Each round gets faster!");
      printGameMessage("------------------------");
      printGameMessage("");
      printGameMessage("Navigate again with joystick ← → to choose another option.");
      printGameMessage("");
      setDisplayText("HELP");
      stateStartTime = millis();
      gameState = gameStateShowScoreMenu; 
    }

    pendingMenuAction = menuActionNone;
    inputLockFlag = false;  
  }
}

void updateStateShowScoreMenu() { 
  if (millis() - stateStartTime >= 2000) 
  goToIdleMenu();
}

void updateStateShowSequence() { 
  unsigned long now = millis();

  if (now - stateStartTime < 800) {
    pauseButtonPressedFlag = false;
  }

  if (pauseButtonPressedFlag) { 
    setDisplayText("PAUS"); 
    printGameMessage("Game paused.");
    gameState = gameStatePaused; 
    stateStartTime = now; 
    return; 
  }

  if (now - stateStartTime >= sequenceDisplayTimeMs) {
    printGameMessage("Sequence display done → your turn!");
    enterInputPhase();
  }
}

void updateStateInputPhase() { 

  if (pauseButtonPressedFlag) { 
    setDisplayText("PAUS"); 
    gameState = gameStatePaused; 
    stateStartTime = millis(); 
    return; 
    }

  if (!digitEditing) { 
    if (joystickMoveLeftFlag) { 
      activeDigitIndex = (activeDigitIndex - 1 + sequenceLength) % sequenceLength; 
      playTickSound(); 
      }
    if (joystickMoveRightFlag){ 
      activeDigitIndex = (activeDigitIndex + 1) % sequenceLength; 
      playTickSound(); 
      }
  }

  if (joystickShortPressFlag) { 
    digitEditing = !digitEditing;
    digitLocked[activeDigitIndex] = !digitEditing;
    playClickSound();
  }

  if (digitEditing) { 
    if (joystickMoveUpFlag) { 
      inputDigits[activeDigitIndex] = (inputDigits[activeDigitIndex] + 1) % 10; playTickSound(); 
      }
    if (joystickMoveDownFlag){ 
      inputDigits[activeDigitIndex] = (inputDigits[activeDigitIndex] + 9) % 10; playTickSound(); 
      }
  }

  for (int i = 0; i < sequenceLength; i++) 
  displayChars[i] = '0' + inputDigits[i];
  updateInputBlinkModes();

  if (joystickLongPressFlag) 
  gameState = gameStateCheckAnswer; 
}

void updateStateCheckAnswer() { 
  bool correct = true;

  for (int i = 0; i < sequenceLength; i++)
    if ('0' + inputDigits[i] != sequenceChars[i]) correct = false;

  resultSuccessFlag = correct;
  if (correct) { 
    currentScore++;
    if (currentScore > highScore) 
    highScore = currentScore;
    sequenceDisplayTimeMs = max(sequenceDisplayTimeMs - sequenceDisplayStepMs, sequenceDisplayMinMs);

    playSuccessSound();
    setDisplayNumber(currentScore);
    printGameMessage("Correct! Starting next round...");

  } else { 
    playErrorSound();
    setDisplayText("Err ");
    printGameMessage(" Wrong! END ROUND → Game lost.");
    printGameMessage("Your final score: " + String(currentScore));
  }
  stateStartTime = millis();
  gameState = gameStateResult;
}

void updateStateResult() { 
  unsigned long now = millis();

  if (pauseButtonPressedFlag) { 
    setDisplayText("PAUS"); 
    gameState = gameStatePaused; 
    stateStartTime = now; 
    return; 
  }

  if (resultSuccessFlag) {
    if (now - stateStartTime >= 800) 
      startNewRound();
  } else {
    if (now - stateStartTime < 700) 
      setDisplayText("Err ");
    else if (now - stateStartTime < 2000) 
      setDisplayNumber(currentScore);
    else 
      goToIdleMenu();
  }
}

void updateStatePaused() { 
   if (millis() - stateStartTime >= 1200) {
    printGameMessage("Resuming to main menu...");
    goToIdleMenu();
  }
}

void setup() { 
  pinMode(pinLatch595, OUTPUT);
  pinMode(pinCeas595, OUTPUT);
  pinMode(pinDate595, OUTPUT);

  for (int i = 0; i < numarCifreAfisaj; i++) { 
    pinMode(piniCifre[i], OUTPUT); 
    digitalWrite(piniCifre[i], HIGH); 
    }
  pinMode(pinJoystickSw, INPUT_PULLUP);
  pinMode(pinButonPauza, INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);

  randomSeed(analogRead(A2));

  setDisplayText("PLAy");
  stateStartTime = millis();

  Serial.begin(9600);

  printGameMessage("=== MEMORY GAME READY ===");
  printGameMessage("Navigate with joystick ← →, press to select.");
  printGameMessage("Current option: START GAME → press to play.");
}

void loop() { 
  updateInputs(); 
  updateBuzzer(); 

  switch (gameState) { 
    case gameStateIdleMenu:       
    updateStateIdleMenu(); 
    break;

    case gameStateMenuAction:     
    updateStateMenuAction(); 
    break;

    case gameStateShowScoreMenu:  
    updateStateShowScoreMenu(); 
    break;

    case gameStateShowSequence:   
    updateStateShowSequence(); 
    break;

    case gameStateInputPhase:     
    updateStateInputPhase(); 
    break;

    case gameStateCheckAnswer:    
    updateStateCheckAnswer(); 
    break;

    case gameStateResult:         
    updateStateResult(); 
    break;

    case gameStatePaused:         
    updateStatePaused(); 
    break;
  }

  updateDisplayMultiplex(); 
  clearInputFlags(); 
}
