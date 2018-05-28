#include <PushButton.h>       //Debounce dos botões
#include <LiquidCrystal.h>    //Display LCD alfanumérico
#include <Wire.h>             //Interface I2C
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307


// Flag para habilitar depuração via Monitor Serial da Arduino IDE
#define DEBUG false


// Estrutura p/ armazenar horário e dias para ligar/desligar coisas
// São 14 bytes para armazenamento
typedef struct{
  byte onHour;
  byte onMinute;
  byte onSecond;
  byte offHour;
  byte offMinute;
  byte offSecond;
  bool sun;
  bool mon;
  bool tue;
  bool wed;
  bool thu;
  bool fri;
  bool sat;
  bool enable;
} turnOnOff;

// Estrutura p/ definir posições no LCD
typedef struct{
  int col;
  int row;
} LcdPosition;


// Configurações para printar hora e data no LCD
#define PRINT_SECONDS         B00000001
#define PRINT_TEXT_DATE       B00000010
#define PRINT_TEXT_TIME       B00000100
#define PRINT_WEEK_DAY        B00001000
#define PRINT_FULL_YEAR       B00010000
#define DOT_SEPARATOR         B00100000
#define DASH_SEPARATOR        B01000000


// Opções para retorno das funções
#define ENABLE_OPT      496
#define DISABLE_OPT     902
#define CONFIG_OPT      277
#define EXIT_MENU       333
#define EPIC_FAIL       666


// Protótipo de Funções
void printMainView();
void printDate(int col, int row, byte configs);
void printTime(int col, int row, byte configs);
void updateScreenDate(int col, int row, byte configs);
void updateScreenTime(int col, int row, byte configs);
void printTimeDigits(int digits);
void printDateDigits(int digits, byte configs);
void printProjectName(int col, int row);
void printProjectVersion(int col, int row);
String mainMenu();
void setDateTimeMenu(String option);
int lightsMenu();
int enableOutput(int num);
String setOnOffTime(int num);
String setOnOffDays(int num);
void printSetTimeView();
void printSetDateView();
void printSaveCancelOptions();
void printErrorMsg(int num, String msg);
#if DEBUG
  void serialClockDisplay();
  void printDigits(int digits);
#endif


// Configurações para LCD
// Número de linhas e de colunas, Pinos a serem usados
// Instância do objeto LiquidCrystal, caracteres especiais
uint8_t lcdNumCol = 20, lcdNumRow = 4;
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte defaultPrintConfigs =  PRINT_SECONDS | PRINT_TEXT_DATE | PRINT_TEXT_TIME | DOT_SEPARATOR | PRINT_WEEK_DAY;
byte blockChar[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };
byte checkChar[8] = {0B00000,0B00000,0B00000,0B00001,0B10010,0B10100,0B11000,0B00000};


