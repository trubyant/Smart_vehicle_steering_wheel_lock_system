#define RXD2 1
#define TXD2 2

void setup()
{
   // Open serial communications
   Serial.begin(115200);
   Serial.println("Neo6M GPS module test code");
   // set the data rate for the SoftwareSerial port
   Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop()
{
   if (Serial2.available())
   {
      Serial.print(char(Serial2.read()));
   }
}


