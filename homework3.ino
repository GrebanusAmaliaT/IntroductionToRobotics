//pinii folositi pentru senzori si componente
const int TrigPIN = 6; //pin de iesire care trimite impuls catre senzorul ultrasonic (HC-SR04)
const int EchoPIN = 7; //pin de intrare care primeste ecoul reflectat de obiect
const int buzzerPIN = 5; //pin de iesire catre buzzer pentru semnal sonor de alarma
const int Rosu = 10; //pin pentru ledul rosu care indica starea de alarma
const int Verde = 11; //pin pentru ledul verde care indica starea dezarmata
const int LDR = A0; //pin analogic conectat la senzorul de lumina (ldr)

//variabile pentru calibrarea senzorului ultrasonic la pornirea sistemului
bool setareInCurs = true; //flag care indica daca sistemul este in etapa de calibrare initiala
unsigned long ultimaMasurare = 0; //timpul ultimei masuratori in milisecunde
int numarCitiri = 0; //numarul de masuratori efectuate pentru calculul mediei
float sumaSetare = 0; //suma valorilor masurate pentru a obtine media
float distantaInitiala = 0; //distanta medie initiala considerata "normala" (fara intrusi)

//structura care defineste starile posibile ale sistemului
struct State {
  static const byte Dezarmat = 0; //sistemul este inactiv, fara monitorizare
  static const byte SeArmeaza = 1; //sistemul intra in armare cu o scurta intarziere
  static const byte Armat = 2; //sistemul este complet armat si monitorizeaza senzorii
  static const byte PregatitDeActivare = 3; //s-a detectat miscare, asteapta parola de siguranta
  static const byte AlarmaActiva = 4; //alarma este activata, sirena si ledul rosu functioneaza
};

byte state = State::Dezarmat; //starea curenta a sistemului la pornire

//variabile pentru gestionarea parolei si a inputului serial
bool asteaptaparola = false; //indica daca sistemul asteapta introducerea unei parole
byte indexparola = 0; //pozitia curenta in sirul de caractere introdus de utilizator
char numeAlarma[35] = "\nAlarma antiefractie - SPY CAT SRL"; //mesaj de start al sistemului
char parolaSetata[32] = "ADOR_PISICILE"; //parola implicita setata initial
char parolaIntrodusa[32]; //buffer unde se stocheaza parola introdusa pentru comparatie

//parametri configurabili care definesc sensibilitatea si comportamentul sistemului
unsigned int distantaMax = 50; //toleranta maxima in centimetri pentru detectia miscarii
unsigned int buzzerFrecventa = 1000; //frecventa semnalului sonor al buzzerului in hertz
unsigned int pragLDR = 60; //valoare prag pentru lumina, sub care se armeaza automat sistemul

//variabile auxiliare de control si afisare
unsigned long timerAlarmare = 0; //momentul utilizat pentru intarzieri si temporizari
bool inSubmeniuSetari = false; //flag care indica daca utilizatorul se afla in meniul de setari
int vreauAfisareMeniu = 0; //flag folosit pentru afisarea meniului principal dupa o actiune
bool asteaptaparolaNoua = false; //flag folosit la pasul de schimbare a parolei

//initializarea sistemului la pornire
void setup() {
  Serial.begin(9600); //porneste comunicatia seriala cu viteza de 9600 bps

  //configurarea pinilor de intrare si iesire
  pinMode(TrigPIN, OUTPUT);
  pinMode(EchoPIN, INPUT);
  pinMode(buzzerPIN, OUTPUT);
  pinMode(Rosu, OUTPUT);
  pinMode(Verde, OUTPUT);
  pinMode(LDR, INPUT);

  //setarea initiala a starii sistemului (dezarmat)
  digitalWrite(Verde, HIGH);
  digitalWrite(Rosu, LOW);
  noTone(buzzerPIN); //opreste buzzerul daca era activ

  //mesaj de pornire si initierea calibrarii senzorului ultrasonic
  Serial.println(numeAlarma);
  Serial.println("Sistem pornit. Incep calibrarea ultrasonicului...");
  setareUltrasonic(); //apeleaza rutina de calibrare initiala
}

//functie care afiseaza meniul principal de optiuni pentru utilizator
void afisareMeniu() {
  if (vreauAfisareMeniu == 1) {
    Serial.println("\n~~~~~~~~~~~~~~~~~~~~~ Meniul Principal ~~~~~~~~~~~~~~~~~~~~~~");
    Serial.println("SPY CAT a spus: Buna! Cum te pot ajuta astazi?");
    Serial.println("1. Armare sistem");
    Serial.println("2. Setari/Configurare");
    Serial.println("3. Testare alarma");
    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  }
  vreauAfisareMeniu = 0; //reseteaza flagul pentru a evita afisari repetate
}

//functie care afiseaza meniul de setari avansate
void afisareSubmeniuSetari() {
  inSubmeniuSetari = true;
  Serial.println("\n---------------------- Setari Avansate ----------------------");
  Serial.println("SPY CAT a spus: Ce doresti sa modifici?");
  Serial.println("1. Setare sensibilitate ultrasonic (distantaMax)");
  Serial.println("2. Setare prag lumina (pragLDR)");
  Serial.println("3. Setare frecventa buzzer (buzzerFrecventa)");
  Serial.println("4. Schimbare parola");
  Serial.println("5. Schimbare nume sistem");
  Serial.println("6. Revenire la meniu principal");
  Serial.println("-------------------------------------------------------------");
}

