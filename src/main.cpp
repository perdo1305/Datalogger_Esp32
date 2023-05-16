#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM.h>
#include <CAN.h>    //https://github.com/sandeepmistry/arduino-CAN/tree/master/src

#define MAX_SIZE 20 // Maximum size of the matrix
#define SCK 14      //Custom spi pins
#define MISO 2      //Custom spi pins
#define MOSI 15     //Custom spi pins
#define CS 13       //Custom spi pins
#define EEPROM_SIZE 1
#define ON_BOARD_LED 25 //led pcb datalogger


int flag1=0,flag2=0,flag3=0,flag4=0,i=0; 
long int previousMillis=0;
unsigned int can_vector[MAX_SIZE][9];
unsigned int eeprom_count,eeprom_print;
char file_name[20];

const String Header = "@@@@@@@@@@@@@@ DATALOGGER @@@@@@@@@@@@@@\n"; //1st thing to write in the SD

SPIClass spi = SPIClass(VSPI); //Custom spi pins shit

//##### Define functions #####
void CHECK_Serial(void);    //Reads keyboard keys
void Read_Can(void);        //Catch the id and create new line on the matrix
void Init_Sd_Card(void);    //Setup SDcard
void DATA_String(void);     //create a msg to send to the sd card
void CLEAN_Matrix(void);    //put 0's on the matrix

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
    //Serial.println("Message appended");
  } else {
    //Serial.println("Append failed");
  }
  file.close();
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
    digitalWrite(ON_BOARD_LED,LOW);
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
    DATA_String();
    vTaskDelay(5/portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);//Starts serial monitor
  EEPROM.begin(EEPROM_SIZE);
  CAN.setPins(22, 21); //(rx,tx)

  //EEPROM.write(0, 0);
  //EEPROM.commit();

  /*--------------initializes Can--------------*/
  if (!CAN.begin(1000E3)){ //1 Mbit/second
    printf("Starting CAN failed!\r\n");
    while (1);
  }
 
  Init_Sd_Card();//initializes the SDcard

  pinMode(ON_BOARD_LED,OUTPUT);
  
  xTaskCreate(TASK1_PRINT,"task 1",10000,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(TASK2_READ_CAN,"task 2",10000,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(TASK3_WRITE_SD,"task 3",10000,NULL,tskIDLE_PRIORITY,NULL);

  delay(500);//just a litle break before it starts to read
  CLEAN_Matrix();
  
  printf("\n########################### DATALOGGER ###########################\r\n\n");
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
  spi.begin(SCK, MISO, MOSI, CS);

  if (!SD.begin(CS, spi, 80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }

  File file = SD.open("/C");

  if (!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");

    eeprom_count = EEPROM.read(0);
    eeprom_print = eeprom_count;

    sprintf(file_name,"/DATA_%d.csv",eeprom_count);
    writeFile(SD, file_name, Header.c_str());

    eeprom_count++;
    EEPROM.write(0, eeprom_count);
    EEPROM.commit();
  } else {
    Serial.println("File already exists");
  }
  file.close();
}

void Read_Can(){//catch the id and create new line on the matrix
  int ID;
  int packetSize = CAN.parsePacket();
  if (packetSize != 0){
    digitalWrite(ON_BOARD_LED,HIGH);
    ID = CAN.packetId();
    int found;
    do {// check if ID already exists in the vector
      found = 0;
      for (int j = 0; j < i; j++) {
        if (can_vector[j][0] == ID) {
          //printf("ID already exists in the matrix: %d,%d,%d\n",i,j,found);
          int row=j;
          for(int k = 0; k < 8;k ++){//LER BYTES!
            can_vector[row][k+1]=CAN.read();//[x,b0,b1,b2,b3,b4,b5,b6,b7]
          }
          found = 1;
          break;
        }
      }
      if (!found) {
        can_vector[i][0] = ID;
        i++;
        //printf("ID added to the matrix.\n");
      } 
    } while (i < MAX_SIZE && found==0);
  }
}

void DATA_String(){ //Take the matrix and tranforms it to a string to write in SDcard
  char dataMessage[1000];
  dataMessage[0] = '\0';
  for(int i =0; i< MAX_SIZE;i++){//escrever so ate row?
    for (int j =0; j<9;j++){
      sprintf(dataMessage + strlen(dataMessage),"%d",can_vector[i][j]);//pesqeuisar unigned int
      sprintf(dataMessage + strlen(dataMessage), ";");
    } 
    sprintf(dataMessage + strlen(dataMessage),"\n");
  }
    appendFile(SD, file_name , dataMessage);
}

void CLEAN_Matrix(){
  for (int k = 0; k < MAX_SIZE; k++) {
    for (int j = 0; j < 9; j++) {
      can_vector[k][j] = 0;
    }
  }
  i=0;
}
