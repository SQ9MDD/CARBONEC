/*  CARBONEC sterownik pieca na ekogroszek
 *  SQ9MDD Rysiek Labus @ 2016
 *  
 *  Software na licencji GPL v.2.0
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
int temperatura_pieca_odczyt = 0;                       // C*10 pomiar temperatury na piecu
int temperatura_alarm_pieca = 750;                      // C*10 temperatura przy ktorej uruchamiam alarm i powiadomienie
int temperatura_setpoint_pieca = 550;                   // C*10 temperatura nastawa pieca (wylaczenie nadmuchu, wyłaczenie flagi rozruch)
int temperatura_wlacz_nadmuch = 530;                    // C*10 temperatura wlaczenia nadmuchu
int temperatura_wlacz_podajnik = 520;                   // C*10 temperatura wlaczenia podajnika
int temperatura_wlacz_pompe = 450;                      // C*10 temperatura startu pompy (nie potrzebne zezwolenie pracy)
int temperatura_wylacz_pompe = 400; 

//wejscia wyjscia
static int drv_ptt = 2;                                 // sterowanie transceiverem RS485
static int drv_went = 3;                                // wentylator LOW - praca, HIGH - stop
static int drv_podajnik = 4;                            // podajnik paliwa LOW - praca, HIGH - stop
static int drv_pompa_wody = 5;                          // pompa wody
static int sens_woda_temp = 6;                          // wejscie pomiarowe czujnika temperatury
static int sens_paliwo_poziom = 0;                      // wejscie awaria brak paliwa
static int reczny_podajnik = A0;                        // LOW - podawanie ręczne
static int praca_stop = A1;                             // LOW - praca, przycisk chwilowy


//zmienne pomocnicze
int flaga_rozruch = 0;                                  // flaga sygnalizacyjna rozruch pieca
int flaga_awaria = 0;                                   // flaga sygnalizacyjna awarii pieca
int pozwolenie_pracy_piec = 0;                          // flaga zezwolenia pracy pieca, ustawiana recznie lub z sieci
int pozwolenie_pracy_went = 0;                          //
int pozwolenie_pracy_podajnik = 0;                      // 
int flaga_chwilowa_blokada_podajnika = 0;               // blokada by podajnik zbyt często nie podawał paliwa
int licznik_podan_kolejnych = 0;
unsigned long czas_wylaczyc_podajnik = 0;               //
unsigned long czas_na_pomiar = 0;                       //
unsigned long czas_resetu_podanie_kolejne = 0;

// mierzymy temperature co 5 sekund
void pomiar_temp(){
  if(millis() >= czas_na_pomiar){
    //tutaj robimy pomiar temperatury dallas

    //zdejmowanie flagi rozruchu po osiągnięciu temperatury zadanej pieca
    if(temperatura_pieca_odczyt >= temperatura_setpoint_pieca){
      flaga_rozruch = 0;
      pozwolenie_pracy_went = 0; //przeniesc do automat_wentylator
    }
    //ustawiamy czas nastepnego pomiaru
    czas_na_pomiar = millis() + 5000;
  }
}


//blokady i awarie
void blokady(){
//jesli uzytkownik ustawi bzdury w setupie wylacz piec
  if(temperatura_setpoint_pieca <= temperatura_wlacz_nadmuch || temperatura_setpoint_pieca <= temperatura_wlacz_podajnik){
    pozwolenie_pracy_piec = 0;
    flaga_awaria = 1;
  }
  if(temperatura_pieca_odczyt >= temperatura_alarm_pieca){
    pozwolenie_pracy_piec = 0;
    flaga_awaria = 1;    
  }
}

//sterowanie pompa obiegowa pracuje zawsze
void sterowanie_pompa(){
  if(temperatura_pieca_odczyt >= temperatura_wlacz_pompe){
    digitalWrite(drv_pompa_wody,LOW);
  }
  if(temperatura_pieca_odczyt <= temperatura_wylacz_pompe){
    digitalWrite(drv_pompa_wody,HIGH);  
  }
}

//funkcje sterowania wentylatorem
void sterowanie_wentylatorem(){
  if(flaga_rozruch == 1 || pozwolenie_pracy_went == 1){
    digitalWrite(drv_went,LOW);
  }else{
    digitalWrite(drv_went,HIGH);
  }
}

//funkcje sterowania podajnikiem
void automat_podajnik(){
  //jesli jest pozwolenie pracy pieca i nie jest to rozruch i niema chwilowej blokady podajnika to
  //podaj paliwo i zablokuj sie do osiagniecia temperatury lub podaj za 90 sec (trzykrotnie)
  if(flaga_rozruch == 0 && pozwolenie_pracy_piec == 1 && flaga_chwilowa_blokada_podajnika == 0){
    if(temperatura_pieca_odczyt <= temperatura_wlacz_podajnik){
      uruchom_podajnik();
      flaga_chwilowa_blokada_podajnika = 1; 
      czas_resetu_podanie_kolejne = millis() + 90000; 
    }
  }
  //trzykrotny reset flagi blokady co 90 sekund jesli temperatura nie wzrasta 
  if(temperatura_pieca_odczyt < temperatura_setpoint_pieca && licznik_podan_kolejnych < 3 && flaga_rozruch == 0 && pozwolenie_pracy_piec == 1){
    if(millis() >= czas_resetu_podanie_kolejne){
      flaga_chwilowa_blokada_podajnika = 0;
      licznik_podan_kolejnych++; 
      czas_resetu_podanie_kolejne = millis() + 90000;     
    }    
  }  
  //reset flagi podajnika jesli osiagnelismy temperature
  //oraz jednoczesnie reset licznika podan kolejnych paliwa
  if(temperatura_pieca_odczyt >= temperatura_setpoint_pieca){
    flaga_chwilowa_blokada_podajnika = 0;
    licznik_podan_kolejnych = 0;
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

//uruchamianie sterownika
void setup() {
  pinMode(drv_ptt,OUTPUT);
  pinMode(drv_went,OUTPUT);
  pinMode(drv_podajnik,OUTPUT);
  pinMode(drv_pompa_wody,OUTPUT);
  pinMode(reczny_podajnik,INPUT_PULLUP);
  pinMode(praca_stop,INPUT_PULLUP);
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
  //klawisz uruchamiania pieca
  if(digitalRead(praca_stop) == LOW){
    delay(100);
    if(digitalRead(praca_stop) == LOW){
      pozwolenie_pracy_piec = 1;
      flaga_rozruch = 1;
      flaga_awaria = 0;
    }
  }  

  //funkcje uruchamiane zawsze
  pomiar_temp();
  sterowanie_wentylatorem();
  sterowanie_pompa();
  automat_podajnik();
  sterowanie_podajnik();
}
