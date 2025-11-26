#include <LiquidCrystal.h>
#include <EEPROM.h>

/pinii pentru ecranul lcd si controalele analogice/digitale
const int PIN_RS = 8, PIN_EN = 9, PIN_D4 = 4, PIN_D5 = 5, PIN_D6 = 6, PIN_D7 = 7;

// pinii pentru joystick si butoane; pullup intern unde e cazul
const int PIN_JOY_X = A0;
const int PIN_JOY_Y = A1;
const int PIN_JOY_BTN = 2;    
const int PIN_BTN_PAUSE = 10; 

const int PIN_BUZZER = 3;    

LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

// definirea grafica a caracterelor speciale (pixel cu pixel)
// 1 inseamna pixel aprins, 0 stins
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

// starile posibile ale jocului pentru state machine
enum State { SELECT_CHAR, MENU, SETTINGS, ABOUT, PLAYING, PAUSED, LOVE_ANIMATION, GAMEOVER };

class GameModel {
  public:
    State currentState = SELECT_CHAR; // jocul incepe direct in meniul de selectie
    static const int MAP_LENGTH = 100; // lungimea totala a nivelului
    byte mapData[2][MAP_LENGTH]; // matricea hartii: 2 randuri si 100 coloane
    
    float playerX; // pozitia x e float pentru miscare fluida
    float playerY; // y e 0 sau 1 (sus sau jos)
    bool isJumping;
    unsigned long jumpStartTime; // retin cand a inceput saritura pentru a o opri automat
    
    unsigned long animStartTime; // timer pentru animatia de final
    unsigned long deathTime;    
    unsigned long aboutStartTime;
    
    int cameraX; // pozitia camerei, ce vede jucatorul pe ecran (fereastra de 16 caractere)
    int score;
    bool won; // flag daca a ajuns la final
    bool selectedGirl = true; // default e selectata fata
    
    int settingsOption = 0; // optiunea curenta din meniul settings (0 sau 1)

    // readuc variabilele la zero pentru un joc nou
    void reset() {
        playerX = 2.0;
        playerY = 1.0; // incepe pe randul de jos
        isJumping = false;
        cameraX = 0;
        score = 0;
        won = false; 
        currentState = PLAYING;
        generateMap(); // generez o harta noua random
    }

    // populez matricea hartii cu obstacole si bonusuri
    void generateMap() {
        for(int c=0; c<MAP_LENGTH; c++) {
            mapData[0][c] = 0; // randul de sus gol initial
            mapData[1][c] = 0; // randul de jos gol initial
            
            // nu pun obstacole in primii 5 pasi si nici la final
            // random(0, 10) > 7 inseamna 20-30% sanse sa apara inamic jos
            if(c > 5 && c < MAP_LENGTH-5 && random(0, 10) > 7) 
                mapData[1][c] = 1; // 1 = inamic/obstacol
            
            // sanse mici sa apara inima sus (bonus)
            if(c > 5 && random(0, 15) > 12) 
                mapData[0][c] = 2; // 2 = inima/punctaj
        }
        mapData[1][MAP_LENGTH-1] = 3; // finalul nivelului
    }
};

// interfata abstracta pentru a putea schimba usor intre lcd si serial daca e nevoie
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
    void init() override { Serial.begin(9600); Serial.println("SERIAL DEBUG READY"); }
    
    void loadCharacters(bool isGirl) override { 
        Serial.print("Selected: "); 
        Serial.println(isGirl ? "GIRL" : "BOY"); 
    }
    
    void drawSelectScreen(bool isGirl) override { 
        Serial.println(isGirl ? "< GIRL >" : "< BOY >"); 
    }
    
    void drawMenu(int highScores[]) override { 
        // trebuia si asta definita chiar daca e goala
    } 
    
    void drawPause() override { Serial.println("PAUSE"); }
    
    void drawGameOver(GameModel &m) override { Serial.println("GAME OVER"); }

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
            Serial.print("Player Pos: "); Serial.println(m.playerX);
            Serial.print("Score: "); Serial.println(m.score);
        }
    }
};
// clasa principala care se ocupa de afisarea pe ecranul 16x2
class LCDRenderer : public IRenderer {
  public:
    void init() override {
        lcd.begin(16, 2);
    }

    // incarc in memoria lcd-ului doar caracterele necesare
    // schimb intre baiat si fata in functie de selectie
    void loadCharacters(bool isGirl) override {
        if (isGirl) {
            lcd.createChar(0, rawGirl); // pozitia 0 e jucatorul
            lcd.createChar(1, rawBoy);  // pozitia 1 e inamicul
        } else {
            lcd.createChar(0, rawBoy);
            lcd.createChar(1, rawGirl);
        }
        lcd.createChar(2, rawHeart); // pozitia 2 e inima
    }

