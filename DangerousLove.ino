#include <LiquidCrystal.h>
#include <EEPROM.h>

/pinii pentru ecranul lcd si controalele analogice/digitale
const int PIN_RS = 8, PIN_EN = 9, PIN_D4 = 4, PIN_D5 = 5, PIN_D6 = 6, PIN_D7 = 7;

const int PIN_JOY_X = A0;
const int PIN_JOY_Y = A1;
const int PIN_JOY_BTN = 2; 

const int PIN_BTN_PAUSE = 10; 

const int PIN_BUZZER = 3;    

LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

byte rawGirl[8] = { 
  0b10001, 0b01110, 0b01110, 0b00100, 
  0b01110, 0b11111, 0b11111, 0b01010 
}; 

byte rawBoy[8] = { 
  0b01110, 0b01110, 0b00100, 0b11111, 
  0b11111, 0b11111, 0b01010, 0b01010 
}; 

byte rawHeart[8]  = { 
  0b00000, 0b01010, 0b11111, 0b11111, 
  0b01110, 0b00100, 0b00000, 0b00000 
}; 

enum State { SELECT_CHAR, MENU, SETTINGS, ABOUT, PLAYING, PAUSED, LOVE_ANIMATION, GAMEOVER };

class GameModel {
  public:
    State currentState = SELECT_CHAR; 
    static const int MAP_LENGTH = 100; 
    byte mapData[2][MAP_LENGTH];
    
    float playerX; 
    float playerY; 
    bool isJumping;
    unsigned long jumpStartTime; 
    
    unsigned long animStartTime; 
    unsigned long deathTime;    
    unsigned long aboutStartTime;
    
    int cameraX; 
    int score;
    bool won; 
    bool selectedGirl = true; 
    int settingsOption = 0; 

    void reset() {
        playerX = 2.0;
        playerY = 1.0; 
        isJumping = false;
      
        cameraX = 0;
        score = 0;
        won = false; 
        currentState = PLAYING;
      
        generateMap(); 
    }

    void generateMap() {
        for(int c=0; c<MAP_LENGTH; c++) {
            mapData[0][c] = 0; 
            mapData[1][c] = 0; 

            if(c > 5 && c < MAP_LENGTH-5 && random(0, 10) > 7) 
                mapData[1][c] = 1; 
            
            if(c > 5 && random(0, 15) > 12) 
                mapData[0][c] = 2; 
        }
        mapData[1][MAP_LENGTH-1] = 3; 
    }
};

class IRenderer {
  public:
    virtual void init() = 0;
    virtual void loadCharacters(bool isGirl) = 0;
    virtual void drawSelectScreen(bool isGirl) = 0;
    virtual void drawMenu(int highScores[]) = 0;
    virtual void drawSettings(int option) = 0;
    virtual void drawAbout() = 0;
    virtual void drawGame(GameModel &model) = 0;
    virtual void drawPause() = 0;
    virtual void drawGameOver(GameModel &model) = 0; 
};

class SerialRenderer : public IRenderer {
    float lastX = -99;
  public:
    void init() override { 
        Serial.begin(9600); 
        Serial.println("SERIAL DEBUG READY"); 
    }
    
    void loadCharacters(bool isGirl) override { 
        Serial.print("Selected: "); 
        Serial.println(isGirl ? "GIRL" : "BOY"); 
    }
    
    void drawSelectScreen(bool isGirl) override { 
        Serial.println(isGirl ? "< GIRL >" : "< BOY >"); 
    }
    
    void drawMenu(int highScores[]) override { 
    } 
    
    void drawPause() override { 
        Serial.println("PAUSE"); 
    }
    
    void drawGameOver(GameModel &m) override { 
        Serial.println("GAME OVER");
    }

    void drawSettings(int option) override {
        Serial.print("Settings Option: ");
        Serial.println(option);
    }

    void drawAbout() override {
        Serial.println("About Screen");
    }
    
    void drawGame(GameModel &m) override {
        if(abs(m.playerX - lastX) > 0.1) {
            lastX = m.playerX;
            Serial.print("Player Pos: "); 
            Serial.println(m.playerX);

            Serial.print("Score: "); 
            Serial.println(m.score);
        }
    }
};

