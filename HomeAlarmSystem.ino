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
  static const byte PregatitDeActivare = 3; //intrus detectat, asteapta parola
  static const byte AlarmaActiva = 4; //alarma este activa
};

byte state = State::Dezarmat; //starea curenta a sistemului

//variabile legate de parola si input serial
bool asteaptaParola = false; //indica daca sistemul asteapta o parola
byte indexParola = 0; //pozitia curenta in sirul introdus
char numeAlarma[35] = "\nAlarma antiefractie - SPY CAT SRL"; //mesaj de start
char parolaSetata[32] = "parola"; //parola initiala
char parolaIntrodusa[32]; //buffer pentru parola introdusa

//parametri configurabili ai sistemului
unsigned int distantaMax = 50; //prag sensibilitate senzor ultrasonic
unsigned int buzzerFrecventa = 1000; //frecventa buzzerului
unsigned int PragLDR = 0; //prag lumina pentru armare automata

//alte variabile de control
unsigned long timerAlarmare = 0; //temporizare pentru diverse procese
bool inSubmeniuSetari = false; //flag daca suntem in meniul de setari
int vreauAfisareMeniu = 0; //flag pentru afisare meniu principal
bool asteaptaParolaNoua = false; //flag pasul 2 din schimbarea parolei

//functie care initializeaza sistemul si porneste calibrarile
void setup() {
  Serial.begin(9600); //porneste comunicarea seriala cu viteza 9600 bps

  //setare pini ca intrari/iesiri pentru componente
  pinMode(TrigPIN, OUTPUT);
  pinMode(EchoPIN, INPUT);
  pinMode(BuzzerPIN, OUTPUT);
  pinMode(Rosu, OUTPUT);
  pinMode(Verde, OUTPUT);
  pinMode(LDR, INPUT);

  //pornire in stare dezarmata
  digitalWrite(Verde, HIGH); //led verde aprins = sistem dezarmat
  digitalWrite(Rosu, LOW); //led rosu stins
  noTone(BuzzerPIN); //buzzer oprit

  //mesaj de pornire si calibrare
  Serial.println(numeAlarma);
  Serial.println("Sistem pornit. Incep calibrarea ultrasonicului...");
  setareUltrasonic(); //apeleaza functia de calibrare
}

//functie care afiseaza meniul principal pentru utilizator
void afisareMeniu() {
  if (vreauAfisareMeniu == 1) { //verifica daca e setat flagul de afisare
    Serial.println("\n~~~~~~~~~~~~~~~~~~~~~ Meniul Principal ~~~~~~~~~~~~~~~~~~~~~~");
    Serial.println("SPY CAT a spus: Buna!!! Cum te pot ajuta astazi?");
    Serial.println("1. Armare sistem"); //pornire manuala a armarii
    Serial.println("2. Setari/Configurare"); //meniu de setari
    Serial.println("3. Testare alarma"); //test buzzer si led
    Serial.println("~~~~~~~~~~~~~~~~~~~~~~ Introdu optiunea dorita ~~~~~~~~~~~~~~~~~~~~~");
  }
  vreauAfisareMeniu = 0; //reseteaza flagul dupa afisare
}

//functie care afiseaza submeniul de configurare
void afisareSubmeniuSetari() {
  inSubmeniuSetari = true; //activeaza flagul de submeniu
  Serial.println("\n---------------------- Setari Avansate ----------------------");
  Serial.println("SPY CAT a spus: Ce doresti sa modifici?");
  Serial.println("1. Setare sensibilitate ultrasonic (distantaMax)");
  Serial.println("2. Setare prag lumina (PragLDR)");
  Serial.println("3. Setare frecventa buzzer (buzzerFrecventa)");
  Serial.println("4. Schimbare parola");
  Serial.println("5. Revenire la meniul principal");
  Serial.println("------------------------ Tasteaza optiunea! --------------------");
}