    void drawSelectScreen(bool isGirl) override {
        lcd.setCursor(0, 0); lcd.print("Choose Player:");
        lcd.setCursor(0, 1);
        // afisam text diferit in functie de selectie
        if (isGirl) lcd.print(" < GIRL >       ");
        else        lcd.print(" < BOY  >       ");
    }

    void drawMenu(int highScores[]) override {
        //trecere prin top 3 scoruri la fiecare 2 secunde folosind millis
        int showIndex = (millis() / 2000) % 3; 
        
        // schimbam titlul jocului la fiecare 3 secunde pentru efect vizual
        const char* titles[] = { "~Dangerous Love~", "~ No FAKE love ~", "~ Hearts Thief ~", "~ Run FAST Run ~" };
        int titleIndex = (millis() / 3000) % 4;

        lcd.setCursor(0, 0); lcd.print(titles[titleIndex]);
        lcd.setCursor(0, 1); lcd.print(showIndex + 1); 

        // afisam sufixul corect pentru locul 1, 2 sau 3
        if(showIndex == 0) lcd.print("st");
        else if(showIndex == 1) lcd.print("nd");
        else lcd.print("rd");
        
        lcd.print(" Score: "); lcd.print(highScores[showIndex]); lcd.print("   "); 
    }

    void drawSettings(int option) override {
        lcd.setCursor(0, 0); 
        lcd.print("Choose Option:");
        lcd.setCursor(0, 1);
        
        // sageata > indica optiunea selectata curent
        if (option == 0) {
            lcd.print("> Reset Score   ");
        } else {
            lcd.print("> About Section ");
        }
    }

    void drawAbout() override {
        lcd.setCursor(0, 0); lcd.print("Creator: Amalia ");
        lcd.setCursor(0, 1); lcd.print("Game for fun <3");
    }

