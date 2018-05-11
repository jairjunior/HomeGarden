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


void setup() {
  // Escreve mensagem por 3 segundos no LCD, depois apaga
  lcd.begin(20,4);
  lcd.setCursor(5,1);
  lcd.print("GrowSystem");
  lcd.setCursor(8,2);
  lcd.print("v1.1");
  delay(3000);
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("GrowSystem");

  //Ajusta o tempo do sistem que é mantido pela biblioteca Time
  //horas, minutos, segundos, dia, mês, ano
  setTime(23,36,50,10,5,2018);

  //Atualiza data e hora no RTC
  RTC.set( now() );

  //Sincroniza a biblioteca Time com a data e hora do RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");

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
  lcd.print(weekDay[myTime.Wday]);
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
