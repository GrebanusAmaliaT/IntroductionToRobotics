//led-uri pentru semafor masini 
const int Rosu_Masini=10;
const int Galben_Masini=9;
const int Verde_Masini=8;

//led-uri pentru semafor pietoni 
const int Verde_Pieton=5;
const int Rosu_Pieton=3;

//buzzer si buton
const int Pin_Buton=2; //butonul de cerere pentru pietoni
const int Pin_Buzzer=6;  //buzzerul pentru semnalul sonor pentru pietoni

//pentru logica semaforului 
volatile bool buton_apasat=false; //setata de ISR atunci cand se apasa butonul
unsigned long timer_start=0; //momentul in care incepe numaratoarea de asteptare

bool se_face_galben=false; //indica faza semafor galben pentru masini
unsigned long timer_galben=0;//momentul de start al fazei semafor galben

bool se_face_rosu=false; //indica daca pietonii au verde
unsigned long timer_rosu=0; //momentul de start al fazei pietonilor

bool warning=false; //indica faza de avertizare
unsigned long timer_warning=0; //momentul de start al avertizarii

//configurare timer
const int pinii_timer[8] = {7, 4, 11, 12, 13, A5, A4, A3}; //a, b, c, d, e, f, g, punct

const byte numere[10][7] = {
  {1,1,1,1,1,1,0}, //0
  {0,1,1,0,0,0,0}, //1
  {1,1,0,1,1,0,1}, //2
  {1,1,1,1,0,0,1}, //3
  {0,1,1,0,0,1,1}, //4
  {1,0,1,1,0,1,1}, //5
  {1,0,1,1,1,1,1}, //6
  {1,1,1,0,0,0,0}, //7
  {1,1,1,1,1,1,1}, //8
  {1,1,1,1,0,1,1}  //9
};

//ca sa afisez pe timer cifra corecta
void afisareNumaratoare(int num) {
  if (num < 0 || num > 9) 
    return;
  for (int i = 0; i < 7; i++)
    digitalWrite(pinii_timer[i], numere[num][i]);
}

//ca sa nu mai apara nimic pe timer
void stergereNumaratoare() {
  for (int i = 0; i < 7; i++)
    digitalWrite(pinii_timer[i], LOW);
}

//starea default (verde masini, rosu pietoni)
void idle() {
  digitalWrite(Verde_Masini, HIGH);
  digitalWrite(Galben_Masini, LOW);
  digitalWrite(Rosu_Masini, LOW);

  digitalWrite(Verde_Pieton, LOW);
  digitalWrite(Rosu_Pieton, HIGH);

  stergereNumaratoare(); //ecranul gol
  noTone(Pin_Buzzer); //buzzer oprit
}

//intrerupere buton (ISR)
//se executa automat cand apas butonul (pinul D2 detecteaza tranzitia la LOW)
void se_apasa_butonul() {
  //verific daca nu este deja apasat butonul
  if (!buton_apasat) {
    buton_apasat = true; //semnalizeaza apasarea
  }
}

void setup() {
  //pinii_timer ca iesiri pentru LED-uri si buzzer
  pinMode(Rosu_Masini, OUTPUT);
  pinMode(Galben_Masini, OUTPUT);
  pinMode(Verde_Masini, OUTPUT);

  pinMode(Rosu_Pieton, OUTPUT);
  pinMode(Verde_Pieton, OUTPUT);

  pinMode(Pin_Buzzer, OUTPUT);

  //configurez pinii_timer pentru afisajul 7-segment
  for (int i = 0; i < 7; i++) 
    pinMode(pinii_timer[i], OUTPUT);

  //activez rezistenta interna de pull-up pentru buton
  pinMode(Pin_Buton, INPUT_PULLUP);

  //leg functia ISR la intreruperea generata de buton
  attachInterrupt(digitalPinToInterrupt(Pin_Buton), se_apasa_butonul, FALLING);

  //starea implicita
  idle();
}

