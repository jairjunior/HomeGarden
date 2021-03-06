#include <PushButton.h>       //Debounce dos botões
#include <LiquidCrystal.h>    //Display LCD alfanumérico
#include <Wire.h>             //Interface I2C
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307

// Flag para habilitar depuração via Monitor Serial da Arduino IDE
#define DEBUG true

// Estrutura p/ armazenar horário e dias para ligar/desligar coisas
typedef struct{
  byte onHour;
  byte onMinute;
  byte onSecond;
  byte offHour;
  byte offMinute;
  byte offSecond;
  byte daysOfWeek;
  bool enable;
  int pin;
  String name;
} TurnOnOff_t;

// Estrutura p/ definir posições no LCD
typedef struct{
  int col;
  int row;
} LcdPosition_t;

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
#define CONFIG_OPT      263
#define CANCEL_OPT      671
#define NEXT_OPT        523
#define SAVE_OPT        777
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
int lightsMenu();
int enableOutput(TurnOnOff_t *output);
int setOnOffTime(TurnOnOff_t *output);
int setOnOffDays(TurnOnOff_t *output);
void setSystemTime(String option);
void printSetTimeView();
void printSetDateView();
void printSaveCancelOptions();
void printErrorMsg(int num, String msg);
#if DEBUG
  void serialClockDisplay();
  void printDigits(int digits);
#endif

// Configurações para LCD
uint8_t lcdNumCol = 20, lcdNumRow = 4;
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte defaultPrintConfigs =  PRINT_SECONDS | PRINT_TEXT_DATE | PRINT_TEXT_TIME | DOT_SEPARATOR | PRINT_WEEK_DAY;
byte blockChar[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };
byte checkChar[8] = { B00000,B00000,B00001,B10010,B10100,B11000,B00000,B00000 };