class LCDRenderer : public IRenderer {
  public:
    void init() override {
        lcd.begin(16, 2);
    }
    void loadCharacters(bool isGirl) override {
        if (isGirl) {
            lcd.createChar(0, rawGirl);
            lcd.createChar(1, rawBoy); 
        } else {
            lcd.createChar(0, rawBoy);
            lcd.createChar(1, rawGirl);
        }
      
        lcd.createChar(2, rawHeart); 
    }

    void drawSelectScreen(bool isGirl) override {
        lcd.setCursor(0, 0); 
        lcd.print("Choose Player:");
        lcd.setCursor(0, 1);
      
        if (isGirl) 
            lcd.print(" < GIRL >       ");
        else        
            lcd.print(" < BOY  >       ");
    }

    void drawMenu(int highScores[]) override {
        int showIndex = (millis() / 2000) % 3; 
        
        const char* titles[] = { "~Dangerous Love~", "~ No FAKE love ~", "~ Hearts Thief ~", "~ Run FAST Run ~" };
        int titleIndex = (millis() / 3000) % 4;

        lcd.setCursor(0, 0); lcd.print(titles[titleIndex]);
        lcd.setCursor(0, 1); lcd.print(showIndex + 1); 
      
        if(showIndex == 0) 
            lcd.print("st");
        else if(showIndex == 1) 
            lcd.print("nd");
        else 
            lcd.print("rd");
        
        lcd.print(" Score: "); 
        lcd.print(highScores[showIndex]); lcd.print("   "); 
    }

    void drawSettings(int option) override {
        lcd.setCursor(0, 0); 
        lcd.print("Choose Option:");
        lcd.setCursor(0, 1);
        
        if (option == 0) {
            lcd.print("> Reset Score   ");
        } else {
            lcd.print("> About Section ");
        }
    }

    void drawAbout() override {
        lcd.setCursor(0, 0); 
        lcd.print("Creator: Amalia ");
      
        lcd.setCursor(0, 1); 
        lcd.print("Game for fun <3");
    }

    void drawGame(GameModel &m) override {
        lcd.clear();

        for(int r=0; r<2; r++) {
            lcd.setCursor(0, r);
          
            for(int col=0; col<16; col++) {
                int mapIdx = m.cameraX + col;

                if(mapIdx >= m.MAP_LENGTH) { 
                    lcd.print(" "); 
                  
                    continue; 
                }

                if ((int)m.playerX == mapIdx && (int)m.playerY == r) {

                    if (m.currentState == LOVE_ANIMATION) lcd.write((byte)2); 
                    else 
                        lcd.write((byte)0); 
                } else {
                    byte tile = m.mapData[r][mapIdx];

                    if(tile == 0) 
                        lcd.print(" ");
                    else if(tile == 1) 
                        lcd.write((byte)1); 
                    else if(tile == 2)
                        lcd.write((byte)2);
                    else if(tile == 3) 
                        lcd.print("|");
                }
            }
        } 
    }

    void drawPause() override {
        lcd.setCursor(0, 0); 
        lcd.print("  !! PAUSED !!  ");
        lcd.setCursor(0, 1); 
        lcd.print(" Press to Resume");
    }

    void drawGameOver(GameModel &m) override {
        int hearts = m.score / 10; 
        int slide = (millis() / 2000) % 3; 

        lcd.clear(); 
        
        if (!m.won && (millis() - m.deathTime < 1500)) {
            lcd.setCursor(7, 0); 
            lcd.write(byte(2)); 

            lcd.setCursor(5, 1); 
            lcd.print("<3 <3");

            return;
        }

        if (slide == 0) {
            if (m.won) {
                lcd.setCursor(0,0); 
                lcd.print(" Level Completed! ");

                lcd.setCursor(0,1); 
                lcd.print("No toxic love");
            } else {
                lcd.setCursor(0,0); 
                lcd.print(" Game over! :( ");

                lcd.setCursor(0,1); 
                lcd.print("You fell in love");
            }
        } else if (slide == 1) {
            lcd.setCursor(0,0); 
            lcd.print("This round score");

            lcd.setCursor(0,1); 
            lcd.print(hearts); 

            lcd.print(" Hearts "); 
            lcd.write(byte(2));
        } else {
            lcd.setCursor(0,0); 
            lcd.print("Press JOYSTICK");

            lcd.setCursor(0,1); 
            lcd.print("to RESTART ->");
        }
    }
};

