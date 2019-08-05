/*
   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>

#define RST_PIN  9          // Configurable, see typical pin layout above
#define SS_PIN  10         // Configurable, see typical pin layout above
#define DOUT  3
#define CLK  2

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;        // Card status
HX711 scale;

byte Card[16][4];
byte buffer[18]; // 16 (data) + 2 (CRC)
byte byteCount = sizeof(buffer);
byte result[48];
byte resultCount = sizeof(result);

float calibration_factor = -7050;
double avg;

int positionPage;
int positionIndex;

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println("--------------------------");
  Serial.println(F("Scan card to display UID, SAK, type, and data blocks."));

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare(); //Reset the scale to 0
  
}

void loop() {
  //if there is no new card - keep searching
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  else {
    
    ReadInfo();
    ShowInfo(); 

    Serial.println(F("Place child on the scale (5 seconds)."));
    Serial.println("--------------------------");

    delay(5000);

    //scale data
    avg = scale.get_units(20)*0.453592;
    if(avg<0)
      avg=avg*-1;
    avg = avg*10;
    avg = (int)avg;
    avg = avg/10;
    Serial.print("Average Weight: ");
    Serial.print(avg);
    Serial.println(" kgs");

    //get the date


    //find position to write data to
    //letting 'delimeter byte' be 0xC5
    Populate1D();
    FindLastDelimeter();

    Serial.println();
    Serial.print("Page: ");
    Serial.println(positionPage);
    Serial.print("Index: ");
    Serial.println(positionIndex);
    Serial.println(result[4*(positionPage-4)+(positionIndex)],HEX);

    UpdateArray();

    //write data
    Serial.println("--------------------------");
    Serial.println(F("Scan card to write data."));
    Serial.println("--------------------------");
    
    WriteData();

    Serial.println("--------------------------");
    Serial.println(F("Scan card again to see that the data has been recorded."));
    ReadInfo();
    ShowInfo(); 

    mfrc522.PICC_HaltA();
  }

}

//Methods

//write data to NFC card
void WriteData(){
  for (int i = 0; i < 12; i++) {
     //data is writen in blocks of 4 bytes (4 bytes per page)
     status = (MFRC522::StatusCode) mfrc522.MIFARE_Ultralight_Write(4+i, &result[i*4], 4);
     if (status != MFRC522::STATUS_OK) {
       Serial.print(F("MIFARE_Read() failed: "));
       Serial.println(mfrc522.GetStatusCodeName(status));
     }
   }
   if (status == MFRC522::STATUS_OK)
    Serial.println("Data Successfully Written to Card"); 
}

//update contents of the Card[][] array
void UpdateArray(){
   result[4*(positionPage-4)+(positionIndex)+1] = (avg, HEX);
   result[4*(positionPage-4)+(positionIndex)+2] = 0xC5;            //same delimeter

   Serial.print("Updated 1D:\t");
   for(int i=0;i<48;i++){
    Serial.print(result[i],HEX);
    Serial.print(" "); 
   }
   Serial.println(); 
  
}

//converting Card[][] to 1D array --> dont have to worry about grid anymore + get rid of non-writable blocks 0-3
void Populate1D(){
  int k = 0;
  for (int i=4; i<16; i++){
    for (int j=0; j<4; j++){
      result[k]=Card[i][j];
      k++;
    }
  }

  Serial.print("Convert to 1D:\t");
  for(int i=0;i<48;i++){
    Serial.print(result[i],HEX);
    Serial.print(" "); 
  }
}

//finds the position of the last delimeter 
void FindLastDelimeter(){
  for (int i=0; i<16; i++){
    for (int j=0; j<4; j++){
      if(Card[i][j] == 0xC5){
        positionPage = i;
        positionIndex = j;
      }
    }
  }
}

// fills the "Card[16][4]" with 0s
void ResetInfo(){
  for (int i=0; i<=15; i++){
    for (int j=0; j<=4; j++){
      Card[i][j]=0;
    }
  }
}

// fills the "Card[16][4]" with data
void ReadInfo() {
  ResetInfo();
  for (byte page=0; page<=15; page+=4){
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(page, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    //memcpy(buffer, result, byteCount);

    //      [page][index]
    //       0-15   0-3
    //  Card [16]   [4]
    //
    //      0-3   page 0
    //      4-7   page 1
    //      8-11  page 2
    //     12-15  page 3
    //  buffer[16+2]

    int i_=0;
    for (int i=page; i<=page+3; i++){
      for (int j=0; j<=3; j++){
        Card[i][j]=buffer[4*i_ + j];
        //Serial.print(buffer[4*i_ + j],HEX);
        //Serial.print("");
      }
      i_++;
    }
  }
  
  //  This is to stop the card from sending the info over and over again
  //mfrc522.PICC_HaltA();
}

// shows the "Card[16][4]" after filling it
void ShowInfo(){

  Serial.println("--------------------------");
  for (int i=0; i<16; i++){
    for (int j=0; j<4; j++){
      Serial.print(Card[i][j],HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println("--------------------------");
}
