#include <PushButton.h>       //Debounce dos botões
#include <LiquidCrystal.h>    //Display LCD alfanumérico
#include <Wire.h>             //Interface I2C
#include <TimeLib.h>          //Tempo
#include <DS1307RTC.h>        //RTC DS1307


// Flag para habilitar depuração via Monitor Serial da Arduino IDE
#define DEBUG false


// Configurações para printar hora e data no LCD
#define PRINT_SECONDS         B00000001
#define PRINT_TEXT_DATE       B00000010
#define PRINT_TEXT_TIME       B00000100
#define PRINT_WEEK_DAY        B00001000
#define DOT_SEPARATOR         B00010000
#define DASH_SEPARATOR        B00100000


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
void printSetTimeView();
void printSetDateView();
void printSaveCancelOptions();
void OutputsMenu();
void printErrorMsg();
#if DEBUG
  void serialClockDisplay();
  void printDigits(int digits);
#endif


// Strings contendo nome e versão do projeto
const String projectName = "GrowSystem";
const String projectVersion = "v1.0";


// Configurações para LCD
// Número de linhas e de colunas, Pinos a serem usados
// Instância do objeto LiquidCrystal, Caractere especial
uint8_t lcdNumCol = 20, lcdNumRow = 4;
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte block[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };


// Configurações dos botões.
// Pinos dos botões e instâncias dos objetos da classe PushButton
const int menuBtnPin = 8;
const int upBtnPin = 9;
const int downBtnPin = 10;
PushButton menuBtn(menuBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton upBtn(upBtnPin, 50, DEFAULT_STATE_HIGH);
PushButton downBtn(downBtnPin, 50, DEFAULT_STATE_HIGH);


//VARIÁVEIS GLOBAIS DE USO GERAL DO PROGRAMA
unsigned long lastUpdate = 0;                         //controla a atualização do LCD a cada 1s
tmElements_t tm;                                      //Armazena tempo do sistema
int tCol = 0, tRow = 2, dCol = 0, dRow = 3;           //Onde serão impressos data e hora no LCD
const String weekDay[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
byte defaultPrintConfigs =  PRINT_SECONDS | PRINT_TEXT_DATE | PRINT_TEXT_TIME | DOT_SEPARATOR | PRINT_WEEK_DAY;
int errorNum;
String errorMsg;

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

  #if DEBUG
    Serial.begin(9600);
    Serial.println("Initializing system...");
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  lcd.createChar(0, block);
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
      Serial.println("Unable to sync with the RTC");
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
      Serial.println("RTC has set the system time");
    #endif
  }
  breakTime(now(), tm);
  printMainView();
}



/********************************************************************************
 * 
 * Função LOOP - É repetida continuamente pelo firmware.
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

    if(menuOption == "Exit"){
      printMainView();
      while( menuBtn.isPressed() );
    }
    else if(menuOption == "Set Time"){
      setDateTimeMenu(menuOption);
      printMainView();
      while( menuBtn.isPressed() );
    }
    else if(menuOption == "Set Date"){
      
    }
    else if(menuOption == "Outputs"){
      OutputsMenu();
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

}

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
 * PRINT TIME - Imprime a hora no LCD na posição indicada popr col e row
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
 * 
 * Função auxiliar para imprimir os dígitos da data com o separador
 *
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
 * 
 * Função auxiliar para imprimir os dígitos da hora com o separador :
 *
 *****************************************************************************/