class GameController {
    GameModel *model;
    IRenderer *view;

    unsigned long lastUpdate = 0;
    const int TICK_RATE = 100; 
    unsigned long noteDuration = 0;
    unsigned long noteStartTime = 0;
    unsigned long lastButtonPress = 0; 

    unsigned long resetTimer = 0;

  public:
    GameController(GameModel *m, IRenderer *v) { model = m; view = v; }

    void setup() {
        view->init();
        pinMode(PIN_JOY_BTN, INPUT_PULLUP); 
        pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
        pinMode(PIN_BUZZER, OUTPUT);
        randomSeed(analogRead(A5)); 
        
        for(int i=0; i<3; i++) {
            int val; EEPROM.get(i*2, val);

            if(val < 0 || val > 30000) { 
                int z=0; 
                EEPROM.put(i*2, z); 
            }
        }
    }

    void saveScore(int newScore) {
        int scores[3];
        for(int i=0; i<3; i++) 
            EEPROM.get(i*2, scores[i]);
        
         if (newScore > scores[0]) { 
                scores[2]=scores[1]; 
                scores[1]=scores[0]; 
                scores[0]=newScore; 
                }
        else if (newScore > scores[1]) { 
            scores[2]=scores[1]; 
            scores[1]=newScore; 
            }
        else if (newScore > scores[2]) { 
            scores[2]=newScore; 
            }
            
        for(int i=0; i<3; i++) 
        EEPROM.put(i*2, scores[i]);
    }

    void update() {
        unsigned long currentMillis = millis();
        bool btnJoy = !digitalRead(PIN_JOY_BTN);
        bool btnPause = !digitalRead(PIN_BTN_PAUSE);

        if (currentMillis - lastButtonPress > 300) {
            if (btnPause && model->currentState == PLAYING) {
                model->currentState = PAUSED;
                playSound(500, 100); 

                lastButtonPress = currentMillis;
            } else if (btnPause && model->currentState == PAUSED) {
                model->currentState = PLAYING;
                playSound(1000, 100); 

                lastButtonPress = currentMillis;
            }
           else if (btnPause && model->currentState == MENU) {
                model->currentState = SETTINGS;
                model->settingsOption = 0;
                playSound(800, 100); 

                lastButtonPress = currentMillis;
            }
        }
      
        if (noteDuration > 0 && (currentMillis - noteStartTime > noteDuration)) {
            noTone(PIN_BUZZER); noteDuration = 0;
        }

        if (currentMillis - lastUpdate >= TICK_RATE) {
          if (model->currentState == PAUSED) { 
                view->drawPause(); 
                return; 
            }
          if (model->currentState == LOVE_ANIMATION) {
                unsigned long dt = currentMillis - model->animStartTime;
                if (dt < 150) 
                    tone(PIN_BUZZER, 523);
                else if (dt < 300) 
                    tone(PIN_BUZZER, 659);
                else if (dt < 450) 
                    tone(PIN_BUZZER, 784);
                else if (dt < 1000) 
                    tone(PIN_BUZZER, 1046);
                else { 
                    noTone(PIN_BUZZER);
                    model->currentState = GAMEOVER; 
                    } 

                view->drawGame(*model); 
                
                return; 
            }
            
           if (model->currentState == ABOUT) {
                view->drawAbout();

                if (currentMillis - model->aboutStartTime > 5000) 
                    model->currentState = MENU;

                if ((btnJoy || btnPause) && (currentMillis - lastButtonPress > 500)) {
                     model->currentState = MENU; 
                     lastButtonPress = currentMillis;
                }
                return;
            }

            lastUpdate = currentMillis;

            int joyX = analogRead(PIN_JOY_X);
            int joyY = analogRead(PIN_JOY_Y);

              switch (model->currentState) {
                case SELECT_CHAR:
                    if (joyX > 800) 
                        model->selectedGirl = true;

                    if (joyX < 200) 
                        model->selectedGirl = false;

                    view->drawSelectScreen(model->selectedGirl);

                    if (btnJoy && (currentMillis - lastButtonPress > 500)) {
                        view->loadCharacters(model->selectedGirl);
                        playSound(1000, 200);
                        model->currentState = MENU;

                        lastButtonPress = currentMillis;
                    }
                    break;

                case MENU:
                    {
                        int scores[3]; 
                        
                        for(int i=0; i<3; i++) 
                            EEPROM.get(i*2, scores[i]);

                        view->drawMenu(scores);
                    }
                     if (btnJoy && (currentMillis - lastButtonPress > 500)) { 
                        model->reset(); playSound(1000, 200); 
                        lastButtonPress = currentMillis;
                    }
                    break;

                case SETTINGS:
                    if (resetTimer > 0) {
                        if (currentMillis - resetTimer > 2000) {
                            lcd.clear();                
                            model->currentState = MENU; 

                            resetTimer = 0;             
                        }
                       return; 
                    }
                    view->drawSettings(model->settingsOption);

                    if (joyY < 200 || joyX > 800) 
                        model->settingsOption = 1; 
                    if (joyY > 800 || joyX < 200) 
                        model->settingsOption = 0; 

                    if (btnJoy && (currentMillis - lastButtonPress > 500)) {
                        if (model->settingsOption == 0) {
                            int zero = 0; for(int i=0; i<3; i++) EEPROM.put(i*2, zero);
                            tone(PIN_BUZZER, 200, 300); 
                            
                            lcd.setCursor(0, 1);
                            lcd.print("SCORE RESET!    ");

                            resetTimer = currentMillis; 
                            
                        } else {
                            model->currentState = ABOUT;
                            model->aboutStartTime = millis();
                        }
                        lastButtonPress = currentMillis;
                    }
                    
                    if (btnPause && (currentMillis - lastButtonPress > 500)) {
                        model->currentState = MENU; 
                        lastButtonPress = currentMillis;
                    }
                    break;
                
                case PLAYING:
                    handlePhysics(joyX, joyY); 
                    view->drawGame(*model); 
                    break;

                case GAMEOVER:
                     view->drawGameOver(*model);
                     if (currentMillis - lastButtonPress > 1000) { 
                         if (btnJoy) { 
                            model->currentState = MENU; 
                            playSound(1000, 200); 
                            lastButtonPress = currentMillis;
                         }
                     }
                     break;
            }
        }
    }