//functie care masoara distanta actuala folosind senzorul ultrasonic
float masurareDistanta() {
  //trimite un impuls scurt catre senzor pentru declansare
  digitalWrite(TrigPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPIN, LOW);

  //citeste timpul pana la intoarcerea ecoului
  unsigned long durata = pulseIn(EchoPIN, HIGH, 30000); //timeout 30ms
  if (durata == 0) 
    return -1; //daca nu s-a primit ecou, returneaza eroare

  //calculeaza distanta in centimetri
  return durata * 0.034 / 2.0;
}

//functie care face calibrarea senzorului ultrasonic la pornire
void setareUltrasonic() {
  if (!setareInCurs) 
    return; //iese daca calibrarea e deja finalizata
  
  unsigned long t = millis(); //timpul curent in milisecunde

  //face o masurare la fiecare 100ms
  if (t - ultimaMasurare >= 100) {
    ultimaMasurare = t;
    float d = masurareDistanta(); //citeste distanta
    if (d > 0) { //verifica daca masurarea e valida
      sumaSetare += d; //adauga la suma totala
      numarCitiri++; //incrementeaza numarul de citiri
      Serial.print("Masurare ");
      Serial.print(numarCitiri);
      Serial.print(": ");
      Serial.print(d);
      Serial.println(" cm");
    }

    //cand a facut 5 masuratori corecte finalizeaza calibrarea
    if (numarCitiri >= 5) {
      distantaInitiala = sumaSetare / numarCitiri; //calculeaza media
      Serial.print("\nDistanta de baza: ");
      Serial.print(distantaInitiala);
      Serial.println(" cm");
      setareInCurs = false; //opreste modul de calibrare

      Serial.println("Setare finalizata. Sistem gata.");
      Serial.println("Sistemul e dezarmat. Armeaza sa fii protejat de CAT SPY");
      vreauAfisareMeniu = 1; //seteaza flagul pentru afisare meniu
      afisareMeniu(); //afiseaza meniul principal
    }
  }
}

//functie care goleste eventuale caractere ramase in bufferul serial
void clearSerialBuffer() {
  if(Serial.available() > 0) 
    Serial.read(); //citeste si ignora un caracter
}