// Configurações dos botões.
const int menuBtnPin = 8;
const int upBtnPin = 9;
const int downBtnPin = 10;
PushButton menuBtn(menuBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton upBtn(upBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton downBtn(downBtnPin, 50, DEFAULT_STATE_HIGH);

//VARIÁVEIS GLOBAIS DE USO GERAL DO PROGRAMA
tmElements_t tm;                              //Armazena tempo atual do sistema
TurnOnOff_t lights[2];                        //Hora e dias p/ ligar/desligar luzes
TurnOnOff_t onOffTemp;                        //Informações temporária
unsigned long lastUpdate = 0;                 //Controla a atualização do LCD na função loop()
int tCol = 0, tRow = 2, dCol = 0, dRow = 3;   //Onde serão impressos data e hora no LCD

// Constantes do programa
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
 * SETUP - Roda uma vez ao inicializar.
 * Configura pinos de entrada e saída dos botões
 * Configura LCD, mostra mensagem e barra de inicialização
 * Ajusta tempo do RTC vi software (opcional)
 * Sincroniza biblioteca Time com periférico externo (RTC)
 *******************************************************************************/
void setup(){

  #if DEBUG
    Serial.begin(9600);
    Serial.println("Initializing system...");
  #endif

  // Configura pinos para as lâmpadas como saída
  lights[0].name = "Light 1";
  lights[1].name = "Light 2";
  lights[0].pin = 11;
  lights[1].pin = 12;
  pinMode(lights[0].pin, OUTPUT);
  pinMode(lights[1].pin, OUTPUT);

  lcd.createChar(0, blockChar);
  lcd.createChar(1, checkChar);
  lcd.begin(lcdNumCol, lcdNumRow);  //Inicializa LCD 20x4
  printProjectName(5,0);            //Imprime nome do projeto
  printProjectVersion(8,1);         //Imprime versão do projeto
  lcd.setCursor(0,3);
  for(int i = 0; i < 20; i++){      //Cria animação de barra de carregamento (loading)
    lcd.write(byte(0));
    delay(200);
    digitalWrite(lights[0].pin, !digitalRead(lights[0].pin));
    digitalWrite(lights[1].pin, !digitalRead(lights[1].pin));
  }
  
  //Ajusta o tempo do sistema e atualiza no RTC
  //Uma vez tendo ajustado o tempo no RTC, as duas linhas seguintes podem ser comentadas
  //setTime(23,59,40,31,12,2018);         //horas, minutos, segundos, dia, mês, ano
  //RTC.set( now() );

  //Sincroniza a biblioteca Time com a data e hora do RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet){
    errorNum = 743;
    errorMsg = "Unable to sync RTC";
    printErrorMsg(errorNum, errorMsg);
    #if DEBUG
      Serial.println("Unable to sync with the RTC.");
    #endif
    while(true);
  }
  else{
    setSyncInterval(60);
    #if DEBUG
      Serial.println("RTC has set the system time.");
    #endif
  }
  printMainView();
}

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 * LOOP - É repetida continuamente pelo firmware.
 * Após o boot, escreve data e hora na tela de acordo com os parâmetros da função
 * Testa botões de MENU ou de INFORMAÇÕES
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
      setSystemTime(menuOption);
      printMainView();
    }
    //Opção SET SYSTEM DATE
    else if(menuOption == menuOptions[1]){
      setSystemTime(menuOption);
      printMainView();
    }
    //Opção LIGHTS
    else if(menuOption == menuOptions[2]){
      //Retorna 0 (lights[0]), 1 (light[1]) ou EXIT_MENU
      int lightNum = lightsMenu();
      if (lightNum != EXIT_MENU){
        //Retorna ENABLE_OPT, DISABLE_OPT, CONFIG_OPT, EXIT_MENU
        int enableOpt = enableOutput(&lights[lightNum]);
        if( (enableOpt == ENABLE_OPT) || (enableOpt == CONFIG_OPT) ){
          //Retorna NEXT_OPT ou CANCEL_OPT
          int optTime = setOnOffTime(&lights[lightNum]);
          if (optTime == NEXT_OPT){
            //Retorna SAVE_OPT ou CANCEL_OPT
            int optDays = setOnOffDays(&lights[lightNum]);
            if (optDays == SAVE_OPT)
              lights[lightNum] = onOffTemp;
          }
        }
      }
      printMainView();
    }
    //Opção WATERING
    else if(menuOption == menuOptions[3]){
      printMainView();
    }
    //Opção SENSORS
    else if(menuOption == menuOptions[4]){
      printMainView();
    }
    //Opção EXIT
    else if(menuOption == menuOptions[5]){
      printMainView();
    }
    while( menuBtn.isPressed() );
  }

  // Só atualiza LCD a cada 1 segundo - Não bloqueia execução do loop
  if( (millis() - lastUpdate) > 1000 ){
    lastUpdate = millis();
    updateScreenTime(tCol, tRow, defaultPrintConfigs);
    updateScreenDate(dCol, dRow, defaultPrintConfigs);
    
    unsigned int shift = weekday() - 1;
    if( hour() == (int) lights[0].onHour && 
        minute() == (int) lights[0].onMinute &&
        second() == (int) lights[0].onSecond &&
        lights[0].daysOfWeek & (1 << shift) )
      digitalWrite(lights[0].pin, HIGH);

    if( hour() == (int) lights[0].offHour && 
        minute() == (int) lights[0].offMinute && 
        second() == (int) lights[0].offSecond && 
        lights[0].daysOfWeek & (1 << shift) )
      digitalWrite(lights[0].pin, LOW);

    #if DEBUG
      time_t currentTime = RTC.get();
      if(currentTime == 0){
        Serial.println("Cannot read time from RTC.");
      }
      else{
        Serial.print("Current time on RTC: ");
        //Serial.print(currentTime);
        //Serial.print(" - ");
        serialClockDisplay();
      }
      //serialOutputOnOff(&lights[0]);
      //serialOutputOnOff(&lights[1]);

      /*Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(":");
      Serial.print(second());
      Serial.print(" - ");
      Serial.println(weekday());*/
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
void setSystemTime(String option){
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
    errorMsg = "Wrong parameter in function setSystemTime().";
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
}//setSystemTime()

/******************************************************************************
 * 
 * Funções auxiliares para o ajuste de Hora e Data
 * utilizadas pela função setSystemTime()
 *
 *****************************************************************************/
void printSetTimeView(){
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("SET SYSTEM TIME");
  printTime(6,2,PRINT_SECONDS);
  printSaveCancelOptions();
}
void printSetDateView(){
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("SET SYSTEM DATE");
  printDate(5,2,DOT_SEPARATOR | PRINT_FULL_YEAR);
  printSaveCancelOptions();
}
void printSaveCancelOptions(){
  lcd.setCursor(1,3);
  lcd.print(SAVE_STR);
  lcd.setCursor(14,3);
  lcd.print(CANCEL_STR);
}
void printSuccessMsg(String option){
  lcd.clear();
  lcd.setCursor(8,0);
  lcd.print("OK");
  lcd.setCursor(1,2);
  if(option == menuOptions[0])
    lcd.print("Time ");
  else if(option == menuOptions[1])
    lcd.print("Date ");
  lcd.print("successfully");
  lcd.setCursor(6,3);
  lcd.print("adjusted");
  delay(1500);
}

/******************************************************************************
 * 
 * LIGHTS MENU - Cria menu para configurar as Lâmpadas
 * Após escolher a lâmpada desejada, o usuário irá configurar a hora e os
 * dias da semana para ligar/desligar as lâmpadas
 *
 *****************************************************************************/
int lightsMenu(){
 bool exitMenu = false;
 int selectorPosition = 1;
  
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("LIGHTS CONFIG.");
  lcd.setCursor(2,1);
  lcd.print("Light 1");
  lcd.setCursor(2,2);
  lcd.print("Light 2");
  lcd.setCursor(2,3);
  lcd.print("Exit");
  while( menuBtn.isPressed() );
  
  while(!exitMenu){
    
    if(selectorPosition == 1){
      lcd.setCursor(1,2);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,3);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,1);
      lcd.print(SELECTOR);
    }
    else if(selectorPosition == 2){
      lcd.setCursor(1,1);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,3);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,2);
      lcd.print(SELECTOR);
    }
    else if(selectorPosition == 3){
      lcd.setCursor(1,1);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,2);
      lcd.print(BLANK_CHAR);
      lcd.setCursor(1,3);
      lcd.print(SELECTOR);
    }

    if( upBtn.isPressed() ){
      if(selectorPosition > 1)
        selectorPosition--;
      else
        selectorPosition = 3;
      delay(DELAY_BTN);      
    }
    if( downBtn.isPressed() ){
      if(selectorPosition < 3)
        selectorPosition++;
      else
        selectorPosition = 1;
      delay(DELAY_BTN);
    }
    if( menuBtn.isPressed() ){
      exitMenu = true;
    }
  }//while
  
  if(selectorPosition == 1)
    return 0;
  else if(selectorPosition == 2)
    return 1;
  else
    return EXIT_MENU;
  
}//lightsMenu()

