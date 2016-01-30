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
unsigned long podajnik_praca_czas = 15000;              // czas jednorazowego podawania wegla
int temperatura_pieca_odczyt = 0;
int temperatura_alarm_pieca = 750;                      // C*10 temperatura przy ktorej uruchamiam alarm i powiadomienie
int temperatura_setpoint_pieca = 550;                   // C*10 temperatura nastawa pieca (wylaczenie nadmuchu, wyłaczenie flagi rozruch)
int temperatura_wlacz_nadmuch = 530;                    // C*10 temperatura wlaczenia nadmuchu
int temperatura_wlacz_podajnik = 520;                   // C*10 temperatura wlaczenia podajnika

//wejscia wyjscia
static int drv_ptt = 2;
static int drv_went = 3;
static int drv_podajnik = 4;                            // LOW - praca, HIGH - stop
static int drv_pompa_wody = 5;
static int sens_woda_temp = 6;
static int sens_paliwo_poziom = 0;
static int reczny_podajnik = A0;                        //LOW - podawanie ręczne
static int praca_stop = A1;                             //LOW - praca, przycisk chwilowy


//zmienne pomocnicze
int flaga_rozruch = 0;
int pozwolenie_pracy_piec = 0;                          //flaga zezwolenia pracy pieca, ustawiana recznie lub z sieci
int pozwolenie_pracy_podajnik = 0;
int flaga_chwilowa_blokada_podajnika = 0;
unsigned long czas_wylaczyc_podajnik = 0;
unsigned long czas_na_pomiar = 0;

//funkcje sterowania wentylatorem
void sterowanie_wentylatorem(){

}

//funkcje sterowania podajnikiem
void automat_podajnik(){
  //jesli jest pozwolenie pracy pieca i nie jest to rozruch i niema chwilowej blokady podajnika to
  //podaj paliwo i zablokuj sie do osiagniecia temperatury 
  if(flaga_rozruch == 0 && pozwolenie_pracy_piec == 1 && flaga_chwilowa_blokada_podajnika == 0){
    if(temperatura_pieca_odczyt <= temperatura_wlacz_podajnik){
      uruchom_podajnik();
      flaga_chwilowa_blokada_podajnika = 1; 
    }
  }
  //reset flagi podajnika jesli osiagnelismy temperature
  if(temperatura_pieca_odczyt >= temperatura_setpoint_pieca){
    flaga_chwilowa_blokada_podajnika = 0;
  }
}

//uruchamianie podajnika sygnal reczny lub z automatyki
void uruchom_podajnik(){
  pozwolenie_pracy_podajnik = 1;
  czas_wylaczyc_podajnik = millis() + 15000;
}

//wyłączanie podajnika
void sterowanie_podajnik(){
  if(pozwolenie_pracy_podajnik == 1){
    digitalWrite(drv_podajnik,LOW);
    if(millis() >= czas_wylaczyc_podajnik){
      digitalWrite(drv_podajnik,HIGH);
      pozwolenie_pracy_podajnik = 0; 
    }
  }
}

void pomiar_temp(){
  if(millis() >= czas_na_pomiar){
    //tutaj robimy pomiar temperatury dallas

    //ustawiamy czas nastepnego pomiaru
    czas_na_pomiar = millis() + 5000;
  }
}

//uruchamianie sterownika
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

//petla glowna
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
  pomiar_temp();
  automat_podajnik();
}
