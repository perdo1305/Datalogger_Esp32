/*
Datalogger 2023
Pedro Ferreira
Eletronics Department
*/

#include <Arduino.h>
//#include <FS.h>
//#include <SD.h>
#include "FS.h"
#include "SD_MMC.h"
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
//#include <ESP32_CAN.h>
#include "C:\Users\pedro\OneDrive - IPLeiria\Documents\PlatformIO\Projects\TESTE\lib\ESP32universal_CAN-master\ESP32universal_CAN-master\src\ESP32_CAN.h"

#define MAX_SIZE 40     // Maximum size of the matrix
#define SCK 14          //Custom spi pins
#define MISO 2          //Custom spi pins
#define MOSI 15         //Custom spi pins
#define CS 13           //Custom spi pins
#define SDA 33          //i2c rtc pins
#define SCL 32          //i2c rtc pins
#define EEPROM_SIZE 1   //in bytes
#define ON_BOARD_LED 25 //led pcb datalogger
#define BUTTON_PIN 35   //pcb button
#define RXD1 26         //Serie para a telemetria
#define TXD1 27         //Serie para a telemetria

int lastSecond = -1;          //RTC
unsigned long lastMillis = 0; //RTC    

boolean Button_State = 0;
struct CarData_MAIN { //32 Bytes
  uint16_t  RPM       =  0; //0-65536
  uint8_t   VSPD      =  0; //0-160
  uint8_t   APPS1     =  0; //0-100
  uint8_t   BRAKE     =  0; //0-1
  uint8_t   DBWPOS    =  0; //0-100
  uint8_t   LAMBDA    =  0; //0-2
  uint8_t   OILT      =  0; //0-160
  uint8_t   OILP      =  0; //0-12
  uint16_t  ENGT1     =  0; //0-1100
  uint16_t  ENGT2     =  0; //0-1100
  uint8_t   BATV      =  0; //0-20
  uint8_t   IAT       =  0; //0-167 subtrair 40 no labView
  uint8_t   MAP       =  0; //0-4
  uint16_t  CLT       =  0; //0-290 subtrair 40 no labView
  uint8_t   FUELP     =  0; //0-1
  uint8_t   IGNANG    =  0; //0-20
  uint8_t   CBUSLD    =  0; //0-100
  uint8_t   LAMCORR   =  0; //75-125
  uint8_t   ECUT      =  0; //0-4
  uint8_t   DBWTRGT   =  0; //0-100
  uint8_t   ACCX      =  0; //0-20
  uint8_t   DataLoggerSTAT      =  0; //0-100
  uint8_t   GearValue =  0; //0-1
  uint8_t   ROLL      =  0; //75-125
  uint8_t   PITCH     =  0; //0-4
  uint8_t   YAW       =  0; //0-100
  uint8_t   LOGSTAT   =  0; //0-20

  char      inicio    = 10; //END
};
struct CarData_MAIN carDataMain;


RTC_DS3231 rtc;
char rtc_time[32];

uint8_t flag1=0,flag2=0,flag3=0,flag4=0; //serial menu

uint8_t Current_Max_Row=0; 

uint32_t millis_count=0;
uint32_t previousMillis=0;

uint8_t can_vector[MAX_SIZE][9];

char NUM_file=0;
uint8_t eeprom_count,eeprom_print;

char file_name[20];

char buffer[20]={0};//rtc update function

boolean LED_25=0;
boolean LED_5=0;

//const String Header = "@@@@@@@@@@@@@@ DATALOGGER @@@@@@@@@@@@@@\n"; //1st thing to write in the SD
//SPIClass spi = SPIClass(VSPI);      //Custom spi pins shit

TWAI_Interface CAN1(1000, 21, 22);  // argument 1 - BaudRate,  argument 2 - CAN_TX PIN,  argument 3 - CAN_RX PIN

int row60,row61;


//##### Define functions #####
void CHECK_Serial(void);    //Reads keyboard keys
void Read_Can(void);        //Catch the id and create new line on the matrix
void Init_Sd_Card(void);    //Setup SDcard
void DATA_String(void);     //create a msg to send to the sd card
void CLEAN_Matrix(void);    //put 0's on the matrix      
void Update_RTC(void);     
void Telemetria_Send(void);

