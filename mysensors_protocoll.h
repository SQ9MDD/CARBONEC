/*  CARBONEC sterownik pieca na ekogroszek
 *  SQ9MDD Rysiek Labus @ 2016
 *  
 *  Software na licencji GPL v.2.0
 * 
 * Zmienne:
 * 1. Setpointy (odczyt i zapis)
 * AV1 = temperatura_alarm_pieca (C)
 * AV2 = temperatura_setpoint_pieca (C)
 * AV3 = temperatura_wlacz_nadmuch (C)
 * AV4 = temperatura_wlacz_podajnik (C)
 * AV5 = temperatura_wlacz_pompe (C)
 * AV6 = temperatura_wylacz_pompe (C)
 * AV7 = dlugosc_czasu_pracy_podajnika (sec)
 * AV8 = interwal_pomiedzy_praca_podajnika (sec)
 * 
 * 2. Odczyty i  stany (odczyt)
 * AI1 = temperatura_pieca_odczyt
 * BV1 = pozwolenie_pracy_piec
 * BV2 = flaga_rozruch
 * BV3 = flaga_awaria
 */

 //ustawienia sieciowe
const int net_adr = 73; //allowed range 10-99 !

//zmienne pomocnicze komunikacji
char character;
String content = "";
unsigned long time_to_send_presence = 0;

//jednorazowo przy starcie wysylamy prezentacje punktow by były widoczne w domoticzu
//powtarzamy co dwie godziny
void send_presence(){
  if(millis() >= time_to_send_presence){
    //first setpoints
    digitalWrite(drv_ptt,HIGH);
    delay(50);
    Serial.print(String(net_adr) + ";11;0;0;29;" + String(net_adr) + ".AV1\n"); 
    delay(15);
    Serial.print(String(net_adr) + ";12;0;0;29;" + String(net_adr) + ".AV2\n"); 
    delay(15);
    Serial.print(String(net_adr) + ";13;0;0;29;" + String(net_adr) + ".AV3\n");
    delay(15);
    Serial.print(String(net_adr) + ";14;0;0;29;" + String(net_adr) + ".AV4\n");
    delay(15);
    Serial.print(String(net_adr) + ";15;0;0;29;" + String(net_adr) + ".AV5\n"); 
    delay(15);
    Serial.print(String(net_adr) + ";16;0;0;29;" + String(net_adr) + ".AV6\n");
    delay(15);
    Serial.print(String(net_adr) + ";17;0;0;29;" + String(net_adr) + ".AV7\n");
    delay(15);
    Serial.print(String(net_adr) + ";18;0;0;29;" + String(net_adr) + ".AV8\n");
    delay(60);       
    digitalWrite(drv_ptt,LOW); 

   time_to_send_presence = millis() + 120000;
  }

}

void send_all_data(){

}

//dekodowanie poleceń z sieci (później)
void decode_packet(){

  
}

