#include <LiquidCrystal.h>    //Display LCD alfanumérico 
#include <Time.h>             //Tempo
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307
#include <Wire.h>             //Interface I2C

tmElements_t myTime;
time_t rtcTime;

const String weekDay[7] = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};

//Pins used with the LCD 
const int rs = 0, 
          en = 1, 
          d4 = 2, 
          d5 = 3, 
          d6 = 4, 
          d7 = 5;

// Instancia objeto do tipo LyquidCrystal
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte block[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };

void setup() {
  // Escreve mensagem por 3 segundos no LCD, depois apaga
  lcd.createChar(0, block);
  lcd.begin(20,4);
  lcd.setCursor(5,0);
  lcd.print("GrowSystem");
  lcd.setCursor(8,1);
  lcd.print("v1.0");
  //Barra de carregamento (loading)
  lcd.setCursor(0,3);
  for(int i = 0; i < 20; i++){
    lcd.write(byte(0));
    delay(250);
  }
  

  //Ajusta o tempo do sistema e atualiza no RTC
  //Uma vez tendo ajustado o tempo no RTC, as duas linhas seguintes podem ser comentadas
  /*setTime(00,9,10,11,5,2018);         //horas, minutos, segundos, dia, mês, ano
   *RTC.set( now() );
   */

  //Sincroniza a biblioteca Time com a data e hora do RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet){
    lcd.clear();
    lcd.print("ERRO:");
    lcd.setCursor(0,1);
    lcd.print("Nao foi possivel");
    lcd.setCursor(0,2);
    lcd.print("sincronizar com RTC.");
    lcd.setCursor(0,3);
    lcd.print("Verifique o sistema.");
  }
  else{
    lcd.clear();
    lcd.setCursor(5,0);
    lcd.print("GrowSystem");
  }

  //Taxa de atualização da biblioteca Time com o RTC (segundos)
  setSyncInterval(1);
}




void loop(){

  rtcTime = RTC.get();
  breakTime(rtcTime, myTime);
  
  //Imprime hora atual na primeira linha do LCD
  lcd.setCursor(0,1);
  lcd.print("Hora: ");
  if(hour() < 10)
    lcd.print("0");
  lcd.print( hour() );
  lcd.print(":");

  if(minute() < 10)
    lcd.print("0");
  lcd.print( minute() );
  lcd.print(":");

  if(second() < 10)
    lcd.print("0");
  lcd.print( second() );


  //Imprime hora atual na segunda linha do LCD
  lcd.setCursor(0,2);
  lcd.print("Data: ");
  lcd.print(weekDay[weekday()-1]);
  lcd.print(", ");

  if(day() < 10)
    lcd.print("0");
  lcd.print( day() );
  lcd.print(".");

  if(month() < 10)
    lcd.print("0");
  lcd.print( month() );
  lcd.print(".");

  lcd.print( year()-2000 );

  delay(1000);
  
}