//####################### SD CARD FUNCTIONS NO MEXER #######################
//##########################################################################
void writeFile(fs::FS &fs, const char *path, const char *message) {
  
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}
void appendFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    //Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    LED_5 = !LED_5;
    digitalWrite(5,LED_5);
    //Serial.println("Message appended");
  } else {
    digitalWrite(5, LOW);
    //Serial.println("Append failed");
  }
  file.close();
}
void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");

    char str[10]={0};
    int i=0;
    while(file.available()){
        //Serial.write(file.read());
        //NUM_file=file.read();
        str[i] = file.read();
        i++;
    }
    NUM_file= atoi(str);
}
//##########################################################################
//##########################################################################

void TASK1_PRINT(void* arg){
  for(;;){
    if(flag1==1){
      printf("task1\r\n");
    }
    if(flag2==1){
      for (int i = 0; i < MAX_SIZE; i++) {
        printf("| ");
        for (int j = 0; j < 9; j++) {
          printf("%d ", can_vector[i][j]);
        }
        printf("|\n");
      }
        printf("__________________________");
        printf("\n");
    }
    if(flag4==1){
      flag4 = 0;
      printf("MATRIX CLEANED\r\n");
      CLEAN_Matrix();
    }
    vTaskDelay(300/portTICK_PERIOD_MS);
  }
}
void TASK2_READ_CAN(void* arg){
  for(;;){
    if(flag1==1){
      printf("task2\r\n");
    }
    Read_Can();
    
    //vTaskDelay(5/portTICK_PERIOD_MS);
  }
}
void TASK3_WRITE_SD(void* arg){
  for(;;){
    if(flag3==1){
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 200) {
        previousMillis = currentMillis;
        printf("WRITING TO SD CARD! EEPROM:%d\r\n",eeprom_print);
      }
    }
  //long int tempo1= millis();
  DATA_String();
  //long int tempo2= millis();

  //long int tempo_final=tempo1-tempo2;
  //printf("%d\n",tempo_final);

    vTaskDelay(5/portTICK_PERIOD_MS);
  }
}