    // functia principala de randare a jocului
    void drawGame(GameModel &m) override {
        lcd.clear(); // stergem tot ca sa nu ramana artefacte

        // parcurg cele 2 randuri si cele 16 coloane vizibile
        for(int r=0; r<2; r++) {
            lcd.setCursor(0, r);
            for(int col=0; col<16; col++) {
                // calculez indexul real din harta bazat pe pozitia camerei
                int mapIdx = m.cameraX + col;

                // daca am iesit din harta, nu desenez nimic
                if(mapIdx >= m.MAP_LENGTH) { 
                    lcd.print(" "); 
                    continue; 
                }

                // verific daca jucatorul e pe aceasta pozitie exacta
                // fac cast la int pentru ca pozitia jucatorului e float
                if ((int)m.playerX == mapIdx && (int)m.playerY == r) {
                    // daca a pierdut prin "iubire", afisam inima in loc de jucator
                    if (m.currentState == LOVE_ANIMATION) lcd.write((byte)2); 
                    else 
                        lcd.write((byte)0); // desenez jucatorul normal
                } else {
                    // daca nu e jucatorul, verific ce e pe harta (inamic, inima, zid)
                    byte tile = m.mapData[r][mapIdx];
                    if(tile == 0) lcd.print(" ");
                    else if(tile == 1) 
                        lcd.write((byte)1); // inamic
                    else if(tile == 2)
                        lcd.write((byte)2); // inima
                    else if(tile == 3) 
                        lcd.print("|");     // linia de final
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

    // ecranul de final cu slide-uri care se schimba la 2 secunde
    void drawGameOver(GameModel &m) override {
        int hearts = m.score / 10; // calculez numarul de inimi colectate
        int slide = (millis() / 2000) % 3; 

        lcd.clear(); 
        
        // daca abia a murit, afisam o animatie scurta de 1.5 secunde
        if (!m.won && (millis() - m.deathTime < 1500)) {
            lcd.setCursor(7, 0); lcd.write(byte(2)); 
            lcd.setCursor(5, 1); lcd.print("<3 <3");
            return;
        }

        // slide 1: mesaj text win/lose
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
        } 
        // slide 2: scorul obtinut
        else if (slide == 1) {
            lcd.setCursor(0,0); 
            lcd.print("This round score");
            lcd.setCursor(0,1); 
            lcd.print(hearts); 
            lcd.print(" Hearts "); 
            lcd.write(byte(2));
        } 
        // slide 3: instructiuni restart
        else {
            lcd.setCursor(0,0); 
            lcd.print("Press JOYSTICK");
            lcd.setCursor(0,1); 
            lcd.print("to RESTART ->");
        }
    }
};

// clasa care leaga logica jocului de afisare si input
class GameController {
    GameModel *model;
    IRenderer *view;

    unsigned long lastUpdate = 0;
    const int TICK_RATE = 100; // viteza jocului - update la 100ms
    unsigned long noteDuration = 0;
    unsigned long noteStartTime = 0;
    unsigned long lastButtonPress = 0; // pentru debouncing la butoane

    unsigned long resetTimer = 0;

  public:
    GameController(GameModel *m, IRenderer *v) { model = m; view = v; }

    void setup() {
        view->init();
        // activez rezistentele de pullup interne pentru a nu avea interferente
        pinMode(PIN_JOY_BTN, INPUT_PULLUP); 
        pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
        pinMode(PIN_BUZZER, OUTPUT);
        // folosim un pin analogic in aer pentru seed random real
        randomSeed(analogRead(A5)); 
        
        // initializam eeprom-ul daca valorile sunt gunoi (prima rulare)
        for(int i=0; i<3; i++) {
            int val; EEPROM.get(i*2, val);
            if(val < 0 || val > 30000) { int z=0; EEPROM.put(i*2, z); }
        }
    }

    // logica de salvare top 3 scoruri (shiftare valori)
    void saveScore(int newScore) {
        int scores[3];
        // citim scorurile existente
        for(int i=0; i<3; i++) 
            EEPROM.get(i*2, scores[i]);
        
        // verificam unde se incadreaza noul scor si mutam restul in jos
        if (newScore > scores[0]) { 
                scores[2]=scores[1]; 
                scores[1]=scores[0]; 
                scores[0]=newScore; }
        else if (newScore > scores[1]) { 
            scores[2]=scores[1]; 
            scores[1]=newScore; }
        else if (newScore > scores[2]) { 
            scores[2]=newScore; }
            
        // scriu inapoi in memorie
        for(int i=0; i<3; i++) 
        EEPROM.put(i*2, scores[i]);
    }

    void update() {
        unsigned long currentMillis = millis();
        // citim starea butoanelor (active pe low)
        bool btnJoy = !digitalRead(PIN_JOY_BTN);
        bool btnPause = !digitalRead(PIN_BTN_PAUSE);

        // logica pentru butonul de pauza cu debounce de 300ms
        if (currentMillis - lastButtonPress > 300) {
            // toggle pauza on/off
            if (btnPause && model->currentState == PLAYING) {
                model->currentState = PAUSED;
                playSound(500, 100); 
                lastButtonPress = currentMillis;
            } else if (btnPause && model->currentState == PAUSED) {
                model->currentState = PLAYING;
                playSound(1000, 100); 
                lastButtonPress = currentMillis;
            }
            // intrare in setari din meniu
            else if (btnPause && model->currentState == MENU) {
                model->currentState = SETTINGS;
                model->settingsOption = 0;
                playSound(800, 100); 
                lastButtonPress = currentMillis;
            }
        }

        // opresc buzzerul daca a trecut durata notei
        if (noteDuration > 0 && (currentMillis - noteStartTime > noteDuration)) {
            noTone(PIN_BUZZER); noteDuration = 0;
        }

        // bucla principala de update la fiecare 100ms
        if (currentMillis - lastUpdate >= TICK_RATE) {
            
            // daca e pauza nu fac update la logica, doar desenez ecranul de pauza
            if (model->currentState == PAUSED) { 
                view->drawPause(); return; }

            // secventa audio-vizuala cand jucatorul pierde ("se indragosteste")
            if (model->currentState == LOVE_ANIMATION) {
                unsigned long dt = currentMillis - model->animStartTime;
                // sunete escalate in frecventa
                if (dt < 150) tone(PIN_BUZZER, 523);
                else if (dt < 300) 
                tone(PIN_BUZZER, 659);
                else if (dt < 450) 
                tone(PIN_BUZZER, 784);
                else if (dt < 1000) 
                tone(PIN_BUZZER, 1046);
                else { 
                    noTone(PIN_BUZZER);
                    model->currentState = GAMEOVER; } // trecem la game over
                view->drawGame(*model); return; 
            }
            
            // afisarea ecranului about pentru 5 secunde sau pana se apasa un buton
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
            // citim valorile joystick-ului (0-1023)
            int joyX = analogRead(PIN_JOY_X);
            int joyY = analogRead(PIN_JOY_Y);

            // masina de stari principala
            switch (model->currentState) {
                case SELECT_CHAR:
                    // selectie stanga/dreapta bazata pe valorile analogice
                    if (joyX > 800) // valoare mare -> girl
                        model->selectedGirl = true;
                    if (joyX < 200) // valoare mica -> boy
                        model->selectedGirl = false;
                    view->drawSelectScreen(model->selectedGirl);

                    // confirmare selectie
                    if (btnJoy && (currentMillis - lastButtonPress > 500)) {
                        view->loadCharacters(model->selectedGirl);
                        playSound(1000, 200);
                        model->currentState = MENU;
                        lastButtonPress = currentMillis;
                    }
                    break;

                case MENU:
                    {
                        // citim scorurile proaspat la fiecare frame (putea fi optimizat)
                        int scores[3]; for(int i=0; i<3; i++) EEPROM.get(i*2, scores[i]);
                        view->drawMenu(scores);
                    }
                    // start joc
                    if (btnJoy && (currentMillis - lastButtonPress > 500)) { 
                        model->reset(); playSound(1000, 200); 
                        lastButtonPress = currentMillis;
                    }
                    break;

                case SETTINGS:
                    // VERIFICARE TIMER (Aici se face trecerea la MENU dupa 2 secunde)
                    if (resetTimer > 0) {
                        // Daca au trecut 2 secunde
                        if (currentMillis - resetTimer > 2000) {
                            lcd.clear();                
                            model->currentState = MENU; 
                            resetTimer = 0;             
                        }
                        //cat astept dau return ca sa nu se deseneze altceva peste mesaj
                        return; 
                    }
                    //DESENARE SI NAVIGARE
                    view->drawSettings(model->settingsOption);

                    if (joyY < 200 || joyX > 800) model->settingsOption = 1; 
                    if (joyY > 800 || joyX < 200) model->settingsOption = 0; 

                    if (btnJoy && (currentMillis - lastButtonPress > 500)) {
                        if (model->settingsOption == 0) {
                            // --- RESET SCORE ---
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
                    handlePhysics(joyX, joyY); // calculeaza miscarea
                    view->drawGame(*model);    // deseneaza rezultatul
                    break;

                case GAMEOVER:
                     view->drawGameOver(*model);
                     // restart la apasarea butonului joystick
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

    // aici se intampla toata fizica jocului
    void handlePhysics(int joyX, int joyY) {
        // miscare stanga: cand joystick-ul e impins la stanga (valoare mica)
        // verificam sa nu iasa din harta (x > 0)
        if (joyX < 200 && model->playerX > 0) 
            model->playerX -= 0.5; 
            
        // miscare dreapta: cand joystick-ul e impins la dreapta (valoare mare)
        if (joyX > 800 && model->playerX < model->MAP_LENGTH - 1) 
            model->playerX += 0.5;

        // saritura: daca joystick e sus si jucatorul e pe jos si nu sare deja
        if (joyY > 800 && !model->isJumping && model->playerY == 1.0) {
             model->isJumping = true; 
             model->playerY = 0; // mutam jucatorul sus
             model->jumpStartTime = millis(); 
             playSound(600, 100);
        }
        // terminarea sariturii dupa 1.5 secunde (gravitatie simulata)
        if (model->isJumping && (millis() - model->jumpStartTime > 1500)) {
            model->playerY = 1; // revine jos
            model->isJumping = false;
        }

        // logica camerei: urmareste jucatorul
        // daca jucatorul se indeparteaza prea mult in dreapta, camera avanseaza
        if (model->playerX - model->cameraX > 8) 
            model->cameraX++;
        // daca jucatorul vine inapoi spre stanga, camera se intoarce (dar nu sub 0)
        if (model->playerX - model->cameraX < 2 && model->cameraX > 0) 
            model->cameraX--;

        // detectie coliziuni
        int pX = (int)model->playerX;
        int pY = (int)model->playerY;
        byte tile = model->mapData[pY][pX]; // ce se afla la picioarele jucatorului

        if (tile == 1) { // coliziune cu inamicul (baiatul/fata opus/a)
            model->won = false; 
            saveScore(model->score); // salvez scorul inainte de game over
            model->deathTime = millis(); 
            model->animStartTime = millis(); 
            model->currentState = LOVE_ANIMATION; // animatia de final
            lastButtonPress = millis();
        }
        else if (tile == 2) { // colectare inima
            model->score += 10; 
            model->mapData[pY][pX] = 0; // sterg inima de pe harta
            playSound(2000, 50); // sunet scurt si ascutit
        }
        else if (tile == 3) { // ajuns la linia de finish
             model->score += 50; model->won = true; 
             saveScore(model->score); 
             model->currentState = GAMEOVER; 
             lastButtonPress = millis();
        }
    }

    // functie helper pentru a genera un sunet asincron (non-blocant partial)
    void playSound(int freq, int duration) {
        tone(PIN_BUZZER, freq);
        noteStartTime = millis();
        noteDuration = duration;
    }
};

// initializare obiecte globale
GameModel model;
LCDRenderer lcdRenderer; 
GameController controller(&model, &lcdRenderer);

// SerialRenderer serialRenderer; //obiectul de serial
// GameController controller(&model, &serialRenderer);

void setup() { 
    controller.setup(); 
}

void loop() { 
    controller.update(); 
}