/******************************************************************************
 * 
 * ENABLE/DISABLE OUTPUT - Cria MENU para habilitar ou desabilitar uma saída
 *
 *****************************************************************************/
int enableOutput(TurnOnOff_t *output){
 bool exitMenu = false;
 int page = 0, lastPage = 1, maxPages;
 int cursorPosition = 1, lastCursorPosition = 0;
 const int lines = 2;
 const int numDisabledOpt = 2, numEnabledOpt = 3;
 const String enabledOptions[numEnabledOpt] = {"Config " ,"Disable" ,"Exit   "};
 const String disabledOptions[numDisabledOpt] = {"Enable", "Exit"};

  lcd.clear();
  //Escreve o nome do Output a ser configurado
  //Ajusta variável enable de acordo com o status do Output
  lcd.setCursor(6,0);
  lcd.print(output->name);

  //Escreve o Status do Output (Enabled ou Disabled)
  //E configura o número de páginas de acordo com as opções do menu
  lcd.setCursor(0,1);
  lcd.print("Status: ");
  if(output->enable){
    lcd.print(ENABLED_STR);
    maxPages = numEnabledOpt - lines;       //2 páginas (0 e 1)
  }
  else{
    lcd.print(DISABLED_STR);
    maxPages = numDisabledOpt - lines;      //somente 1 página (page 0)
  }
  while( menuBtn.isPressed() );

  while(!exitMenu){
    //Atualiza as opções do menu de acordo com a página
    if(output->enable){
      if(page != lastPage){
        lastPage = page;
        lcd.setCursor(2,2);
        lcd.print(enabledOptions[ (lines-2)+page ]);
        lcd.setCursor(2,3);
        lcd.print(enabledOptions[ (lines-1)+page ]);
      }
    }
    else{
      if(page != lastPage){
        lastPage = page;
        lcd.setCursor(2,2);
        lcd.print(disabledOptions[ (lines-2)+page ]);
        lcd.setCursor(2,3);
        lcd.print(disabledOptions[ (lines-1)+page ]);
      }
    }
    //Reposiciona cursor quando necessário
    if(cursorPosition == 1){
      if(cursorPosition != lastCursorPosition){
        lastCursorPosition = cursorPosition;
        lcd.setCursor(1,3);
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,2);
        lcd.print(SELECTOR);
      }
    }
    else if(cursorPosition == 2){
      if(cursorPosition != lastCursorPosition){
        lastCursorPosition = cursorPosition;
        lcd.setCursor(1,2);
        lcd.print(BLANK_CHAR);
        lcd.setCursor(1,3);
        lcd.print(SELECTOR);
      }
    }
    //Botão MENU
    if( menuBtn.isPressed() )
      exitMenu = true;
    //Botão UP
    else if( upBtn.isPressed() ){
        if(cursorPosition == lines) 
          cursorPosition--;
        else if( (cursorPosition == 1) && (page > 0) )
          page--;
        else if( (cursorPosition == 1) && (page == 0) ){ 
          cursorPosition++;
          page = maxPages;
        }
        delay(800);
    }
    //Botão DOWN
    else if( downBtn.isPressed() ){
      if(cursorPosition == 1)
        cursorPosition++;
      else if( (cursorPosition == lines) && (page < maxPages) )
        page++;
      else if( (cursorPosition == lines) && (page == maxPages) ){
        page = 0;
        cursorPosition = 1;
      }
      delay(800);
    }
  }//while

  if(output->enable){
    if( (page + cursorPosition - 1) == 0 )
      return CONFIG_OPT;
    else if( (page + cursorPosition - 1) == 1 ){
      output->enable = false;
      return DISABLE_OPT;
    }
    else if( (page + cursorPosition - 1) == 2 )
      return EXIT_MENU;
  }
  else{
    if( (page + cursorPosition - 1) == 0 )
      return ENABLE_OPT;
    else if( (page + cursorPosition - 1) == 1 )
      return EXIT_MENU;
  }

  return EPIC_FAIL;

}//enableOutput()

