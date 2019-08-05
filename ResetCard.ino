#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN  9          // Configurable, see typical pin layout above
#define SS_PIN  10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status;        // Card status

byte Card[16][4];
byte buffer[18]; // 16 (data) + 2 (CRC)
byte byteCount = sizeof(buffer);
byte result[48];
byte resultCount = sizeof(result);

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println("--------------------------");
  Serial.println(F("Scan card to reset all memory blocks."));

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

    SetZeros();

    //option to add a single byte as 'delimiter' byte
    //result[0]=0xC5;

    WriteData();

    Serial.println("--------------------------");
    Serial.println(F("Data Currently on Card"));
    ReadInfo();
    ShowInfo(); 

    mfrc522.PICC_HaltA();

  }
} 

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
    Serial.println("Card Reset Successful"); 
}

//set the result array values to 0 to be written to the card
void SetZeros(){
  for(int i=0; i<48; i++){
    result[i]=0;
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