//functie care masoara distanta actuala folosind senzorul ultrasonic
float masurareDistanta() {
  //trimite un impuls scurt catre senzor pentru initierea masuratorii
  digitalWrite(TrigPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPIN, LOW);

  //citeste timpul pana cand ecoul este receptionat
  unsigned long durata = pulseIn(EchoPIN, HIGH, 30000); //timeout de 30ms
  if (durata == 0) 
    return -1; //daca nu s-a primit niciun ecou, returneaza eroare

  //calculeaza distanta in centimetri (viteza sunetului ~0.034 cm/us)
  return durata * 0.034 / 2.0;
}

//functie care realizeaza calibrarea automata a senzorului ultrasonic la pornire
void setareUltrasonic() {
  if (!setareInCurs) 
    return;
  
  unsigned long t = millis();

  //realizeaza o masurare la fiecare 100ms
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

    //dupa 5 masuratori valide se finalizeaza calibrarea
    if (numarCitiri >= 5) {
      distantaInitiala = sumaSetare / numarCitiri; //calculeaza media
      Serial.print("\nDistanta de baza: ");
      Serial.print(distantaInitiala);
      Serial.println(" cm");
      setareInCurs = false;

      Serial.println("Calibrare finalizata. Sistem gata de utilizare.");
      Serial.println("Sistemul este dezarmat. Armeaza pentru a activa protectia SPY CAT.");
      vreauAfisareMeniu = 1;
      afisareMeniu();
    }
  }
}

//functie care curata bufferul serial pentru a evita caractere reziduale
void clearSerialBuffer() {
  if(Serial.available() > 0) Serial.read();
}

//functie care gestioneaza meniul principal si comenzile utilizatorului
void handleMenu() {
  //verifica daca exista input valid si sistemul nu asteapta parola
  if (!asteaptaparola && Serial.available() > 0 && !inSubmeniuSetari) {
    char opt = Serial.read(); //citeste optiunea introdusa
    switch (opt) {
      case '1': //armare manuala
        if (state == State::Dezarmat) {
          Serial.println("Armare manuala initiata...");
          timerAlarmare = millis();
          state = State::SeArmeaza;
        }
        break;

      case '2': //intrare in meniul de setari
        if (state == State::Dezarmat) 
          afisareSubmeniuSetari();
        break;

      case '3': //porneste testul de alarma manual
        if (state == State::Dezarmat) {
          Serial.println("Pornire test alarma...");
          state = State::AlarmaActiva;
        }
        break;
    }
  }
}

//functie care gestioneaza meniul de setari avansate
void handleSettingsMenu() {
  char opt = 0;
  bool asteaptaValoare = false;

  if (!inSubmeniuSetari) return;

  if (!asteaptaValoare && Serial.available()) {
    String optStr = Serial.readStringUntil('\n');
    optStr.trim();

    //ignora inputul gol
    if (optStr.length() == 0)
      return;

    opt = optStr.charAt(0);

    Serial.print("Ai ales optiunea: ");
    Serial.println(opt);

    asteaptaValoare = true;
  }

  if (asteaptaValoare && Serial.available()) {

    String valStr = Serial.readStringUntil('\n');
    valStr.trim();

    switch (opt) {
      case '1': //setare sensibilitate ultrasonic
        distantaMax = valStr.toInt();
        Serial.print("Toleranta setata la ");
        Serial.print(distantaMax);
        Serial.println(" cm");
        break;

      case '2': //setare prag lumina pentru armare automata
        pragLDR = valStr.toInt();
        Serial.print("Prag lumina setat la ");
        Serial.println(pragLDR);
        break;

      case '3': //setare frecventa buzzer
        buzzerFrecventa = valStr.toInt();
        Serial.print("Frecventa buzzer setata la ");
        Serial.print(buzzerFrecventa);
        Serial.println(" Hz");
        tone(buzzerPIN, buzzerFrecventa, 500);
        break;

      case '4': //schimbare parola sistem
        Serial.println("Introdu parola curenta:");
        {
          String valStr = "";
          while (valStr.length() == 0) {
            if (Serial.available()) {
              valStr = Serial.readStringUntil('\n');
              valStr.trim();
            }
          }
          if (valStr == parolaSetata) {
            Serial.println("Introdu noua parola:");
            String valNoua = "";
            while (valNoua.length() == 0) {
              if (Serial.available()) {
                valNoua = Serial.readStringUntil('\n');
                valNoua.trim();
              }
            }
            valNoua.toCharArray(parolaSetata, sizeof(parolaSetata));
            clearSerialBuffer();
            Serial.println("parola a fost actualizata cu succes!");
          } else {
            Serial.println("parola incorecta! Acces refuzat.");
          }
        }
        break;

      case '5': //schimbare nume sistem
        Serial.println("Introdu noul nume al sistemului:");
        {
          String valNume = "";
          while (valNume.length() == 0) {
            if (Serial.available()) {
              valNume = Serial.readStringUntil('\n');
              valNume.trim();
            }
          }
          valNume.toCharArray(numeAlarma, sizeof(numeAlarma));
          clearSerialBuffer();
          Serial.print("Nume sistem actualizat: ");
          Serial.println(numeAlarma);
        }
        break;

      case '6': //revenire la meniul principal
        inSubmeniuSetari = false;
        Serial.println("Revenire la meniul principal...");
        vreauAfisareMeniu=1;
        afisareMeniu();
        opt = 0;
        asteaptaValoare = false;
        return;

      default:
        Serial.println("Optiune invalida! Reincearca.");
        afisareSubmeniuSetari();
        break;
    }
    asteaptaValoare = false;
    opt = 0;
    Serial.println();
    afisareSubmeniuSetari();
  }
}

