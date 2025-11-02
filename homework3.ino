//pinii folositi pentru senzori si componente
const int TrigPIN = 6; //pin de iesire pentru declansarea senzorului ultrasonic
const int EchoPIN = 7; //pin de intrare pentru ecoul senzorului ultrasonic
const int BuzzerPIN = 5; //pin de iesire pentru buzzer (alarma sonora)
const int Rosu = 10; //pin pentru ledul rosu (stare alarma)
const int Verde = 11; //pin pentru ledul verde (stare dezarmat)
const int LDR = A0; //pin analogic pentru senzorul de lumina (ldr)

//variabile pentru calibrarea si masurarea distantei initiale
bool setareInCurs = true; //flag care indica daca sistemul este in calibrare
unsigned long ultimaMasurare = 0; //momentul ultimei masuratori
int numarCitiri = 0; //numarul total de citiri la calibrare
float sumaSetare = 0; //suma tuturor masuratorilor pentru media
float distantaInitiala = 0; //distanta medie initiala masurata

//structura pentru starea sistemului
struct State {
  static const byte Dezarmat = 0; //sistem inactiv
  static const byte SeArmeaza = 1; //sistemul se armeaza
  static const byte Armat = 2; //sistemul monitorizeaza senzorii
  static const byte Pregatit_De_Activare = 3; //intrus detectat, asteapta parola
  static const byte AlarmaActiva = 4; //alarma este activa
};

byte state = State::Dezarmat; //starea curenta a sistemului

//variabile legate de parola si input serial
bool asteaptaParola = false; //indica daca sistemul asteapta o parola
byte index_parola = 0; //pozitia curenta in sirul introdus
char NumeSistem[35] = "\nAlarma antiefractie - SPY CAT SRL"; //mesaj de start
char ParolaSetata[32] = "ADOR_PISICILE"; //parola initiala
char ParolaIntrodusa[32]; //buffer pentru parola introdusa

//parametri configurabili ai sistemului
unsigned int distantaMax = 50; //prag sensibilitate senzor ultrasonic
unsigned int buzzerFrecventa = 1000; //frecventa buzzerului
unsigned int PragLDR = 0; //prag lumina pentru armare automata

//alte variabile de control
unsigned long timer_alarmare = 0; //temporizare pentru diverse procese
bool inSubmeniuSetari = false; //flag daca suntem in meniul de setari
int vreauAfisareMeniu = 0; //flag pentru afisare meniu principal
bool asteaptaParolaNoua = false; //flag pasul 2 din schimbarea parolei

//functie care goleste bufferul serial
void clearSerialBuffer() {
  while (Serial.available() > 0) Serial.read();
}

//initializare sistem
void setup() {
  Serial.begin(9600); //pornire comunicare seriala

  //setare pini
  pinMode(TrigPIN, OUTPUT);
  pinMode(EchoPIN, INPUT);
  pinMode(BuzzerPIN, OUTPUT);
  pinMode(Rosu, OUTPUT);
  pinMode(Verde, OUTPUT);
  pinMode(LDR, INPUT);

  //pornire in stare dezarmata
  digitalWrite(Verde, HIGH);
  digitalWrite(Rosu, LOW);
  noTone(BuzzerPIN);

  //mesaj de start si calibrare
  Serial.println(NumeSistem);
  Serial.println("Sistem pornit. Incep calibrarea ultrasonicului...");
  setareUltrasonic(); //apeleaza calibrarea initiala
}

//functie care afiseaza meniul principal
void afisareMeniu() {
  if (vreauAfisareMeniu == 1) {
    Serial.println("\n~~~~~~~~~~~~~~~~~~~~~ Meniul Principal ~~~~~~~~~~~~~~~~~~~~~~");
    Serial.println("SPY CAT a spus: Buna!!! Cum te pot ajuta astazi? ^~^");
    Serial.println("1. Armare sistem");
    Serial.println("2. Setari/Configurare");
    Serial.println("3. Testare alarma");
    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    Serial.println("Introdu optiunea dorita:");
  }
  vreauAfisareMeniu = 0; //reseteaza flagul
}

//afiseaza submeniul de setari
void afisareSubmeniuSetari() {
  inSubmeniuSetari = true;
  Serial.println("\n---------------------- Setari Avansate ----------------------");
  Serial.println("SPY CAT a spus: Ce doresti sa modifici? ^-^");
  Serial.println("1. Setare sensibilitate ultrasonic (distantaMax)");
  Serial.println("2. Setare prag lumina (PragLDR)");
  Serial.println("3. Setare frecventa buzzer (buzzerFrecventa)");
  Serial.println("4. Schimbare parola");
  Serial.println("5. Revenire la meniu principal");
  Serial.println("-----------------------------------------------------------");
  Serial.println("Alege optiunea:");
}

//functie care masoara distanta actuala
float masurareDistanta() {
  //trimite un impuls scurt catre senzor
  digitalWrite(TrigPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPIN, LOW);

  //citeste timpul ecoului
  unsigned long durata = pulseIn(EchoPIN, HIGH, 30000); //timeout 30ms
  if (durata == 0) return -1; //daca nu s-a primit ecou, returneaza eroare

  //calculeaza distanta in cm
  return durata * 0.034 / 2.0;
}