void TASK4_Telemetria(void* arg){
  for(;;){

    //Telemetria_Send();
    carDataMain.RPM = (can_vector[row60][1]<<8) | can_vector[row60][0];
    carDataMain.OILP = (can_vector[row60][3]<<8) | can_vector[row60][2];
    carDataMain.CLT = (can_vector[row60][5]<<8) | can_vector[row60][4];
    carDataMain.BATV = (can_vector[row60][7]<<8) | can_vector[row60][6];

    carDataMain.VSPD = (can_vector[row61][1]<<8) | can_vector[row61][0];
    carDataMain.GearValue = (can_vector[row61][3]<<8) | can_vector[row61][2];

    Serial1.println("RPM"+String(carDataMain.RPM)+"OP"+String(carDataMain.OILP/10)+"ET"+String(carDataMain.CLT/10)+"BV"+String(carDataMain.BATV/10)+"GEAR"+String(carDataMain.GearValue)+"VS"+String(carDataMain.VSPD));
   

    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

void setup() {

  Serial.begin(115200);//Starts serial monitor
  Serial1.begin(4800, SERIAL_8N1, RXD1, TXD1);

  EEPROM.begin(EEPROM_SIZE);
  //EEPROM.write(0,0);
  //EEPROM.commit();

  Wire.setPins (SDA,SCL);
  Wire.begin();
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  //rtc.adjust(DateTime(2023, 1, 21, 5, 0, 0));
 
  pinMode(ON_BOARD_LED,OUTPUT);
  pinMode(5,OUTPUT);

  Init_Sd_Card();//initializes the SDcard
  delayMicroseconds(50);
  CLEAN_Matrix();
  delay(100);//just a litle break before it starts to read

  
  xTaskCreate(TASK1_PRINT,    "task 1",4096,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(TASK2_READ_CAN, "task 2",4096,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(TASK3_WRITE_SD, "task 3",8192,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(TASK4_Telemetria,"task 4",4096,NULL,tskIDLE_PRIORITY,NULL);


  delay(10);//just a litle break before it starts to read
  
  printf("\n\n########################### DATALOGGER ###########################\r\n\n");
  printf("1 - Show Tasks\r\n"); 
  printf("2 - Show Matrix\r\n");
  printf("3 - Write to SD Card\r\n");
  printf("4 - Clean Matrix\r\n");
}

void loop() {
  
  CHECK_Serial();

}

void CHECK_Serial(){
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
    case '1':
        flag1 = !flag1;
        break;
    case '2':
        flag2 = !flag2;
        break;
    case '3':
        flag3 = !flag3;
        break;
    case '4':
        flag4 = !flag4;
        break;
    }
  }
}

void Init_Sd_Card(){ //Setup SDcard
  pinMode(2,INPUT_PULLUP);

  if(!SD_MMC.begin()){
        Serial.println("Card Mount Failed");
        return;
    }

  File file = SD_MMC.open("/C");

  readFile(SD_MMC, "/config.txt");
  int y = NUM_file;
  y++;
  char buffer_y[5];
  sprintf(buffer_y,"%d",y); 
  writeFile(SD_MMC, "/config.txt", buffer_y);

  if (!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    /*
    eeprom_count = EEPROM.read(0);
    eeprom_print = eeprom_count;
    */

    //sprintf(file_name,"/DATA_%d.csv",eeprom_count);
    sprintf(file_name,"/DATA_%d.csv",NUM_file);

    DateTime now = rtc.now();
    sprintf(rtc_time, "%02d:%02d:%02d %02d/%02d/%02d\n", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  
    //writeFile(SD, file_name, Header.c_str());
    writeFile(SD_MMC, file_name, rtc_time);

    /*
    eeprom_count++;
    EEPROM.write(0, eeprom_count);
    EEPROM.commit();
    */
  } else {
    Serial.println("File already exists");
  }
  file.close();
}

void Read_Can(){ // catch the id and create new line on the matrix
  int ID;
  ID = CAN1.RXpacketBegin(); // Get Frame ID //Make Sure this ir running in the loop 
  if (ID != 0){
    LED_25 = !LED_25;
    digitalWrite(ON_BOARD_LED,LED_25);
    boolean found;
    do{ // check if ID already exists in the vector
        
        if (ID < 0x60 || ID > 0x72){ // restringir ids recebidos
        return;
        }
        
        found = 0;
  
        for (int j = 0; j < Current_Max_Row; j++)
        {

          if (can_vector[j][0] == ID){
            // printf("ID already exists in the matrix: %d,%d,%d\n",i,j,found);
            uint8_t row = j;
            for (int k = 0; k < 8; k++){           // LER BYTES!
              can_vector[row][k + 1] = CAN1.RXpacketRead(k); //[x,b0,b1,b2,b3,b4,b5,b6,b7]

            }
            if(ID==96){
              row60=row;
            }
            if (ID==97){
              row61=row;
            }

            //memcpy(can_vector,telemetira_vector,sizeof( can_vector));

            found = 1;
            break;
          }
        }
        if (!found){
          can_vector[Current_Max_Row][0] = ID;
          Current_Max_Row++;
          // printf("ID added to the matrix.\n");
        }
    }while (Current_Max_Row < MAX_SIZE && found == 0);
  }
}

void DATA_String(){ //Take the matrix and tranforms it to a string to write in SDcard

  char dataMessage[1000];
  dataMessage[0] = '\0';
    //Current_Max_Row
  for(int i = 0; i<1 ;i++){
    
    if(i==0){
      Update_RTC();
      strcpy(dataMessage,buffer);
      sprintf( dataMessage + strlen(dataMessage),"\n");
    }
    
    for (int j =0; j<9;j++){
      sprintf(dataMessage + strlen(dataMessage),"%d",can_vector[i][j]);//pesquisar unsigned int
      if (j<8){
        sprintf(dataMessage + strlen(dataMessage), ";");
      }
    } 
    sprintf(dataMessage + strlen(dataMessage),"\n");
  }

  /*
  char *ptr = dataMessage; // pointer to the start of dataMessage

  for (int i = 0; i < Current_Max_Row; i++){
    for (int j = 0; j < 9; j++){
      *ptr++ = can_vector[i][j] + '0'; // convert int to char and store it
      if (j < 8){
        *ptr++ = ';';
      }
    }
    *ptr++ = '\n';
  }
  *ptr = '\0'; // null terminate the string

  */
 
  //if(y==1){
  //  sprintf(dataMessage + strlen(dataMessage),"\n");
  //}
  //y=0;
  appendFile(SD_MMC, file_name , dataMessage);
}

void CLEAN_Matrix(){
  for (int k = 0; k < MAX_SIZE; k++) {
    for (int j = 0; j < 9; j++) {
      can_vector[k][j] = 0;
    }
  }
  Current_Max_Row=0;
}

void Update_RTC(){ //falta colocar na string do cartao
  DateTime now = rtc.now();
  
  // Check if the seconds have updated
  if (now.second() != lastSecond) {
    // Store the current value of millis()
    lastMillis = millis();
    lastSecond = now.second();
  }
  
  // Calculate the number of milliseconds since the last second update
  unsigned long currentTime = millis();
  int milliseconds = currentTime - lastMillis;
  
  // Print the current time including milliseconds
  //char buffer[20];
  sprintf(buffer, "%02d:%02d:%02d.%03d", now.hour(), now.minute(), now.second(), milliseconds);
  //Serial.println(buffer);

}

void Telemetria_Send(){
  int a=(can_vector[row60][0]<<8) | can_vector[row60][1];
  int b=(can_vector[row60][2]<<8) | can_vector[row60][3];
  int c=(can_vector[row60][4]<<8) | can_vector[row60][5];
  int d=(can_vector[row60][6]<<8) | can_vector[row60][7];
  int e=(can_vector[row61][0]<<8) | can_vector[row61][1];
  int f=(can_vector[row61][2]<<8) | can_vector[row61][3];

  carDataMain.RPM = a;
  carDataMain.OILP =b;
  carDataMain.CLT = c;
  carDataMain.BATV = d;
  carDataMain.VSPD = e;
  carDataMain.GearValue = f;

  /*
  carDataMain.RPM = CAN_Bus_Data[1];
  carDataMain.APPS1 = CAN_Bus_Data[2];
  carDataMain.IAT = CAN_Bus_Data[3];
  carDataMain.MAP = CAN_Bus_Data[4];
  carDataMain.CLT = CAN_Bus_Data[5];
  carDataMain.VSPD = CAN_Bus_Data[6];
  carDataMain.OILT = CAN_Bus_Data[7];
  carDataMain.OILP = CAN_Bus_Data[8] * 10;
  carDataMain.FUELP = CAN_Bus_Data[9];
  carDataMain.BATV = CAN_Bus_Data[10]*10;
  carDataMain.IGNANG = CAN_Bus_Data[11];
  carDataMain.LAMBDA = CAN_Bus_Data[13]*10;
  carDataMain.ENGT1 = CAN_Bus_Data[15];
  carDataMain.ENGT2 = CAN_Bus_Data[16];
  carDataMain.CBUSLD = CAN_Bus_Data[17];
  carDataMain.ECUT = CAN_Bus_Data[18];
  carDataMain.DBWPOS = CAN_Bus_Data[19];
  carDataMain.DBWTRGT = CAN_Bus_Data[20];


  carDataMain.DataLoggerSTAT = Logger_Status;
  carDataMain.GearValue = CAN_Bus_Data[44];

  carDataMain.PITCH = CAN_Bus_Data[40];
  carDataMain.ROLL = CAN_Bus_Data[41];
  carDataMain.YAW = CAN_Bus_Data[42];
    if(brakeSensor<186){
    carDataMain.BRAKE = 1;
  }else{
    carDataMain.BRAKE = 0;
  }  
  carDataMain.LOGSTAT = Logger_Status;
  if(Serial1.available()>0){

if(Serial1.readString().toInt()==30){
       if(D_Logger==0){
      D_Logger = 255;
    }else{
      D_Logger = 0;
    }
    }
  
   
  }
  */
  //Serial1.println("ola");
  Serial1.write((uint8_t *) &carDataMain, sizeof(carDataMain));
}