//functie care verifica starea senzorilor in functie de stare
void checkSensors() {
  unsigned long timerPrezent = millis();
  int luminaPrezent = analogRead(LDR); //citeste valoarea curenta a luminii ambientale
  float distanta = masurareDistanta(); //citeste distanta actuala masurata

  //comportament in stare dezarmata
  if (state == State::Dezarmat) {
    digitalWrite(Verde, HIGH);
    digitalWrite(Rosu, LOW);
    noTone(buzzerPIN);

    if (luminaPrezent <= pragLDR) {
      //armare automata cand nivelul de lumina este scazut
      state = State::SeArmeaza;
      timerAlarmare = timerPrezent;
      Serial.println("Se armeaza automat (lumina scazuta detectata)");
    }
  }

  //comportament in stare de armare (intarziere de 3 secunde)
  if (state == State::SeArmeaza)
    if (timerPrezent - timerAlarmare >= 3000UL) {
      state = State::Armat;
      Serial.println("Sistemul a fost armat complet.");
      digitalWrite(Verde, LOW);
      Serial.println("SPY CAT monitorizeaza zona...");
    } else {
      digitalWrite(Verde, (timerPrezent / 500) % 2); //led verde clipitor
    }

  //comportament in stare armat (detectie miscare)
  if (state == State::Armat && distanta > 0 && distanta < 400) {
    float diferenta = fabs(distanta - distantaInitiala);
    if (diferenta > distantaMax) {
      timerAlarmare = timerPrezent;
      state = State::PregatitDeActivare;
      Serial.println("SPY CAT a detectat un posibil intrus! Introduceti parola:");
      asteaptaparola = true;
    }
  }

  //daca parola nu este introdusa in timp util (3 secunde)
  if (state == State::PregatitDeActivare && (timerPrezent - timerAlarmare >= 3000UL) && asteaptaparola == true) {
    Serial.println("Timp expirat - parola neintrodusa!");
    state = State::AlarmaActiva;
    asteaptaparola = true;
    Serial.println("!!!!ALARMA ACTIVATA!!!!");
  }
}

//functie care gestioneaza introducerea si verificarea parolei
void handlePassword() {
  if (Serial.available() > 0 && asteaptaparola) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      parolaIntrodusa[indexparola] = '\0';

      if (strcmp(parolaIntrodusa, parolaSetata) == 0) {
        Serial.println("parola corecta. Sistem dezarmat.");
        state = State::Dezarmat;
        asteaptaparola = false;
        noTone(buzzerPIN);
        digitalWrite(Rosu, LOW);
        digitalWrite(Verde, HIGH);
        Serial.println("Sistemul este acum in stare DEZARMATA.");
      } else {
        Serial.println("parola incorecta! Alarma ramane activa.");
        state = State::AlarmaActiva;
        asteaptaparola = true;
        Serial.println("parola gresita – alarma ramane pornita!");
      }
      indexparola = 0;
      parolaIntrodusa[0] = '\0';
    } else {
      if (indexparola < sizeof(parolaIntrodusa) - 1)
        parolaIntrodusa[indexparola++] = c;
    }
  }
}

//functie care actualizeaza comportamentul sonor si vizual al alarmei
void updateAlarm() {
  if (state == State::AlarmaActiva) {
    static unsigned long bipaitura = 0;
    if (millis() - bipaitura > 300) {
      bipaitura = millis();
      digitalWrite(Rosu, !digitalRead(Rosu)); //led rosu clipitor
    }
    tone(buzzerPIN, buzzerFrecventa); //buzzer activ constant
  } else {
    noTone(buzzerPIN);
    digitalWrite(Rosu, LOW);
  }
}

//bucla principala care coordoneaza intregul sistem
void loop() {
  if (setareInCurs) {
    setareUltrasonic(); //continua procesul de calibrare pana la finalizare
    return;
  }
  if (state == State::AlarmaActiva) {
    handlePassword(); //permite dezarmarea in timpul alarmei
    updateAlarm(); //actualizeaza buzzer si led
  } else {
    handleMenu(); //gestioneaza comenzile utilizatorului
    handleSettingsMenu(); //proceseaza meniul de configurare
    checkSensors();
    handlePassword();
    updateAlarm();
  }
}
    