//calibreaza senzorul ultrasonic la pornire
void setareUltrasonic() {
  if (!setareInCurs) return;
  unsigned long t = millis();

  //masoara la fiecare 100ms
  if (t - ultimaMasurare >= 100) {
    ultimaMasurare = t;
    float d = masurareDistanta();
    if (d > 0) {
      sumaSetare += d;
      numarCitiri++;
      Serial.print("Masurare ");
      Serial.print(numarCitiri);
      Serial.print(": ");
      Serial.print(d);
      Serial.println(" cm");
    }

    //dupa 5 masuratori corecte finalizeaza calibrarea
    if (numarCitiri >= 5) {
      distantaInitiala = sumaSetare / numarCitiri;
      Serial.print("\nDistanta de baza: ");
      Serial.print(distantaInitiala);
      Serial.println(" cm");
      setareInCurs = false;
      Serial.println("Setare finalizata. Sistem gata.");
      Serial.println("Sistemul e dezarmat. Armeaza sa fii protejat de CAT SPY");
      vreauAfisareMeniu = 1;
      afisareMeniu();
    }
  }
}

//proceseaza meniul principal
void handleMenu() {
  //verifica daca exista input si nu asteapta parola
  if (!asteaptaParola && Serial.available() > 0 && !inSubmeniuSetari) {
    char opt = Serial.read(); //citeste optiunea introdusa
    switch (opt) {
      case '1': //armare manuala
        if (state == State::Dezarmat) {
          Serial.println("Armare manuala initiata...");
          timer_alarmare = millis();
          state = State::SeArmeaza;
        }
        break;
      case '2': //intra in meniul de setari
        if (state == State::Dezarmat) 
        afisareSubmeniuSetari();
        break;
      case '3': //testeaza alarma
        if (state == State::Dezarmat) {
          Serial.println("Pornire test alarma...");
          state = State::AlarmaActiva;
        }
        break;
    }
  }
}

//proceseaza meniul de setari
void handleSettingsMenu() {
  static char optSet = 0; //optiunea curenta
  static bool asteaptaValoare = false; //flag pentru introducere numerica
  static bool schimbareParola = false; //flag pentru proces de schimbare parola

  if (!inSubmeniuSetari) 
  return;

  //citeste optiunea de meniu
  if (!asteaptaValoare && Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') 
      return;
    optSet = c;

    //verifica fiecare caz
    if (optSet == '1') {
      Serial.println("Introduceti noua sensibilitate ultrasonic (cm):");
      clearSerialBuffer();
      asteaptaValoare = true;
    } else if (optSet == '2') {
      Serial.println("Introduceti noul prag LDR (0-1023):");
      clearSerialBuffer();
      asteaptaValoare = true;
    } else if (optSet == '3') {
      Serial.println("Introduceti noua frecventa buzzer (Hz):");
      clearSerialBuffer();
      asteaptaValoare = true;
    } else if (optSet == '4') {
      //schimbare parola
      Serial.println("Introduceti parola curenta:");
      clearSerialBuffer();

      asteaptaValoare = true;
      schimbareParola = true;

      index_parola = 0;
      ParolaIntrodusa[0] = '\0';
    } else if (optSet == '5') {
      //revenire la meniu principal
      inSubmeniuSetari = false;
      asteaptaValoare = false;
      schimbareParola = false;
      asteaptaParolaNoua = false;
      optSet = 0;

      clearSerialBuffer();
      vreauAfisareMeniu = 1;
      afisareMeniu();

      return;
    } else {
      Serial.println("Optiune aleasa nu este valida!");
      afisareSubmeniuSetari();
      return;
    }
  }

  //proces de schimbare parola
  if (optSet == '4' && Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      ParolaIntrodusa[index_parola] = '\0';
      int len = strlen(ParolaIntrodusa);
      while (len > 0 && (ParolaIntrodusa[len - 1] == '\r' || ParolaIntrodusa[len - 1] == '\n'))
        ParolaIntrodusa[--len] = '\0';

      if (asteaptaParolaNoua) {
        strncpy(ParolaSetata, ParolaIntrodusa, sizeof(ParolaSetata) - 1);
        ParolaSetata[sizeof(ParolaSetata) - 1] = '\0';
        Serial.println("Parola a fost schimbata cu succes!");
        asteaptaParolaNoua = false;
        asteaptaValoare = false;
        index_parola = 0;
        ParolaIntrodusa[0] = '\0';
        Serial.println();
        afisareSubmeniuSetari();
        return;
      }

      //verificare parola curenta
      if (strcmp(ParolaIntrodusa, ParolaSetata) == 0) {
        Serial.println("Parola corecta. Introduceti noua parola:");
        asteaptaParolaNoua = true;
        index_parola = 0;
        ParolaIntrodusa[0] = '\0';
      } else {
        Serial.println("Parola este incorecta! Nu se poate schimba parola.");
        asteaptaValoare = false;
        index_parola = 0;
        ParolaIntrodusa[0] = '\0';
        Serial.println();
        afisareSubmeniuSetari();
      }
    } else {
      //adauga caractere in buffer
      if (index_parola < sizeof(ParolaIntrodusa) - 1)
        ParolaIntrodusa[index_parola++] = c;
    }
    return;
  }

  //citire valori numerice pentru alte setari
  if (asteaptaValoare && Serial.available() > 0 && optSet != '4') {
   
    String inputString = Serial.readStringUntil('\n'); // citește tot ce ai scris până la Enter
  inputString.trim(); // elimina spatiile, \r, \n
  int valoare = inputString.toInt();

    if (optSet == '1') {
      distantaMax = valoare;
      Serial.print("Noua sensibilitate setata: ");
      Serial.println(distantaMax);
    } else if (optSet == '2') {
      PragLDR =valoare;
      Serial.print("Prag LDR setat: ");
      Serial.println(PragLDR);
    } else if (optSet == '3') {
      buzzerFrecventa = valoare;
      Serial.print("Noua frecventa buzzer: ");
      Serial.println(buzzerFrecventa);
    }
    asteaptaValoare = false;
    optSet = 0;
    Serial.println();
    afisareSubmeniuSetari();
  }
}

