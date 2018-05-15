#include <PushButton.h>
#include <LiquidCrystal.h>    //Display LCD alfanumérico
#include <Wire.h>             //Interface I2C
#include <Time.h>             //Tempo
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307

#define PRINT_SECONDS         B00000001
#define PRINT_TEXT_DATE       B00000010
#define PRINT_TEXT_HOUR       B00000100
#define DOT_SEPARATOR         B00001000
#define DASH_SEPARATOR        B00010000

void printDate(int col, int row, byte printConfigs);
void printTime(int col, int row, byte printConfigs);
void printDateSeparator(byte printConfigs);
void printProjectName(int col, int row);
void printProjectVersion(int col, int row);
String mainMenu();
void setTimeMenu();
void setDateMenu();
void OutputsMenu();

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

// Caractere especial para LCD
byte block[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };

// Instancia objeto para botão de MENU
PushButton menuBtn(menuBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton upBtn(upBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton downBtn(downBtnPin, 50, DEFAULT_STATE_HIGH);


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
    delay(200);
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

  //Taxa de atualização da biblioteca Time com o RTC (segundos)
  setSyncInterval(60);

  //Limpa LCD e reescreve nome no topo, centralizado
  lcd.clear();
  printProjectName(5,0);
}



/********************************************************************************
 * 
 * Função LOOP - É repetida continuamente pelo firmware.
 * Após o boot, escreve data e hora na tela de acordo com os parâmetros da função
 * Testa botões de MENU ou de INFORMAÇÕES
 * 
 *******************************************************************************/
void loop(){
 String menuControl = "";
 unsigned long count = 0;

  if( menuBtn.isPressed() ){
    menuControl = mainMenu();
    Serial.println(menuControl);

    if(menuControl == "Exit"){
      printProjectName(5,0);
      printTime(0,2, PRINT_TEXT_HOUR);
      printDate(0,3, PRINT_TEXT_DATE | DOT_SEPARATOR);
      while( menuBtn.isPressed() );
    }
    else if(menuControl == "Set Time"){
      setTimeMenu();
    }
    else if(menuControl == "Set Date"){
      setDateMenu();
    }
    else if(menuControl == "Outputs"){
      OutputsMenu();
    }
  }

  // Só atualiza LCD depois de 1 segundo
  // Não bloqueia execução do loop
  if( (millis() - count) > 1000 ){
    printProjectName(5,0);
    printTime(0,2, PRINT_TEXT_HOUR);
    printDate(0,3, PRINT_TEXT_DATE | DOT_SEPARATOR);
    count = millis();
  }

}


/******************************************************************************
 * 
 *
 *****************************************************************************/
void printDate(int col, int row, byte printConfigs){
const String weekDay[7] = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};

  //Imprime hora atual na segunda linha do LCD
  lcd.setCursor(col,row);
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
void printTime(int col, int row, byte printConfigs){
  
  //Imprime hora atual na primeira linha do LCD
  lcd.setCursor(col,row);
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

/******************************************************************************
 * 
 * Função MAIN MENU - cria o menu principal na tela
 * Retorna uma String com o menu que foi selecionado
 *
 *****************************************************************************/
String mainMenu(){
 int numberOfOptions = 6;
 const String options[numberOfOptions] = {"Set Time", "Set Date", "Outputs", "Watering", "Sensors", "Exit"};
 const int lines = 3;
 const char selector = '>';
 int selectorPosition = 1;
 int page = 0, lastPage = 1;
 int maxPages = numberOfOptions - lines;
 bool exitMenu = false;

  while(!exitMenu){

    //Verifica se houve mudança de página
    if(page != lastPage){

      lastPage = page;
      
      lcd.clear();
      lcd.setCursor(8,0);
      lcd.print("MENU");
    
      lcd.setCursor(2,1);
      lcd.print(options[ (lines-3)+page ]);
      
      lcd.setCursor(2,2);
      lcd.print(options[ (lines-2)+page ]);
      
      lcd.setCursor(2,3);
      lcd.print(options[ (lines-1)+page ]);

      if(selectorPosition == 1){
        lcd.setCursor(1,1);
        lcd.print(selector);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(selector);
      }
      else if(selectorPosition == 3){
        lcd.setCursor(1,3);
        lcd.print(selector);
      }

      while(menuBtn.isPressed());
      delay(800);
    }

    //Botão MENU
    if( menuBtn.isPressed() ){
      exitMenu = true;
    }

    //Botão UP
    else if( upBtn.isPressed() ){
      if(selectorPosition == 3){
        lcd.setCursor(1,3);
        lcd.print(" ");
        lcd.setCursor(1,2);
        lcd.print(selector);
        selectorPosition = 2;
        delay(800);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(" ");
        lcd.setCursor(1,1);
        lcd.print(selector);
        selectorPosition = 1;
        delay(800);
      }
      else if(selectorPosition == 1){
        if(page > 0)
          page--;
        else{
          page = maxPages;
          selectorPosition = 3;
        }
      }
    }

    //Botão DOWN
    else if( downBtn.isPressed() ){
      if(selectorPosition == 1){
        lcd.setCursor(1,1);
        lcd.print(" ");
        lcd.setCursor(1,2);
        lcd.print(selector);
        selectorPosition = 2;
        delay(800);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(" ");
        lcd.setCursor(1,3);
        lcd.print(selector);
        selectorPosition = 3;
        delay(800);
      }
      else if(selectorPosition == 3){
        if(page < maxPages)
          page++;
        else{
          page = 0;
          selectorPosition = 1;
        }
      }
    }//else if
    
  }//while

  lcd.clear();
  return options[ page + selectorPosition - 1 ];
  
}//mainMenu()


/******************************************************************************
 * 
 * Função SET TIME MENU - cria menu para ajustar hora na tela
 *
 *****************************************************************************/
void setTimeMenu(){
 int countBtnClicks = 0;
 bool exitMenu = false;
 int myHour = hour();
 int myMinute = minute();
 int mySecond = second();
 int lastHour = myHour;
 int lastMin = myMinute;
 int lastSec = mySecond;

  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SET TIME");
  printTime(6,2,PRINT_SECONDS);
  lcd.setCursor(1,3);
  lcd.print("Save");
  lcd.setCursor(14,3);
  lcd.print("Cancel");
  
  while( menuBtn.isPressed() );
  lcd.blink();

  
  while( !exitMenu ){

    if( menuBtn.isPressed() ){
      if(countBtnClicks < 4)
        countBtnClicks++;
      else{
        countBtnClicks = 0;
        lcd.setCursor(13,3);
        lcd.print(" ");
        lcd.blink();
      }
      while( menuBtn.isPressed() );       
    }

    //Cursor piscando sobre as horas
    if(countBtnClicks == 0){
      if(myHour != lastHour){
        lastHour = myHour;      
        lcd.setCursor(6,2);
        if(myHour < 10)
          lcd.print("0");
        lcd.print(myHour);
      }
      lcd.setCursor(7,2);

      if( upBtn.isPressed() ){
        if(myHour < 23)
          myHour++;
        else
          myHour = 0;
        while( upBtn.isPressed() );
      }
      else if( downBtn.isPressed() ){
        if(myHour > 0)
          myHour--;
        else
          myHour = 23;
        while( downBtn.isPressed() );
      } 
    }
    
    //Cursor piscando sobre os minutos
    else if(countBtnClicks == 1){
      if(myMinute != lastMin){
        lastMin = myMinute;      
        lcd.setCursor(9,2);
        if(myMinute < 10)
          lcd.print("0");
        lcd.print(myMinute);
      }
      lcd.setCursor(10,2);

      if( upBtn.isPressed() ){
        if(myMinute < 59)
          myMinute++;
        else
          myMinute = 0;
        while( upBtn.isPressed() );
      }
      else if( downBtn.isPressed() ){
        if(myMinute > 0)
          myMinute--;
        else
          myMinute = 59;
        while( downBtn.isPressed() );
      }
    }
    
    //Cursor piscando sobre os segundos
    else if(countBtnClicks == 2){
      if(mySecond != lastSec){
        lastSec = mySecond;      
        lcd.setCursor(12,2);
        if(mySecond < 10)
          lcd.print("0");
        lcd.print(mySecond);
      }
      lcd.setCursor(13,2);

      if( upBtn.isPressed() ){
        if(mySecond < 59)
          mySecond++;
        else
          mySecond = 0;
        while( upBtn.isPressed() );
      }
      else if( downBtn.isPressed() ){
        if(mySecond > 0)
          mySecond--;
        else
          mySecond = 59;
        while( downBtn.isPressed() );
      }
    }
    //Cursor na opção Set Time
    else if(countBtnClicks == 3){
      lcd.noBlink();
      lcd.setCursor(0,3);
      lcd.print('>');
    }
    //Cursor na opção Cancel
    else if(countBtnClicks == 4){
      lcd.setCursor(0,3);
      lcd.print(" ");
      lcd.setCursor(13,3);
      lcd.print('>');
    }
    
  }

  lcd.noBlink();
}


void setDateMenu(){

}



void OutputsMenu(){

}