    void handlePhysics(int joyX, int joyY) {
        if (joyX < 200 && model->playerX > 0) 
            model->playerX -= 0.5; 
            
        if (joyX > 800 && model->playerX < model->MAP_LENGTH - 1) 
            model->playerX += 0.5;

       if (joyY > 800 && !model->isJumping && model->playerY == 1.0) {
             model->isJumping = true; 
             model->playerY = 0;
             model->jumpStartTime = millis(); 

             playSound(600, 100);
        }
        if (model->isJumping && (millis() - model->jumpStartTime > 1500)) {
            model->playerY = 1; 
            model->isJumping = false;
        }

        if (model->playerX - model->cameraX > 8) 
            model->cameraX++;
      
       if (model->playerX - model->cameraX < 2 && model->cameraX > 0) 
            model->cameraX--;

        int pX = (int)model->playerX;
        int pY = (int)model->playerY;

        byte tile = model->mapData[pY][pX]; 

        if (tile == 1) { 
            model->won = false; 

            saveScore(model->score); 

            model->deathTime = millis(); 
            model->animStartTime = millis(); 
            model->currentState = LOVE_ANIMATION; 

            lastButtonPress = millis();
        }
        else if (tile == 2) { 
            model->score += 10; 
            model->mapData[pY][pX] = 0; 
            
            playSound(2000, 50);
        }
        else if (tile == 3) { 
             model->score += 50; model->won = true; 
             saveScore(model->score); 
             model->currentState = GAMEOVER; 

             lastButtonPress = millis();
        }
    }

    void playSound(int freq, int duration) {
        tone(PIN_BUZZER, freq);
        noteStartTime = millis();
        noteDuration = duration;
    }
};

GameModel model;
LCDRenderer lcdRenderer; 
GameController controller(&model, &lcdRenderer);

void setup() { 
    controller.setup(); 
}

void loop() { 
    controller.update(); 
}