//functie care proceseaza optiunile introduse in meniul principal
void handleMenu() {
  //verifica daca exista input si nu se asteapta o parola
  if (!asteaptaParola && Serial.available() > 0 && !inSubmeniuSetari) {
    char opt = Serial.read(); //citeste optiunea introdusa

    switch (opt) {
      case '1': //armare manuala
        if (state == State::Dezarmat) {
          Serial.println("Armare manuala initiata...");
          timerAlarmare = millis(); //porneste temporizatorul de armare
          state = State::SeArmeaza; //trece in stare de armare
        }
        break;

      case '2': //intra in meniul de setari
        if (state == State::Dezarmat) 
          afisareSubmeniuSetari();
        break;

      case '3': //testeaza alarma
        if (state == State::Dezarmat) {
          Serial.println("Pornire test alarma...");
          state = State::AlarmaActiva; //porneste alarma pentru test
        }
        break;
    }
  }
}
//functie care proceseaza meniul de setari si modificarile parametrilor
void handleSettingsMenu() {
  static char opt = 0; //retine optiunea selectata din meniu
  static bool asteaptaValoare = false; //indica daca se asteapta o valoare noua
  static bool schimbParola = false; //indica daca se face schimbarea parolei
  static bool asteptParolaCurenta = false; //asteapta introducerea parolei actuale
  static bool asteptParolaNoua = false; //asteapta noua parola

  if (!inSubmeniuSetari) 
    return; //iese daca nu suntem in meniul de setari

  //citeste optiunea din meniu daca nu se asteapta alta valoare
  if (!schimbParola && !asteaptaValoare && Serial.available()) {
    String optStr = Serial.readStringUntil('\n'); //citeste pana la enter
    optStr.trim(); //elimina spatiile goale
    if (optStr.length() == 0)
      return;
    opt = optStr.charAt(0); //retine prima cifra
    Serial.print("Ai ales optiunea: ");
    Serial.println(opt);

    //daca optiunea este schimbarea parolei
    if (opt == '4') {
      Serial.println("Introdu parola curenta:");
      schimbParola = true;
      asteptParolaCurenta = true;
      asteptParolaNoua = false;
      asteaptaValoare = false;
      return; //iese pentru a astepta parola
    }
    asteaptaValoare = true; //altfel se asteapta o valoare numerica
  }

  //proces pentru schimbarea parolei
  if (schimbParola && Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (asteptParolaCurenta) { //verifica parola curenta
      if (input == parolaSetata) {
        Serial.println("Parola corecta. Introdu noua parola:");
        asteptParolaCurenta = false;
        asteptParolaNoua = true;
      } else {
        Serial.println("Parola gresita. Iesire din schimbare parola.");
        asteptParolaCurenta = false;
        asteptParolaNoua = false;
        schimbParola = false;
        afisareSubmeniuSetari(); //revine in meniu
      }
    } else if (asteptParolaNoua) { //seteaza noua parola
      input.toCharArray(parolaSetata, sizeof(parolaSetata));
      Serial.println("Parola a fost actualizata. SPY CAT te saluta!");
      asteptParolaNoua = false;
      schimbParola = false;
      afisareSubmeniuSetari();
    }
  }

  //daca s-a ales o optiune care necesita o valoare numerica
  if (asteaptaValoare && Serial.available()) {
    String valStr = Serial.readStringUntil('\n');
    valStr.trim();

    switch (opt) {
      case '1': //modifica distanta maxima
        distantaMax = valStr.toInt();
        Serial.print("Toleranta setata la ");
        Serial.print(distantaMax);
        Serial.println(" cm");
        break;
      case '2': //modifica pragul de lumina
        PragLDR = valStr.toInt();
        Serial.print("Prag lumina setat la ");
        Serial.println(PragLDR);
        break;
      case '3': //modifica frecventa buzzerului
        buzzerFrecventa = valStr.toInt();
        Serial.print("Frecventa setata la ");
        Serial.print(buzzerFrecventa);
        Serial.println(" Hz");
        tone(BuzzerPIN, buzzerFrecventa, 500); //emite un bip scurt
        break;
      case '5': //revenire la meniul principal
        Serial.println("Revenire la meniul principal.");
        vreauAfisareMeniu = 1;
        afisareMeniu();
        asteaptaValoare = false;
        opt = 0;
        inSubmeniuSetari = false;
        return;
      default:
        Serial.println("Optiune invalida!");
        afisareSubmeniuSetari();
        break;
    }
    asteaptaValoare = false;
    opt = 0;

    Serial.println();
    afisareSubmeniuSetari(); //reafiseaza meniul dupa modificare
  }
}

