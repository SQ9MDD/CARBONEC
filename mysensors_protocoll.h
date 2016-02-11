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
unsigned long time_to_send_setpoints = 0;
unsigned long time_to_send_status = 0;

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
    delay(15);
    Serial.print(String(net_adr) + ";1;0;0;6;" + String(net_adr) + ".AI1\n"); 
    delay(60);       
    

    time_to_send_presence = millis() + 7200000; //raz na dwie godziny
    digitalWrite(drv_ptt,LOW); 
  }

}

void send_setpoints(){
  if(millis() >= time_to_send_setpoints){
    digitalWrite(drv_ptt,HIGH);
    delay(50);
    float tmp = float(temperatura_alarm_pieca/10);
    Serial.print(String(net_adr) + ";11;1;0;45;" + String(tmp) + "\n");  
    delay(15);
    tmp = float(temperatura_setpoint_pieca/10);
    Serial.print(String(net_adr) + ";12;1;0;45;" + String(tmp) + "\n");  
    delay(15);
    tmp = float(temperatura_wlacz_nadmuch/10);
    Serial.print(String(net_adr) + ";13;1;0;45;" + String(tmp) + "\n");
    delay(15);
    tmp = float(temperatura_wlacz_podajnik/10);
    Serial.print(String(net_adr) + ";14;1;0;45;" + String(tmp) + "\n");    
    delay(15);
    tmp = float(temperatura_wlacz_pompe/10);
    Serial.print(String(net_adr) + ";15;1;0;45;" + String(tmp) + "\n"); 
    delay(15);
    tmp = float(temperatura_wylacz_pompe/10);
    Serial.print(String(net_adr) + ";16;1;0;45;" + String(tmp) + "\n");
    delay(15);
    int tmp_b = (dlugosc_czasu_pracy_podajnika/1000);
    Serial.print(String(net_adr) + ";17;1;0;45;" + String(tmp_b) + "\n");   
    delay(15);
    tmp_b = (interwal_pomiedzy_praca_podajnika/1000);
    Serial.print(String(net_adr) + ";18;1;0;45;" + String(tmp_b) + "\n");       
        
    time_to_send_setpoints = millis() + 36000000; //raz na godzine
    digitalWrite(drv_ptt,LOW);
  }
}

void send_status(){
  if(millis() >= time_to_send_status){
    digitalWrite(drv_ptt,HIGH);
    delay(20);
    float tmp = float(temperatura_pieca_odczyt/10);
    Serial.print(String(net_adr) + ";1;1;0;0;" + String(tmp) + "\n");
    
    delay(60); 
    time_to_send_status = millis() + 60000; //raz na minute
    digitalWrite(drv_ptt,LOW);
  }

}

//dekodowanie poleceń z sieci (później)
void decode_packet(){

  
}