void printTimeDigits(int digits){
  lcd.print(":");
  if(digits < 10)
    lcd.print("0");
  lcd.print(digits);
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
void setDateTimeMenu(String option){
 int countBtnClicks = 0;
 bool exitMenu = false;
 int myHour = hour();
 int myMinute = minute();
 int mySecond = second();
 int lastHour = myHour;
 int lastMin = myMinute;
 int lastSec = mySecond;
 unsigned int delayBtn = 400;

  if(option == "Set Time")
    printSetTimeView();
  else if(option == "Set Date")
    printSetDateView();
  else{
    printErrorMsg();
  }
  while( menuBtn.isPressed() );


  //Laço que fica testanto botões e realizando as respectivas operações
  //Vai sair do laço quando a opção Save ou Cancel forem selecionadas
  while( !exitMenu ){

    //Se apertar o botão Menu
    //São 4 posições possíveis para o cursor: hora, min, seg, Save, Cancel
    //dualFunction retorna 1 se o click for simples ou -1 se o click for longo (enter)
    if( menuBtn.dualFunction() == 1 ){
      if(countBtnClicks < 4)
        countBtnClicks++;
      else{
        countBtnClicks = 0;
        lcd.setCursor(13,3);                //Apaga seletor que estava na opção Cancel
        lcd.print(" ");
        lcd.blink();
      }
    }
    else if( menuBtn.dualFunction() == -1 ){
      if(countBtnClicks == 3){  //Opção Save
        setTime(myHour, myMinute, mySecond, day(), month(), year());

        #if DEBUG
          Serial.print("Time setted by user: ");
          serialClockDisplay();
        #endif
        
        tmElements_t myTime;
        breakTime(now(), myTime);
        
        if( RTC.write(myTime) ){
          #if DEBUG
            Serial.println("OK! Time successfully adjusted in RTC.");
          #endif
          lcd.clear();
          lcd.setCursor(8,0);
          lcd.print("OK");
          lcd.setCursor(1,2);
          lcd.print("Time successfully");
          lcd.setCursor(6,3);
          lcd.print("adjusted");
          delay(1500);
        }
        else {
          #if DEBUG
            Serial.println("ERROR! Cannot update time in RTC.");
          #endif
          lcd.clear();
          lcd.setCursor(7,1);
          lcd.print("ERROR!");
          lcd.setCursor(1,2);
          lcd.print("Cannot adjust time");
          delay(1500);
        }
        exitMenu = true;
      }
      else if(countBtnClicks == 4){ //Opção Cancel: apenas sai do loop.
        exitMenu = true;
      }
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
        delay(delayBtn);
      }
      else if( downBtn.isPressed() ){
        if(myHour > 0)
          myHour--;
        else
          myHour = 23;
        delay(delayBtn);
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
        delay(delayBtn);
      }
      else if( downBtn.isPressed() ){
        if(myMinute > 0)
          myMinute--;
        else
          myMinute = 59;
        delay(delayBtn);
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
        delay(delayBtn);
      }
      else if( downBtn.isPressed() ){
        if(mySecond > 0)
          mySecond--;
        else
          mySecond = 59;
        delay(delayBtn);
      }
    }
    
    //Cursor na opção Save
    else if(countBtnClicks == 3){
      lcd.noBlink();
      lcd.setCursor(0,3);
      lcd.print('>');
    }
    
    //Cursor na opção Cancel
    else if(countBtnClicks == 4){
      lcd.setCursor(0,3);           //Apaga o seletor que estava na opção Save
      lcd.print(" ");
      lcd.setCursor(13,3);          //Escreve novo seletor
      lcd.print('>');
    }
    
  }//while
  lcd.noBlink();
  lcd.clear();
}//function



void printSetTimeView(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SET TIME");
  printTime(6,2,PRINT_SECONDS);
  printSaveCancelOptions();
  lcd.setCursor(7,0);
  lcd.blink();
}
void printSetDateView(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SET DATE");
  printDate(6,2,DOT_SEPARATOR);
  printSaveCancelOptions();
  lcd.setCursor(7,0);
  lcd.blink();
}
void printSaveCancelOptions(){
  lcd.setCursor(1,3);
  lcd.print("Save");
  lcd.setCursor(14,3);
  lcd.print("Cancel");
}

/******************************************************************************
 * 
 * Função OUTPUTS MENU - Cria menu para configurar saídas do sistema
 *
 *****************************************************************************/
void OutputsMenu(){

}




void printErrorMsg(){
  
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