/******************************************************************************
 * 
 * SET ON/OFF TIME - ajusta o horário para ligar ou desligar uma lâmpada
 *
 *****************************************************************************/
int setOnOffTime(TurnOnOff_t *output){
 bool exitMenu = false;
 int cursorPosition = 1, lastCursorPosition = 0;
 int retValue;
 byte onHour, onMin, onSec;
 byte offHour, offMin, offSec;
 byte lastOnHour = 0, lastOnMin = 0, lastOnSec = 0;
 byte lastOffHour = 0, lastOffMin = 0, lastOffSec = 0;

  printOnOffTimeView(output);
  while( menuBtn.isPressed() );
  
  onHour = output->onHour;
  onMin = output->onMinute;
  onSec = output->onSecond;
  offHour = output->offHour;
  offMin = output->offMinute;
  offSec = output->offSecond;

  while(!exitMenu){

    if( menuBtn.dualFunction() == 1 ){          //Click CURTO (muda de opção no menu)
      if(cursorPosition < 8)
        cursorPosition++;
      else
        cursorPosition = 1;
    }
    
    else if( menuBtn.dualFunction() == -1 ){    //Click LONGO (ENTER)
      if(cursorPosition == 7){                  //Cursor na opção NEXT
        onOffTemp = *output;
        onOffTemp.onHour = onHour;
        onOffTemp.onMinute = onMin;
        onOffTemp.onSecond = onSec;
        onOffTemp.offHour = offHour;
        onOffTemp.offMinute = offMin;
        onOffTemp.offSecond = offSec;
        retValue = NEXT_OPT;
        exitMenu = true;
      }
      else if(cursorPosition == 8){             //Cursor na opção CANCEL
        retValue = CANCEL_OPT;
        exitMenu = true;
      }
    }

    //------------------------------------------------------------------------------
    //Cursor piscando sobre a HORA de LIGAR
    if(cursorPosition == 1){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(13,3);                    //Apaga cursor da opção Cancel
        lcd.print(BLANK_CHAR);
        lcd.setCursor(11,1);                    //Reposiciona cursor sobre as HORAS de LIGAR
        lcd.blink();                            //Cursor piscando
      }
      if(onHour != lastOnHour){                 //Só atualiza LCD se for necessário
        lastOnHour = onHour;
        lcd.setCursor(10,1);
        if(onHour < 10)
          lcd.print("0");
        lcd.print(onHour);
        lcd.setCursor(11,1);
      }      
      if( upBtn.isPressed() ){                  //Incrementa HORAS de LIGAR
          if(onHour < 23)
            onHour++;
          else
            onHour = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa HORAS de LIGAR
          if(onHour > 0)
            onHour--;
          else
            onHour = 23;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre o MINUTO de LIGAR
    else if(cursorPosition == 2){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(14,1);
      }
      if(onMin != lastOnMin){                   //Só atualiza LCD se for necessário
        lastOnMin = onMin;
        lcd.setCursor(12,1);
        printTimeDigits(onMin);
        lcd.setCursor(14,1);
      }
      if( upBtn.isPressed() ){                  //Incrementa MINUTOS de LIGAR
          if(onMin < 59)
            onMin++;
          else
            onMin = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa MINUTOS de LIGAR
          if(onMin > 0)
            onMin--;
          else
            onMin = 59;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre o SEGUNDO de LIGAR
    else if(cursorPosition == 3){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(17,1);
      }
      if(onSec != lastOnSec){                   //Só atualiza LCD se for necessário
        lastOnSec = onSec;
        lcd.setCursor(15,1);
        printTimeDigits(onSec);
        lcd.setCursor(17,1);
      }
      if( upBtn.isPressed() ){                  //Incrementa SEGUNDOS de LIGAR
          if(onSec < 59)
            onSec++;
          else
            onSec = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa SEGUNDOS de LIGAR
          if(onSec > 0)
            onSec--;
          else
            onSec = 59;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre a HORA de DESLIGAR
    else if(cursorPosition == 4){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(11,2);
      }
      if(offHour != lastOffHour){               //Só atualiza LCD se for necessário
        lastOffHour = offHour;
        lcd.setCursor(10,2);
        if(offHour < 10)
          lcd.print("0");
        lcd.print(offHour);
        lcd.setCursor(11,2);
      }
      if( upBtn.isPressed() ){                  //Incrementa HORAS de DESLIGAR
          if(offHour < 23)
            offHour++;
          else
            offHour = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa HORAS de DESLIGAR
          if(offHour > 0)
            offHour--;
          else
            offHour = 23;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre o MINUTO de DESLIGAR
    else if(cursorPosition == 5){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(14,2);
      }
      if(offMin != lastOffMin){                 //Só atualiza LCD se for necessário
        lastOffMin = offMin;
        lcd.setCursor(12,2);
        printTimeDigits(offMin);
        lcd.setCursor(14,2);
      }
      if( upBtn.isPressed() ){                  //Incrementa MINUTOS de DESLIGAR
          if(offMin < 59)
            offMin++;
          else
            offMin = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa MINUTOS de DESLIGAR
          if(offMin > 0)
            offMin--;
          else
            offMin = 59;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor piscando sobre o SEGUNDO de DESLIGAR
    else if(cursorPosition == 6){
      if(cursorPosition != lastCursorPosition){ //Só reposiciona cursor se for necessário
        lastCursorPosition = cursorPosition;
        lcd.setCursor(17,2);
      }
      if(offSec != lastOffSec){                 //Só atualiza LCD se for necessário
        lastOffSec = offSec;
        lcd.setCursor(15,2);
        printTimeDigits(offSec);
        lcd.setCursor(17,2);
      }
      if( upBtn.isPressed() ){                  //Incrementa SEGUNDOS de DESLIGAR
          if(offSec < 59)
            offSec++;
          else
            offSec = 0;
          delay(DELAY_BTN);
      }
      else if( downBtn.isPressed() ){           //Decrementa SEGUNDOS de DESLIGAR
          if(offSec > 0)
            offSec--;
          else
            offSec = 59;
          delay(DELAY_BTN);
      }
    }
    //------------------------------------------------------------------------------
    //Cursor na opção NEXT
    else if(cursorPosition == 7){
      lcd.noBlink();                            //Cursor para de piscar
      lcd.setCursor(0,3);                       //Coloca seletor em NEXT
      lcd.print(SELECTOR);
    }
    //------------------------------------------------------------------------------
    //Cursor na opção CANCEL
    else if(cursorPosition == 8){
      lcd.setCursor(0,3);                       //Apaga seletor que estava na opção Next
      lcd.print(BLANK_CHAR);
      lcd.setCursor(13,3);                      //Coloca seletor em CANCEL
      lcd.print(SELECTOR);
    }
  }//while
  lcd.noBlink();
  return retValue;
}//setOnOffTime()

/******************************************************************************
 * 
 * SET ON/OFF DAYS - ajusta o dia da semana para ligar ou desligar uma lâmpada
 *
 *****************************************************************************/
int setOnOffDays(TurnOnOff_t *output){
 bool exitMenu = false;
 unsigned int cursorPosition = 0, lastCursorPosition = 1;
 int retValue;
 LcdPosition_t *cursorXY = (LcdPosition_t*) malloc( sizeof(LcdPosition_t) * 9 );
 LcdPosition_t *checkXY = (LcdPosition_t*) malloc( sizeof(LcdPosition_t) * 7 );

  printWeekDaysView();
  while( menuBtn.isPressed() );

  //Posições onde o cursor deve ser escrito de acordo com a opção
  cursorXY[0].col = 0;    //Sun
  cursorXY[0].row = 0;
  cursorXY[1].col = 7;    //Mon
  cursorXY[1].row = 0;
  cursorXY[2].col = 14;   //Tuw
  cursorXY[2].row = 0;
  cursorXY[3].col = 0;    //Wed
  cursorXY[3].row = 1;
  cursorXY[4].col = 7;    //Thu
  cursorXY[4].row = 1;
  cursorXY[5].col = 14;   //Fri
  cursorXY[5].row = 1;
  cursorXY[6].col = 7;    //Sat
  cursorXY[6].row = 2;
  cursorXY[7].col = 0;    //Save
  cursorXY[7].row = 3;
  cursorXY[8].col = 13;   //Cancel
  cursorXY[8].row = 3;

  //Posições onde o check deve ser escrito quando selecionar um dia da semana
  checkXY[0].col = 4;     //Sun
  checkXY[0].row = 0;
  checkXY[1].col = 11;    //Mon
  checkXY[1].row = 0;
  checkXY[2].col = 18;    //Tue
  checkXY[2].row = 0;
  checkXY[3].col = 4;     //Wed
  checkXY[3].row = 1;
  checkXY[4].col = 11;    //Thu
  checkXY[4].row = 1;
  checkXY[5].col = 18;    //Fri
  checkXY[5].row = 1;
  checkXY[6].col = 11;    //Sat
  checkXY[6].row = 2;

  for(int i = 0; i < 7; i++){
    if( output->daysOfWeek & (1<<i) ){
      lcd.setCursor(checkXY[i].col, checkXY[i].row);
      lcd.write(byte(1));
    }
  }

  while(!exitMenu){

    //Reposiciona cursor no LCD
    if(cursorPosition != lastCursorPosition){
      lastCursorPosition = cursorPosition;
      if(cursorPosition == 0){
        lcd.setCursor(cursorXY[8].col, cursorXY[8].row);
        lcd.print(BLANK_CHAR);}
      else{
        lcd.setCursor(cursorXY[cursorPosition-1].col, cursorXY[cursorPosition-1].row);
        lcd.print(BLANK_CHAR);}
      
      if(cursorPosition == 8){
        lcd.setCursor(cursorXY[0].col, cursorXY[0].row);
        lcd.print(BLANK_CHAR);}
      else{
        lcd.setCursor(cursorXY[cursorPosition+1].col, cursorXY[cursorPosition+1].row);
        lcd.print(BLANK_CHAR);}
      
      lcd.setCursor(cursorXY[cursorPosition].col, cursorXY[cursorPosition].row);
      lcd.print(SELECTOR);
    }
    //Botão MENU
    if( menuBtn.dualFunction() == 1 ){          //Click NORMAL (faz check em dia da semana)
      if(cursorPosition < 7){
        lcd.setCursor(checkXY[cursorPosition].col, checkXY[cursorPosition].row);
        if( onOffTemp.daysOfWeek & (1 << cursorPosition) ){
          lcd.print(BLANK_CHAR);
          onOffTemp.daysOfWeek &= ~(1 << cursorPosition); 
        }
        else{
          lcd.write(byte(1));
          onOffTemp.daysOfWeek |= 1 << cursorPosition;
        }
      }
    }
    else if( menuBtn.dualFunction() == -1 ){    //click LONGO (enter)
      if(cursorPosition == 7){
        onOffTemp.enable = true;
        retValue = SAVE_OPT;
        exitMenu = true;
      }
      else if(cursorPosition == 8){
        retValue = CANCEL_OPT;
        exitMenu = true;
      }
    }
    //Botão DOWN
    if( downBtn.isPressed() ){
      if(cursorPosition < 8)
        cursorPosition++;
      else
        cursorPosition = 0;
      delay(800);
    }
    //Botão UP
    else if( upBtn.isPressed() ){
      if(cursorPosition > 0)
        cursorPosition--;
      else
        cursorPosition = 8;
      delay(800);
    }
  }//while
  free(cursorXY);
  free(checkXY);
  return retValue;
}//setOnOffDays()

/******************************************************************************
 * 
 * Funções auxiliares para o ajuste de Hora e Data
 * utilizadas pelas funções setOnOffTime() e setOnOffDays()
 *
 *****************************************************************************/
void printOnOffTimeView(TurnOnOff_t *output){
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("SET ");
  lcd.print(output->name);
  lcd.setCursor(0,1);
  lcd.print("Turn On:");
  printTurnOnTime(10,1,output);
  lcd.setCursor(0,2);
  lcd.print("Turn Off:");
  printTurnOffTime(10,2,output);
  printNextCancelOptions();
}
void printWeekDaysView(){
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print(weekDay[0]);
  lcd.setCursor(8,0);
  lcd.print(weekDay[1]);
  lcd.setCursor(15,0);
  lcd.print(weekDay[2]);
  lcd.setCursor(1,1);
  lcd.print(weekDay[3]);
  lcd.setCursor(8,1);
  lcd.print(weekDay[4]);
  lcd.setCursor(15,1);
  lcd.print(weekDay[5]);
  lcd.setCursor(8,2);
  lcd.print(weekDay[6]);
  printSaveCancelOptions();
}
void printTurnOnTime(int col, int row, TurnOnOff_t *output){
  lcd.setCursor(col,row);
  if(output->onHour < 10)
    lcd.print("0");
  lcd.print(output->onHour);
  printTimeDigits( (int) output->onMinute );
  printTimeDigits( (int) output->onSecond );
}
void printTurnOffTime(int col, int row, TurnOnOff_t *output){
  lcd.setCursor(col,row);
  if(output->offHour < 10)
    lcd.print("0");
  lcd.print(output->offHour);
  printTimeDigits( (int) output->offMinute );
  printTimeDigits( (int) output->offSecond );
}
void printNextCancelOptions(){
  lcd.setCursor(1,3);
  lcd.print(NEXT_STR);
  lcd.setCursor(14,3);
  lcd.print(CANCEL_STR);
}

/******************************************************************************
 * 
 * Função responsável por escrever no LCD uma mensagem de erro
 * com seu respectivo código.
 *
 *****************************************************************************/
void printErrorMsg(int num, String msg){
  lcd.clear();
  lcd.print(num);
  lcd.setCursor(0,1);
  lcd.print(msg);
}
















/******************************************************************************
 * 
 * Funções auxiliares para debug que imprimem relógio e calendário
 * via porta Serial.
 *
 *****************************************************************************/
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
void serialOutputOnOff(TurnOnOff_t *output){
  Serial.print("Status: ");
  if(output->enable)
    Serial.println(ENABLED_STR);
  else
    Serial.println(DISABLED_STR);
  Serial.print("On: ");
  Serial.print(output->onHour);
  printDigits(output->onMinute);
  printDigits(output->onSecond);
  Serial.println();

  Serial.print("Off: ");
  Serial.print(output->offHour);
  printDigits(output->offMinute);
  printDigits(output->offSecond);
  Serial.println();
  
  Serial.print("Days: ");
  for(int i = 0; i < 7; i++){
    if(output->daysOfWeek & (1 << i) ){
      Serial.print(weekDay[i]);
      Serial.print(" ");
    }
  }
  Serial.println();
}
void printDigits(int digits){
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
#endif
