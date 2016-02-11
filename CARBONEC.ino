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
 *  Roadmap:
 *  1. mysensors wysyłanie nastaw do sieci
 *  2. mysensors obsługa setpointów
 *  3. zapis do eeprom
 *  
 *  CHANGELOG
 *  2016.02.11 mysensors protocoll, pierwsza wersja
 *  2016.02.09 poprawki błędów wersja stabilna
 *  2016.02.05 czyszczenie kodu, reformating, komentarze, dodana funkcja stop pieca z klawisza, dodana sygnalizacja stanu pracy diodami LED
 *  2016.03.02 poprawki naliczania powtorzen podajnika, czasy pracy podajnika i interwalu pomiedzy wyrzucone do konfiguracji
 * 
 */
// biblioteki
#include <OneWire.h>
#include <DallasTemperature.h>

//**************** konfiguracja (jeśli nie uzywasz domoticza do konfiguracji) *****************************
//
int temperatura_alarm_pieca = 750;                          // C*10 temperatura przy ktorej uruchamiam alarm i powiadomienie
int temperatura_setpoint_pieca = 410;                       // C*10 temperatura nastawa pieca (wylaczenie nadmuchu, wyłaczenie flagi rozruch)
int temperatura_wlacz_nadmuch = 405;                        // C*10 temperatura wlaczenia nadmuchu
int temperatura_wlacz_podajnik = 400;                       // C*10 temperatura wlaczenia podajnika
int temperatura_wlacz_pompe = 280;                          // C*10 temperatura startu pompy (nie potrzebne zezwolenie pracy)
int temperatura_wylacz_pompe = 200; 
unsigned long dlugosc_czasu_pracy_podajnika = 40000;        // jak długo pracuje podajnik po impulsie (msec)
unsigned long interwal_pomiedzy_praca_podajnika = 30000;    // po skonczonej pracy podajnika czekaj (msec)
//
//************** koniec konfiguracji nie modyfikuj nic poniżej jesli nie musisz ***************************

//wejscia wyjscia
static int drv_ptt = 2;                                 // sterowanie transceiverem RS485
static int drv_went = 3;                                // wentylator LOW - praca, HIGH - stop
static int drv_podajnik = 4;                            // podajnik paliwa LOW - praca, HIGH - stop
static int drv_pompa_wody = 5;                          // pompa wody
static int sens_woda_temp = 7;                          // wejscie pomiarowe czujnika temperatury
static int sens_paliwo_poziom = 0;                      // wejscie awaria brak paliwa
static int reczny_podajnik = A0;                        // LOW - podawanie ręczne
static int praca_stop = A1;                             // LOW - praca, przycisk chwilowy
static int led_praca = 8;                               // LED sygnalizacja pracy pieca (ZIELONA)
static int led_rozruch = 9;                             // LED sygnalizacja rozruchu pieca (ŻÓŁTA)
static int led_awaria = 10;                             // LED sygbazlizacja awarii pieca (CZERWONA)
static int led_komunikacja = 11;                        // LED sygnalizacja komunikacji po RS485

//zmienne pomocnicze
int temperatura_pieca_odczyt = 0;                       // C*10 pomiar temperatury na piecu
int flaga_rozruch = 0;                                  // flaga sygnalizacyjna rozruch pieca
int flaga_awaria = 0;                                   // flaga sygnalizacyjna awarii pieca
int pozwolenie_pracy_piec = 0;                          // flaga zezwolenia pracy pieca, ustawiana recznie lub z sieci
int wymuszenie_pracy_went = 0;                          //
int pozwolenie_pracy_podajnik = 0;                      // 
int flaga_chwilowa_blokada_podajnika = 0;               // blokada by podajnik zbyt często nie podawał paliwa
int licznik_podan_kolejnych = 0;                        //
unsigned long czas_wylaczyc_podajnik = 0;               //
unsigned long czas_na_pomiar = 0;                       //
unsigned long czas_resetu_podanie_kolejne = 0;          //

//inicjalizacja bibliotek OneWire i Dallas temp
OneWire oneWire(7);
DallasTemperature sensors(&oneWire);

// podlaczamy komunikacje z domoticz
#include "mysensors_protocoll.h"

