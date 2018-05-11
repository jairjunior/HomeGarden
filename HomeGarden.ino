#include <LiquidCrystal.h>    //Display LCD alfanumérico 
#include <Time.h>             //Tempo
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307
#include <Wire.h>             //Interface I2C

#define PRINT_SECONDS         B00000001
#define PRINT_TEXT_DATE       B00000010
#define PRINT_TEXT_HOUR       B00000100
#define DOT_SEPARATOR         B00001000
#define DASH_SEPARATOR        B00010000


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



/********************************************************************************
 * 
 * Função SETUP - Rod auma vez ao inicializar.
 * Configura LCD, mostra mensagem e barra de inicialização
 * Ajusta tempo do RTC vi software (opcional)
 * Sincroniza biblioteca Time com periférico externo (RTC)
 * 
 *******************************************************************************/
void setup() {
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
  //setTime(10,29,0,11,5,2018);         //horas, minutos, segundos, dia, mês, ano
  //RTC.set( now() );


  //Sincroniza a biblioteca Time com a data e hora do RTC
  //Caso a sincronização tenha falhado, exibe mensagem de erro no LCD e trava execução
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet){
    lcd.clear();
    lcd.print("ERRO 743:");
    lcd.setCursor(0,1);
    lcd.print("Nao foi possivel");
    lcd.setCursor(0,2);
    lcd.print("sincronizar com RTC.");
    lcd.setCursor(0,3);
    lcd.print("Verifique o sistema.");
    while(true){}
  }
  //Se a sincronização ocorreu, escreve nome no LCD
  else{
    lcd.clear();
    lcd.setCursor(5,0);
    lcd.print("GrowSystem");
  }

  //Taxa de atualização da biblioteca Time com o RTC (segundos)
  setSyncInterval(60);
}




void loop(){

  printDateAndHour(0,3, 0,2, PRINT_TEXT_DATE | PRINT_TEXT_HOUR | PRINT_SECONDS | DASH_SEPARATOR);
  delay(1000);
  
}



void printDateAndHour(int dateCol, int dateRow, int hourCol, int hourRow, byte printConfigs ){
const String weekDay[7] = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};

  //Imprime hora atual na primeira linha do LCD
  lcd.setCursor(hourCol,hourRow);
  if(printConfigs & PRINT_TEXT_HOUR)
    lcd.print("Hora: ");
  if(hour() < 10)
    lcd.print("0");
  lcd.print( hour() );
  lcd.print(":");

  if(minute() < 10)
    lcd.print("0");
  lcd.print( minute() );
  lcd.print(":");

  if(printConfigs & PRINT_SECONDS){
    if(second() < 10)
      lcd.print("0");
    lcd.print( second() );
  }

  //Imprime hora atual na segunda linha do LCD
  lcd.setCursor(dateCol,dateRow);
  if(printConfigs & PRINT_TEXT_DATE)
    lcd.print("Data: ");
  lcd.print(weekDay[weekday()-1]);
  lcd.print(", ");

  if(day() < 10)
    lcd.print("0");
  lcd.print( day() );
  printDateSeparator(printConfigs);
  
  if(month() < 10)
    lcd.print("0");
  lcd.print( month() );
  printDateSeparator(printConfigs);

  lcd.print( year()-2000 );
}



void printDateSeparator(byte printConfigs){
  if(printConfigs & DOT_SEPARATOR)
    lcd.print(".");
  else if(printConfigs & DASH_SEPARATOR)
    lcd.print("-");
  else
    lcd.print("/");
}