// Configurações dos botões.
// Pinos dos botões e instâncias dos objetos da classe PushButton
const int menuBtnPin = 8;
const int upBtnPin = 9;
const int downBtnPin = 10;
PushButton menuBtn(menuBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton upBtn(upBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton downBtn(downBtnPin, 50, DEFAULT_STATE_HIGH);


//VARIÁVEIS GLOBAIS DE USO GERAL DO PROGRAMA
unsigned long lastUpdate = 0;                         //controla a atualização do LCD na função loop()
tmElements_t tm;                                      //Armazena tempo atual do sistema
turnOnOff light1, light2;                             //Armazena informações de liga/desliga dos dispositivos
int tCol = 0, tRow = 2, dCol = 0, dRow = 3;           //Onde serão impressos data e hora no LCD

// Constantes
const String PROJECT_NAME_STR = "GrowSystem";
const String PROJECT_VERSION_STR = "v1.0";
const int menuNumOpt = 6;
const String menuOptions[menuNumOpt] = {"Set System Time", "Set System Date", "Lights", "Water Pumps", "Sensors", "Exit"};
const String weekDay[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const String SAVE_STR = "Save";
const String NEXT_STR = "Next";
const String BACK_STR = "Back";
const String CANCEL_STR = "Cancel";
const String ENABLED_STR = "Enabled";
const String DISABLED_STR = "Disabled";
const char SELECTOR = '>', BLANK_CHAR = ' ';
const int DELAY_BTN = 400;

// Variáveis para tratamento de erros
int errorNum;
String errorMsg;

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 * 
 * SETUP - Roda uma vez ao inicializar.
 * Configura pinos de entrada e saída dos botões
 * Configura LCD, mostra mensagem e barra de inicialização
 * Ajusta tempo do RTC vi software (opcional)
 * Sincroniza biblioteca Time com periférico externo (RTC)
 * 
 *******************************************************************************/
void setup(){

  #if DEBUG
    Serial.begin(9600);
    Serial.println("Initializing system...");
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  lcd.createChar(0, blockChar);
  lcd.createChar(1, checkChar);
  lcd.begin(lcdNumCol, lcdNumRow);  //Inicializa LCD 20x4
  printProjectName(5,0);            //Imprime nome do projeto
  printProjectVersion(8,1);         //Imprime versão do projeto
  lcd.setCursor(0,3);
  for(int i = 0; i < 20; i++){      //Cria animação de barra de carregamento (loading)
    lcd.write(byte(0));
    delay(200);
  }
  
  //Ajusta o tempo do sistema e atualiza no RTC
  //Uma vez tendo ajustado o tempo no RTC, as duas linhas seguintes podem ser comentadas
  //setTime(23,59,40,31,12,2018);         //horas, minutos, segundos, dia, mês, ano
  //RTC.set( now() );

  //Sincroniza a biblioteca Time com a data e hora do RTC
  //Caso a sincronização tenha falhado, exibe mensagem de erro no LCD e trava execução
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet){
    #if DEBUG
      Serial.println("Unable to sync with the RTC.");
    #endif
    lcd.clear();
    lcd.print("ERRO 743:");
    lcd.setCursor(0,1);
    lcd.print("Nao foi possivel");
    lcd.setCursor(0,2);
    lcd.print("sincronizar com RTC.");
    lcd.setCursor(0,3);
    lcd.print("Verifique o sistema.");
    while(true);
  }
  else{
    setSyncInterval(60);
    
    #if DEBUG
      Serial.println("RTC has set the system time.");
    #endif
  }
  breakTime(now(), tm);
  printMainView();
}

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 * 
 * LOOP - É repetida continuamente pelo firmware.
 * Após o boot, escreve data e hora na tela de acordo com os parâmetros da função
 * Testa botões de MENU ou de INFORMAÇÕES
 * 
 *******************************************************************************/
void loop(){
 String menuOption = "";
 
  if( menuBtn.isPressed() ){
    menuOption = mainMenu();
    #if DEBUG
      Serial.print("MENU option: ");
      Serial.println(menuOption);
    #endif

    //Opção SET SYSTEM TIME
    if(menuOption == menuOptions[0]){
      setDateTimeMenu(menuOption);
      printMainView();
      while( menuBtn.isPressed() );
    }
    //Opção SET SYSTEM DATE
    else if(menuOption == menuOptions[1]){
      setDateTimeMenu(menuOption);
      printMainView();
      while( menuBtn.isPressed() );
    }
    //Opção LIGHTS
    else if(menuOption == menuOptions[2]){
      int output = lightsMenu();  //Retorna 1 (light1), 2 (light2) ou EXIT_MENU
      #if DEBUG
        if(output != EXIT_MENU){
          Serial.print("Config Light ");
          Serial.println(output);
        }
        else
          Serial.println("Exit Menu Lights.");
      #endif
      
      if(output != EXIT_MENU){
        int opt = enableOutput(output);   //Retorna Enable, Disable, Config, Exit
        #if DEBUG
          Serial.print("User option: ");
          Serial.println(opt);
        #endif
        if(opt != EXIT_MENU){
          String optConfig = setOnOffTime(output);
          #if DEBUG
            Serial.print("User option: ");
            Serial.println(optConfig);
          #endif
          if(optConfig == NEXT_STR)
            setOnOffDays(output);
        }
      }
      printMainView();
      while( menuBtn.isPressed() );
    }
    //Opção WATERING
    else if(menuOption == menuOptions[3]){
      printMainView();
      while( menuBtn.isPressed() );
    }
    //Opção SENSORS
    else if(menuOption == menuOptions[4]){
      printMainView();
      while( menuBtn.isPressed() );
    }
    //Opção EXIT
    else if(menuOption == menuOptions[5]){
      printMainView();
      while( menuBtn.isPressed() );
    }
  }

  // Só atualiza LCD depois de 1 segundo
  // Não bloqueia execução do loop
  if( (millis() - lastUpdate) > 1000 ){
    lastUpdate = millis();
    updateScreenTime(tCol, tRow, defaultPrintConfigs);
    updateScreenDate(dCol, dRow, defaultPrintConfigs);
    
    #if DEBUG
      time_t currentTime = RTC.get();
      if(currentTime == 0){
        Serial.println("Cannot read current time from RTC.");
      }
      else{
        Serial.print("Current time on RTC: ");
        Serial.print(currentTime);
        Serial.print(" - ");
        serialClockDisplay();
      }
    #endif
  }

}//loop()

/******************************************************************************
 * 
 * DEFAULT VIEW - Nome do projeto centralizado no topo, data e hora abaixo
 *
 *****************************************************************************/
void printMainView(){
  lcd.clear();
  printProjectName(5,0);
  printTime(tCol, tRow, defaultPrintConfigs);
  printDate(dCol, dRow, defaultPrintConfigs);
}
/******************************************************************************
 * 
 * PRINT DATE - Imprime a data no LCD na posição indicada por col e row
 *
 *****************************************************************************/
void printDate(int col, int row, byte configs){
  lcd.setCursor(col,row);
  if(configs & PRINT_TEXT_DATE)
    lcd.print("Date: ");
  if(configs & PRINT_WEEK_DAY){
    lcd.print(weekDay[weekday()-1]);
    lcd.print(", ");
  }
  if(day() < 10)
    lcd.print("0");
  lcd.print(day());
  printDateDigits(month(), configs);
  if(configs & PRINT_FULL_YEAR)
    printDateDigits(year(), configs);
  else
    printDateDigits(year()-2000, configs);
}

/******************************************************************************
 * 
 * UPDATE DATE ON SCREEN - atualiza data no LCD
 * Os parâmetros col e row devem descrever a primeira posição da data na tela,
 * mesmo que ela possua a palavra "Date: " na frente.
 * A função faz o offset necessário.
 * 
 * Essa função atualiza apenas o dia da semana e os dígitos da data quando 
 * se faz necessário. 
 * Uma comparação é feita entre a data atual e os dados armazenados na
 * variável global "tmElements_t tm".
 *
 *****************************************************************************/
void updateScreenDate(int col, int row, byte configs){
  int offset = col;

  if(configs & PRINT_TEXT_DATE)
    offset += 6;
    
  if(configs & PRINT_WEEK_DAY){
    if(weekday() != tm.Wday){
      tm.Wday = weekday();
      lcd.setCursor(offset, row); //offset 0 ou 6
      lcd.print(weekDay[weekday()-1]);
      lcd.print(", ");
    }
    offset += 5;
  }
  

  if(day() != tm.Day){
    tm.Day = day();
    lcd.setCursor(offset, row); //offset 0, 5, 6 ou 11
    if(day() < 10)
      lcd.print("0");
    lcd.print(day());
  }
  offset += 2;
  
  if(month() != tm.Month){
    tm.Month = month();
    lcd.setCursor(offset, row); //coluna 13
    printDateDigits(month(), configs);
  }
  offset += 3;
  
  if(year()-1970 != tm.Month){
    tm.Year = year()-1970;
    lcd.setCursor(offset, row); //coluna 16
    printDateDigits(year()-2000, configs);
  }
}

/******************************************************************************
 * 
 * PRINT TIME - Imprime a hora no LCD na posição indicada por col e row
 *
 *****************************************************************************/
void printTime(int col, int row, byte configs){
  lcd.setCursor(col,row);
  if(configs & PRINT_TEXT_TIME)
    lcd.print("Time: ");
  
  if(hour() < 10)
    lcd.print("0");
  lcd.print(hour());
  printTimeDigits(minute());
  if(configs & PRINT_SECONDS)
    printTimeDigits(second());
}

/******************************************************************************
 * 
 * UPDATE TIME ON SCREEN - atualiza a hora no LCD
 * Os parâmetros col e row devem descrever a primeira posição da hora na tela, 
 * mesmo que ela possua a palavra "Time: " na frente.
 * A função faz o offset necessário.
 * 
 * Essa função atualiza apenas os dígitos da hora quando se faz necessário. 
 * Uma comparação é feita entre a hora atual e os dados armazenados na
 * variável global "tmElements_t tm".
 *
 *****************************************************************************/
void updateScreenTime(int col, int row, byte configs){
  int offset = col;
  
  if(configs & PRINT_TEXT_TIME)
    offset += 6;
    
  if(hour() != tm.Hour){
    tm.Hour = hour();
    lcd.setCursor(offset, row); //coluna 6
    if(hour() < 10)
      lcd.print("0");
    lcd.print(hour());
  }
  offset += 2;

  if(minute() != tm.Minute){
    tm.Minute = minute();
    lcd.setCursor(offset, row); //colua 8
    printTimeDigits(minute());
  }
  offset += 3;

  if(configs & PRINT_SECONDS)
    if(second() != tm.Second){
      tm.Second = second();
      lcd.setCursor(offset, row); //coluna 11
      printTimeDigits(second());
    }
}

/******************************************************************************
 * Função auxiliar para imprimir os dígitos da data com o separador
 *****************************************************************************/
void printDateDigits(int digits, byte configs){
  if(configs & DOT_SEPARATOR){
    lcd.print(".");
    if(digits < 10)
      lcd.print("0");
    lcd.print(digits);
  }
  else if(configs & DASH_SEPARATOR){
    lcd.print("-");
    if(digits < 10)
      lcd.print("0");
    lcd.print(digits);
  }
  else{
    lcd.print("/");
    if(digits < 10)
      lcd.print("0");
    lcd.print(digits);
  }
}
/****************************************************************************** 
 * Função auxiliar para imprimir os dígitos da hora com o separador :
 *****************************************************************************/
void printTimeDigits(int digits){
  lcd.print(":");
  if(digits < 10)
    lcd.print("0");
  lcd.print(digits);
}
void printProjectName(int col, int row){
    lcd.setCursor(col,row);
    lcd.print(PROJECT_NAME_STR);
}
void printProjectVersion(int col, int row){
    lcd.setCursor(col,row);
    lcd.print(PROJECT_VERSION_STR);
}




/******************************************************************************
 ******************************************************************************
 ******************************************************************************
 * 
 * Função MAIN MENU - cria o menu principal na tela
 * Retorna uma String com o menu que foi selecionado
 *
 *****************************************************************************/
String mainMenu(){
 const int lines = 3;
 int selectorPosition = 1;
 int page = 0, lastPage = 1;
 int maxPages = menuNumOpt - lines;
 bool exitMenu = false;

  #if DEBUG
    Serial.println("MAIN MENU");
  #endif

  while(!exitMenu){

    //Verifica se houve mudança de página
    if(page != lastPage){
      lastPage = page;
      lcd.clear();
      lcd.setCursor(8,0);
      lcd.print("MENU");
      lcd.setCursor(2,1);
      lcd.print(menuOptions[ (lines-3)+page ]);
      lcd.setCursor(2,2);
      lcd.print(menuOptions[ (lines-2)+page ]);
      lcd.setCursor(2,3);
      lcd.print(menuOptions[ (lines-1)+page ]);

      if(selectorPosition == 1){
        lcd.setCursor(1,1);
        lcd.print(SELECTOR);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(SELECTOR);
      }
      else if(selectorPosition == 3){
        lcd.setCursor(1,3);
        lcd.print(SELECTOR);
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
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,2);
        lcd.print(SELECTOR);
        selectorPosition = 2;
        delay(800);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,1);
        lcd.print(SELECTOR);
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
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,2);
        lcd.print(SELECTOR);
        selectorPosition = 2;
        delay(800);
      }
      else if(selectorPosition == 2){
        lcd.setCursor(1,2);
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,3);
        lcd.print(SELECTOR);
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
  return menuOptions[ page + selectorPosition - 1 ];
  
}//mainMenu()


/******************************************************************************
 * 
 * Função SET DATE/TIME MENU - cria menu para ajustar hora ou data na tela
 *
 *****************************************************************************/
void setDateTimeMenu(String option){
 int countBtnClicks = 0;
 bool exitMenu = false;
 int myHour = hour(), myMinute = minute(), mySecond = second();
 int lastHour = myHour, lastMin = myMinute, lastSec = mySecond;
 int myDay = day(), myMonth = month(), myYear = year();
 int lastDay = myDay, lastMonth = myMonth, lastYear = myYear;

  if(option == menuOptions[0])          //Set System Time
    printSetTimeView();
  else if(option == menuOptions[1])     //Set System Date
    printSetDateView();
  else{
    errorNum = 21;
    errorMsg = "Wrong parameter in function SetDateTimeMenu().";
    printErrorMsg(errorNum, errorMsg);
  }
  while( menuBtn.isPressed() );
  lcd.blink();

  while( !exitMenu ){
    
    if( menuBtn.dualFunction() == 1 ){      //Click NORMAL (muda de opção no menu)
      if(countBtnClicks < 4)
        countBtnClicks++;
      else{
        countBtnClicks = 0;
        lcd.setCursor(13,3);                //Apaga seletor que estava na opção Cancel
        lcd.print(BLANK_CHAR);
        lcd.blink();
      }
    }
    else if( menuBtn.dualFunction() == -1 ){    //click LONGO (enter)
      if(countBtnClicks == 3){                  //Opção SAVE
        if(option == menuOptions[0])
          setTime(myHour, myMinute, mySecond, day(), month(), year());
        else if(option == menuOptions[1])
          setTime(hour(), minute(), second(), myDay, myMonth, myYear);  
        tmElements_t myTime;
        breakTime(now(), myTime);
        
        #if DEBUG
          Serial.print("Time and date setted by user: ");
          serialClockDisplay();
        #endif
        
        if( RTC.write(myTime) ){
          printSuccessMsg(option);
          #if DEBUG
            Serial.println("OK! Time/Date successfully adjusted in RTC.");
          #endif
        }
        else {
          errorNum = 721;
          errorMsg = "Cannot update RTC.";
          printErrorMsg(errorNum, errorMsg);
          delay(1500);
          #if DEBUG
            Serial.println("ERROR! Cannot update Time/Date in RTC.");
          #endif
        }
        exitMenu = true;
      }
      else if(countBtnClicks == 4){             //Opção CANCEL
        exitMenu = true;
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre as HORAS ou DIA
    if(countBtnClicks == 0){
      if(option == menuOptions[0]){           //Ajuste das HORAS
        if(myHour != lastHour){           //Só atualiza LCD se for necessário
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
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(myHour > 0)
            myHour--;
          else
            myHour = 23;
          delay(DELAY_BTN);
        }
      }     
      else if(option == menuOptions[1]){      //Ajuste do DIA
        if(myDay != lastDay){             //Só atualiza LCD se for necessário
          lastDay = myDay;      
          lcd.setCursor(5,2);
          if(myDay < 10)
            lcd.print("0");
          lcd.print(myDay);
        }
        lcd.setCursor(6,2);
  
        if( upBtn.isPressed() ){
          if(myDay < 31)
            myDay++;
          else
            myDay = 1;
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(myDay > 1)
            myDay--;
          else
            myDay = 31;
          delay(DELAY_BTN);
        }
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre os MINUTOS ou MÊS
    else if(countBtnClicks == 1){
      if(option == menuOptions[0]){           //Ajuste das MINUTOS
        if(myMinute != lastMin){          //Só atualiza LCD se for necessário
          lastMin = myMinute;      
          lcd.setCursor(8,2);
          printTimeDigits(myMinute);
        }
        lcd.setCursor(10,2);
  
        if( upBtn.isPressed() ){
          if(myMinute < 59)
            myMinute++;
          else
            myMinute = 0;
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(myMinute > 0)
            myMinute--;
          else
            myMinute = 59;
          delay(DELAY_BTN);
        }
      }
      else if(option == menuOptions[1]){      //Ajuste do MÊS
        if(myMonth != lastMonth){         //Só atualiza LCD se for necessário
          lastMonth = myMonth;
          lcd.setCursor(7,2);
          printDateDigits(myMonth, DOT_SEPARATOR);
        }
        lcd.setCursor(9,2);
  
        if( upBtn.isPressed() ){
          if(myMonth < 12)
            myMonth++;
          else
            myMonth = 1;
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(myMonth > 1)
            myMonth--;
          else
            myMonth = 12;
          delay(DELAY_BTN);
        }
      }
    }

    //------------------------------------------------------------------------------
    //Cursor piscando sobre os SEGUNDOS ou ANO
    else if(countBtnClicks == 2){
      if(option == menuOptions[0]){           //Ajuste dos SEGUNDOS
        if(mySecond != lastSec){          //Só atualiza LCD se for necessário
          lastSec = mySecond;      
          lcd.setCursor(11,2);
          printTimeDigits(mySecond);
        }
        lcd.setCursor(13,2);
  
        if( upBtn.isPressed() ){
          if(mySecond < 59)
            mySecond++;
          else
            mySecond = 0;
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(mySecond > 0)
            mySecond--;
          else
            mySecond = 59;
          delay(DELAY_BTN);
        }
      }
      else if(option == menuOptions[1]){      //Ajuste do ANO
        if(myYear != lastYear){           //Só atualiza LCD se for necessário
          lastYear = myYear;
          lcd.setCursor(10,2);
          printDateDigits(myYear, DOT_SEPARATOR);
        }
        lcd.setCursor(14,2);
  
        if( upBtn.isPressed() ){
          if(myYear < 2099)
            myYear++;
          else
            myYear = 2015;
          delay(DELAY_BTN);
        }
        else if( downBtn.isPressed() ){
          if(myYear > 2015)
            myYear--;
          else
            myYear = 2099;
          delay(DELAY_BTN);
        }
      }
    }

    //------------------------------------------------------------------------------
    //Cursor na opção Save
    else if(countBtnClicks == 3){
      lcd.noBlink();
      lcd.setCursor(0,3);
      lcd.print(SELECTOR);
    }

    //------------------------------------------------------------------------------
    //Cursor na opção Cancel
    else if(countBtnClicks == 4){
      lcd.setCursor(0,3);               //Apaga o seletor que estava na opção Save
      lcd.print(BLANK_CHAR);
      lcd.setCursor(13,3);              //Escreve novo seletor na opção Cancel
      lcd.print(SELECTOR);
    }
    
  }//while
  lcd.noBlink();
}//setDateTimeMenu()

void printSetTimeView(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SET TIME");
  printTime(6,2,PRINT_SECONDS);
  printSaveCancelOptions();
}
void printSetDateView(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SET DATE");
  printDate(5,2,DOT_SEPARATOR | PRINT_FULL_YEAR);
  printSaveCancelOptions();
}
void printSaveCancelOptions(){
  lcd.setCursor(1,3);
  lcd.print("Save");
  lcd.setCursor(14,3);
  lcd.print("Cancel");
}
void printSuccessMsg(String option){
  lcd.clear();
  lcd.setCursor(8,0);
  lcd.print("OK");
  lcd.setCursor(1,2);
  if(option == "Set Time")
    lcd.print("Time ");
  else if(option == "Set Date")
    lcd.print("Date ");
  lcd.print("successfully");
  lcd.setCursor(6,3);
  lcd.print("adjusted");
  delay(1500);
}

/******************************************************************************
 * 
 * Função OUTPUTS MENU - Cria menu para configurar saídas do sistema
 *
 *****************************************************************************/
void OutputsMenu(){

}




void printErrorMsg(int num, String msg){
  num++;
  msg += ".";
}





// Funções auxiliares para debug que imprimem relógio e calendário via porta Serial
#if DEBUG
void serialClockDisplay(){
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print("  ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year()); 
  Serial.println(); 
}
void printDigits(int digits){
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
#endif
