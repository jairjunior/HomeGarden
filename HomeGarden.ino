#include <PushButton.h>
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

// Strings contendo nome e versão do projeto
const String projectName = "GrowSystem";
const String projectVersion = "v1.0";

// Pinos dos botões
const int menuBtnPin = 8;
const int upBtnPin = 9;
const int downBtnPin = 10;

// Pinos usados pelo display LCD
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;

// Instancia objeto do tipo LyquidCrystal
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Instancia objeto para botão de MENU
PushButton menuBtn(menuBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton upBtn(upBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton downBtn(downBtnPin, 50, DEFAULT_STATE_HIGH);

byte block[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };



/********************************************************************************
 * 
 * Função SETUP - Roda uma vez ao inicializar.
 * Configura pinos de entrada e saída dos botões
 * Configura LCD, mostra mensagem e barra de inicialização
 * Ajusta tempo do RTC vi software (opcional)
 * Sincroniza biblioteca Time com periférico externo (RTC)
 * 
 *******************************************************************************/
void setup(){

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  lcd.createChar(0, block);
  lcd.begin(20,4);              //Inicializa LCD 20x4
  printProjectName(5,0);        //Imprime nome do projeto
  printProjectVersion(8,1);     //Imprime versão do projeto
  lcd.setCursor(0,3);           //Cria animação de barra de carregamento (loading)
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
  //Se a sincronização ocorreu, escreve nome do projeto no topo LCD
  else{
    lcd.clear();
    printProjectName(5,0);
  }

  //Taxa de atualização da biblioteca Time com o RTC (segundos)
  setSyncInterval(60);
}



/********************************************************************************
 * 
 * Função LOOP - É repetida continuamente pelo firmware.
 * Após o boot, escreve data e hora na tela de acordo com os parâmetros da função
 * Testa botões de MENU ou de INFORMAÇÕES
 * 
 *******************************************************************************/
void loop(){
unsigned long count = 0;

  // Só atualiza LCD depois de 1 segundo
  // Não bloqueia execução do loop
  if( (millis() - count) > 1000 ){
    printDateAndHour(0,3, 0,2, PRINT_TEXT_DATE | PRINT_TEXT_HOUR | DOT_SEPARATOR);
    count = millis();
  }

}


/******************************************************************************
 * 
 *
 *****************************************************************************/
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

  if(printConfigs & PRINT_SECONDS){
    lcd.print(":");
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

/******************************************************************************
 * 
 *
 *****************************************************************************/
void printDateSeparator(byte printConfigs){
  if(printConfigs & DOT_SEPARATOR)
    lcd.print(".");
  else if(printConfigs & DASH_SEPARATOR)
    lcd.print("-");
  else
    lcd.print("/");
}

/******************************************************************************
 * 
 *
 *****************************************************************************/
void printProjectName(int col, int row){
    lcd.setCursor(col,row);
    lcd.print(projectName);
}

/******************************************************************************
 * 
 *
 *****************************************************************************/
void printProjectVersion(int col, int row){
    lcd.setCursor(col,row);
    lcd.print(projectVersion);
}
