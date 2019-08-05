# JourneyPilot-Scale
This project aimed to test the validity of integrating a digital scale and NFC card reader into the JourneyPilot project developed by Jembi Health Systems. 

The Journey Pilot is mobile based application that will allow Health Care Workers (HCWs) to eﬀectively manage events related to child vaccinations by streamlining the various activities related to immunization tracking.

This digital scale will weigh the child and write the mass to the child’s card allowing Health Care Workers to record and graph the child’s growth progress. This will ultimately monitor the health of a chil to ensure a healthy lifestyle in the future.

The following components were used to build the electronic scale:
Arduino Uno - to supply power and control hardware.
HX711 Load Cell Sensor - a high precision strain gauge used to measure mass.
RFID-RC522 - a two-way radio transmitter-receiver used to read and write to NFC card.

The Main.ino program contains the final code for the digital scale. However, to acheive this, a few calibration programs were adapted first. First, I determined the type of NFC cards that were already implemented in the project. Then the load sensor was calibrated with a known weight. The remaining programs were used to calibrate the sensors, identify the PICC type of the NFC card and erase the data on the card.