// mierzymy temperature co 5 sekund uśrednianie dodac pozniej
void pomiar_temp(){
  if(millis() >= czas_na_pomiar){
    //tutaj robimy pomiar temperatury dallas
    sensors.requestTemperatures();
    //Serial.println(sensors.getTempCByIndex(0));    
    float tempC = sensors.getTempCByIndex(0);
    //Serial.println(tempC);
    temperatura_pieca_odczyt = int(tempC*10);
    /* START DEBUG */
    // Serial.println(String(temperatura_pieca_odczyt)+","+flaga_awaria+","+pozwolenie_pracy_piec+","+flaga_rozruch+","+wymuszenie_pracy_went+","+pozwolenie_pracy_podajnik+","+flaga_chwilowa_blokada_podajnika+","+licznik_podan_kolejnych); //debug
    /* STOP DEBUG */
    //zdejmowanie flagi rozruchu po osiągnięciu temperatury zadanej pieca
    if(temperatura_pieca_odczyt >= temperatura_setpoint_pieca){
      flaga_rozruch = 0;
      //wymuszenie_pracy_went = 0; //przeniesc do automat_wentylator
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

//sterowanie pompa obiegowa pracuje zawsze jesli temp przekroczona niezaleznie od awarii
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
  if(flaga_awaria == 0 && pozwolenie_pracy_piec == 1){
    if(flaga_rozruch == 1 || wymuszenie_pracy_went == 1){
      digitalWrite(drv_went,LOW);
    }else{
      digitalWrite(drv_went,HIGH);
    }     
  }else{
    digitalWrite(drv_went,HIGH);
  }
}

//sterowanie wentylatorem
void automat_wentylator(){
  if (temperatura_pieca_odczyt <= temperatura_wlacz_nadmuch){
    wymuszenie_pracy_went = 1;
  }
  if (temperatura_pieca_odczyt >= temperatura_setpoint_pieca){
    wymuszenie_pracy_went = 0;
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
      czas_resetu_podanie_kolejne = millis() + dlugosc_czasu_pracy_podajnika + interwal_pomiedzy_praca_podajnika; 
    }
  }
  //trzykrotny reset flagi blokady co xxx sekund jesli temperatura nie wzrasta 
  if(temperatura_pieca_odczyt < temperatura_setpoint_pieca && licznik_podan_kolejnych < 3 && flaga_rozruch == 0 && pozwolenie_pracy_piec == 1 && flaga_chwilowa_blokada_podajnika == 1){
    if(millis() >= czas_resetu_podanie_kolejne){
      flaga_chwilowa_blokada_podajnika = 0;
      licznik_podan_kolejnych++; 
      czas_resetu_podanie_kolejne = millis() + dlugosc_czasu_pracy_podajnika + interwal_pomiedzy_praca_podajnika;     
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
  czas_wylaczyc_podajnik = millis() + dlugosc_czasu_pracy_podajnika;
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

//sygnalizacja stanów pracy sterownika
void sygnalizacja_status_led(){
  //praca stop
  if(pozwolenie_pracy_piec == 1){
    digitalWrite(led_praca,HIGH);
  }else{
    digitalWrite(led_praca,LOW);
  }
  //rozruch
  if(flaga_rozruch == 1){
    digitalWrite(led_rozruch,HIGH);
  }else{
    digitalWrite(led_rozruch,LOW);
  }
  //flaga awaria
  if(flaga_awaria == 1){
    digitalWrite(led_awaria,HIGH);
  }else{
    digitalWrite(led_awaria,LOW);
  }
}

//uruchamianie sterownika
void setup() {
  sensors.begin();
  pinMode(drv_ptt,OUTPUT);
  pinMode(drv_went,OUTPUT);
  pinMode(drv_podajnik,OUTPUT);
  pinMode(drv_pompa_wody,OUTPUT);
  pinMode(reczny_podajnik,INPUT_PULLUP);
  pinMode(praca_stop,INPUT_PULLUP);
  pinMode(led_praca,OUTPUT);
  pinMode(led_rozruch,OUTPUT);
  pinMode(led_awaria,OUTPUT);
  pinMode(led_komunikacja,OUTPUT);
  digitalWrite(drv_ptt,LOW);
  digitalWrite(drv_went,HIGH);
  digitalWrite(drv_podajnik,HIGH);
  digitalWrite(drv_pompa_wody,HIGH);
  //mysensors protocol
  Serial.begin(115200);
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
      //start pieca
      if(pozwolenie_pracy_piec == 0){
        pozwolenie_pracy_piec = 1;
        flaga_rozruch = 1;
        flaga_awaria = 0;    
       //stop pieca   
      }else{
        pozwolenie_pracy_piec = 0;
        flaga_rozruch = 0;
        flaga_awaria = 0;        
      }
      delay(300); //opoznienie by nie klikac bez sensu wlacz wylacz szybko
    }
  }  

  //funkcje uruchamiane zawsze
  pomiar_temp();
  automat_wentylator();
  sterowanie_wentylatorem();
  sterowanie_pompa();
  automat_podajnik();
  sterowanie_podajnik();
  sygnalizacja_status_led();

  //komunikacja słuchamy eresa
  while(Serial.available()) {
    character = Serial.read();
    content.concat(character);
    delay (10);
  }
    //obróbka danych z eresa
    content.trim();//clearing CR i LF  
    if(content != ""){
     decode_packet();
     content = "";
    }  
  //funkcje komunikacyjne  
  send_presence();    
  send_setpoints();
}