//functie care verifica senzorii
void checkSensors() {
  unsigned long timer_prezent = millis();
  int lumina_prezent = analogRead(LDR); //citeste valoarea de lumina
  float distanta = masurareDistanta(); //citeste distanta curenta

  //stare dezarmat
  if (state == State::Dezarmat) {
    digitalWrite(Verde, HIGH);
    digitalWrite(Rosu, LOW);
    noTone(BuzzerPIN);
    if (lumina_prezent <= PragLDR) {
      //armare automata cand e intuneric
      state = State::SeArmeaza;
      timer_alarmare = timer_prezent;
      Serial.println("Se armeaza automat (lumina scazuta)");
    }
  }

  //stare se armeaza
  if (state == State::SeArmeaza)
    if (timer_prezent - timer_alarmare >= 3000UL) {
      state = State::Armat;
      Serial.println("Sistemul a fost armat.");
      digitalWrite(Verde, LOW);
      Serial.println("SPY CAT este pe pozitii");
    } else {
      digitalWrite(Verde, (timer_prezent / 500) % 2);
    }

  //stare armat - detectare miscare
  if (state == State::Armat && distanta > 0 && distanta < 400) {
    float diferenta = fabs(distanta - distantaInitiala);
    if (diferenta > distantaMax) {
      timer_alarmare = timer_prezent;
      state = State::Pregatit_De_Activare;
      Serial.println("SPY CAT a detectat un posibil intrus! Introduceti parola de siguranta:");
      asteaptaParola = true;
    }
  }

  //stare pregatit - daca nu se introduce parola in 3 secunde
  if (state == State::Pregatit_De_Activare && (timer_prezent - timer_alarmare >= 3000UL) && asteaptaParola == true) {
    Serial.println("Parola neintrodusa la timp — alarma pornita!");
    state = State::AlarmaActiva;
    asteaptaParola = true;
    Serial.println("!!!!ALARMA A FOST ACTIVATA!!!!");
  }
}

//functie care gestioneaza introducerea parolei
void handlePassword() {
  if (Serial.available() > 0 && asteaptaParola) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      ParolaIntrodusa[index_parola] = '\0';
      if (strcmp(ParolaIntrodusa, ParolaSetata) == 0) {
        Serial.println("Parola corecta. Sistemul a fost dezarmat.");
        state = State::Dezarmat;
        asteaptaParola = false;
        noTone(BuzzerPIN);
        digitalWrite(Rosu, LOW);
        digitalWrite(Verde, HIGH);
        Serial.println("Sistemul este DEZARMAT");
      } else {
        Serial.println("Parola incorecta!");
        state = State::AlarmaActiva;
        asteaptaParola = true;
        Serial.println("Parola gresita – ALARMA ACTIVATA!");
      }
      index_parola = 0;
      ParolaIntrodusa[0] = '\0';
    } else {
      if (index_parola < sizeof(ParolaIntrodusa) - 1)
        ParolaIntrodusa[index_parola++] = c;
    }
  }
}

//actualizeaza starea alarmei
void updateAlarm() {
  if (state == State::AlarmaActiva) {
    static unsigned long bipaitura = 0;
    if (millis() - bipaitura > 300) {
      bipaitura = millis();
      digitalWrite(Rosu, !digitalRead(Rosu)); //led rosu clipitor
    }
    tone(BuzzerPIN, buzzerFrecventa); //buzzer activ
  } else {
    noTone(BuzzerPIN);
    digitalWrite(Rosu, LOW);
  }
}

//bucla principala
void loop() {
  if (setareInCurs) {
    setareUltrasonic(); //calibrare la start
    return;
  }
  if (state == State::AlarmaActiva) {
    handlePassword();
    updateAlarm();
  } else {
    handleMenu();
    handleSettingsMenu();
    checkSensors();
    handlePassword();
    updateAlarm();
  }
}
