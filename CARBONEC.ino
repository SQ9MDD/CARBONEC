/*  CARBONEC sterownik pieca na ekogroszek
 *  Copyright SQ9MDD Rysiek Labus
 * 
 *  Założenia hardware:
 *    - sterowanie wentylatorem nadmuchu
 *    - sterowanie podajnikiem węgla
 *    - sterowanie pompą obiegową
 *    - monitoring temperatury pieca
 *    - monitoring ilosci paliwa
 *    
 *  Komunikacja, protokół my sensors po RS-485
 *  Tryby pracy, rozpalanie, praca ciągła.
 * 
 */
//

//konfiguracja (jeśli nie uzywasz domoticza do konfiguracji)
unsigned long podajnik_praca_czas = 15000; //wstepnie dosypujemy porcco 10minut 

//wejscia wyjscia
static int drv_ptt = 2;
static int drv_went = 3;
static int drv_podajnik = 4;                            // LOW - praca, HIGH - stop
static int drv_pompa_wody = 5;
static int sens_woda_temp = 0;
static int sens_paliwo_poziom = 0;
static int reczny_podajnik = A0;


//zmienne pomocnicze
int pozwolenie_pracy_piec = 0;
int pozwolenie_pracy_podajnik = 0;
unsigned long czas_wylaczyc_podajnik = 0;

//funkcje sterowania podajnikiem
void uruchom_podajnik(){
  pozwolenie_pracy_podajnik = 1;
  czas_wylaczyc_podajnik = millis() + 15000;
}

void sterowanie_podajnik(){
  if(pozwolenie_pracy_podajnik == 1){
    digitalWrite(drv_podajnik,LOW);
    if(millis() >= czas_wylaczyc_podajnik){
      digitalWrite(drv_podajnik,HIGH);
      pozwolenie_pracy_podajnik = 0; 
    }
  }
}


void setup() {
  pinMode(drv_ptt,OUTPUT);
  pinMode(drv_went,OUTPUT);
  pinMode(drv_podajnik,OUTPUT);
  pinMode(drv_pompa_wody,OUTPUT);
  pinMode(reczny_podajnik,INPUT_PULLUP);
  digitalWrite(drv_ptt,LOW);
  digitalWrite(drv_went,HIGH);
  digitalWrite(drv_podajnik,HIGH);
  digitalWrite(drv_pompa_wody,HIGH);
  Serial.begin(9600);
}

void loop() {
  //klawisz sterowania recznego podajnikiem
  if(digitalRead(reczny_podajnik) == LOW){
    delay(100);
    if(digitalRead(reczny_podajnik) == LOW){
      uruchom_podajnik();
    }
  }

  //funkcje uruchamiane zawsze
  sterowanie_podajnik();
}