//functie care verifica senzorii si gestioneaza starile sistemului
void checkSensors() {
  unsigned long timerPrezent = millis(); //retine timpul curent
  int luminaPrezent = analogRead(LDR); //citeste nivelul de lumina
  float distanta = masurareDistanta(); //citeste distanta curenta

  //cand sistemul este dezarmat
  if (state == State::Dezarmat) {
    digitalWrite(Verde, HIGH); //led verde aprins
    digitalWrite(Rosu, LOW); //led rosu stins
    noTone(BuzzerPIN); //buzzer oprit

    //daca lumina scade sub prag, armeaza automat
    if (luminaPrezent < PragLDR) {
      state = State::SeArmeaza;
      timerAlarmare = timerPrezent;
      Serial.println("Se armeaza automat (lumina scazuta)");
    }
  }

  //cand sistemul se armeaza manual sau automat
  if (state == State::SeArmeaza)
    if (timerPrezent - timerAlarmare >= 3000UL) { //dupa 3 secunde devine armat
      state = State::Armat;
      Serial.println("Sistemul a fost armat.");
      digitalWrite(Verde, LOW);
      Serial.println("SPY CAT este pe pozitii");

      vreauAfisareMeniu=1;
      afisareMeniu();
    } else {
      digitalWrite(Verde, (timerPrezent / 500) % 2); //clipeste verde
    }

  //cand sistemul este armat si detecteaza o miscare
  if (state == State::Armat && distanta > 0 && distanta < 400) {
    float diferenta = fabs(distanta - distantaInitiala); //calculeaza diferenta
    if (diferenta > distantaMax) { //daca depaseste pragul
      timerAlarmare = timerPrezent;
      state = State::PregatitDeActivare;
      Serial.println("SPY CAT a detectat un posibil intrus! Introduceti parola de siguranta:");
      asteaptaParola = true;
    }
  }

  //daca nu se introduce parola in timp util
  if (state == State::PregatitDeActivare && (timerPrezent - timerAlarmare >= 3000UL) && asteaptaParola == true) {
    Serial.println("Parola neintrodusa la timp — alarma pornita!");
    state = State::AlarmaActiva;
    asteaptaParola = true;
    Serial.println("!!!!ALARMA A FOST ACTIVATA!!!!");
  }
}

//functie care proceseaza introducerea parolei
void handlePassword() {
  if (Serial.available() > 0 && asteaptaParola) {
    char c = Serial.read(); //citeste caracterul introdus
    
    if (c == '\n' || c == '\r') { //cand s-a apasat enter
      parolaIntrodusa[indexParola] = '\0'; //termina sirul
      //compara parola introdusa cu cea setata
      if (strcmp(parolaIntrodusa, parolaSetata) == 0) {
        Serial.println("Parola corecta. Sistemul a fost dezarmat.");
        state = State::Dezarmat; //revine in stare dezarmata
        asteaptaParola = false;
        noTone(BuzzerPIN); //opreste alarma
        digitalWrite(Rosu, LOW);
        digitalWrite(Verde, HIGH);
        Serial.println("Sistemul este DEZARMAT");
      } else {
        Serial.println("Parola incorecta!");
        state = State::AlarmaActiva; //porneste alarma
        asteaptaParola = true;
        Serial.println("Parola gresita – ALARMA ACTIVATA!");
      }
      indexParola = 0;
      parolaIntrodusa[0] = '\0'; //reseteaza bufferul
    } else {
      if (indexParola < sizeof(parolaIntrodusa) - 1)
        parolaIntrodusa[indexParola++] = c; //adauga caracterul la sir
    }
  }
}

//functie care actualizeaza starea alarmei cand este activa
void updateAlarm() {
  if (state == State::AlarmaActiva) {
    static unsigned long bipaitura = 0; //momentul ultimului bip
    if (millis() - bipaitura > 300) { //la fiecare 300ms
      bipaitura = millis();
      digitalWrite(Rosu, !digitalRead(Rosu)); //ledul rosu clipeste
    }
    tone(BuzzerPIN, buzzerFrecventa); //activeaza buzzerul continuu
  } else {
    noTone(BuzzerPIN); //opreste buzzerul
    digitalWrite(Rosu, LOW); //stinge ledul rosu
  }
}

//functie principala care ruleaza continuu si coordoneaza toate celelalte
void loop() {
  if (setareInCurs) { //daca e in calibrare
    setareUltrasonic(); //repeta masuratorile initiale
    return;
  }

  if (state == State::AlarmaActiva) { //cand alarma este activa
    handlePassword(); //permite oprirea cu parola
    updateAlarm(); //mentine buzzerul si ledul
  } else {
    handleMenu(); //proceseaza meniul principal
    handleSettingsMenu(); //proceseaza submeniul de setari
    checkSensors(); //verifica senzorii
    handlePassword(); //verifica introducerea parolei
    updateAlarm(); //actualizeaza buzzer si leduri
  }
}