void loop() {
  unsigned long timer_in_prezent = millis(); //timp curent


//Starea 1: asteapta apasarea butonului (Idle)
  if (buton_apasat && !se_face_galben && !se_face_rosu && !warning) {
    buton_apasat = false; //resetez flagul
    timer_start = timer_in_prezent; //pentru a tine minte momentul apasarii

    se_face_galben = false;
    se_face_rosu = false;
    warning = false;
  }

//Starea 1.5: numaratoare de 8s inainte de galben
  if (timer_start > 0 && !se_face_galben && !se_face_rosu && !warning) {

    //dupa 8 secunde, se face galben la masini
    if (timer_in_prezent - timer_start >= 8000UL) {
      se_face_galben = true;
      timer_galben = timer_in_prezent;
    } else { //timpul ramas pana la schimbarea culorii
      unsigned long diferenta = (timer_start + 8000UL) - timer_in_prezent;
      int secunde = (diferenta / 1000) + 1;

      afisareNumaratoare(secunde);
    }
  }


//Starea 2: galben masini pentru 3s
  if (se_face_galben) {
    digitalWrite(Verde_Masini, LOW);
    digitalWrite(Galben_Masini, HIGH);
    digitalWrite(Rosu_Masini, LOW);

    digitalWrite(Rosu_Pieton, HIGH);
    digitalWrite(Verde_Pieton, LOW);

    noTone(Pin_Buzzer); //fara sunet

    unsigned long diferenta = (timer_galben + 3000UL) - timer_in_prezent;
    int secunde = (diferenta / 1000) + 1;
    afisareNumaratoare(secunde); //afiseaza numaratoarea de la 3 la 0

    //dupa 3s trece la rosu pentru masini si la verde pentru pietoni
    if (timer_in_prezent - timer_galben >= 3000UL) {
      se_face_galben = false;
      se_face_rosu = true;

      timer_rosu = timer_in_prezent;
    }
  }

//Starea 3: masinile rosu, pietonii verde 8s
  if (se_face_rosu) {
    digitalWrite(Verde_Masini, LOW);
    digitalWrite(Galben_Masini, LOW);
    digitalWrite(Rosu_Masini, HIGH);

    digitalWrite(Rosu_Pieton, LOW);
    digitalWrite(Verde_Pieton, HIGH);

    unsigned long diferenta = (timer_rosu + 8000UL) - timer_in_prezent;
    int secunde = (diferenta / 1000) + 1;
    afisareNumaratoare(secunde);

    //semnal sonor lent – 1 Hz (beep 1s on / 1s off)
    if ((timer_in_prezent / 1000) % 2 == 0) 
      tone(Pin_Buzzer, 1000);
    else 
      noTone(Pin_Buzzer);

    //dupa 8s se intra in warning
    if (timer_in_prezent - timer_rosu >= 8000UL) {
      se_face_rosu = false;
      warning = true;

      timer_warning = timer_in_prezent;

      noTone(Pin_Buzzer);
    }
  }

//Starea 4: warning 4s  verdele clipeste si beepul e mai rapid
  if (warning) {
    digitalWrite(Rosu_Masini, HIGH);
    digitalWrite(Galben_Masini, LOW);
    digitalWrite(Verde_Masini, LOW);

    digitalWrite(Rosu_Pieton, LOW);

    unsigned long diferenta = (timer_warning + 4000UL) - timer_in_prezent;
    int secunde = (diferenta / 1000) + 1;
    afisareNumaratoare(secunde);

    //pentru clipirea rapida a LED-ului verde si beepaitul rapid (~3 Hz)
    if ((timer_in_prezent / 300) % 2 == 0) {
      digitalWrite(Verde_Pieton, HIGH);
      tone(Pin_Buzzer, 1500);
    } else {
      digitalWrite(Verde_Pieton, LOW);
      noTone(Pin_Buzzer);
    }

    //dupa 4s revine in starea initiala (Idle)
    if (timer_in_prezent - timer_warning >= 4000UL) {
      warning = false;

      idle(); //resetez sa revina la starea initiala
      timer_start = 0;
    }
  }
}
