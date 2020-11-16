// Demo: NMEA2000 library. 
// This demo reads messages from NMEA 2000 bus and
// sends them translated to clear text to Serial.

// Note! If you use this on Arduino Mega, I prefer to also connect interrupt line
// for MCP2515 and define N2k_CAN_INT_PIN to related line. E.g. MessageSender
// sends so many messages that Mega can not handle them. If you only use
// received messages internally without slow operations, then youmay survive
// without interrupt.

#include <Arduino.h>
//#include <Time.h>  // 
#define N2k_CAN_INT_PIN 21
#include <NMEA2000_CAN.h>
#include <N2kMessages.h>
#include <N2kMessagesEnumToStr.h>


#include <SPI.h>
#include <ST7735_t3.h> // Hardware-specific library
#include <ST7789_t3.h> // Hardware-specific library
#include <ST7735_t3_font_Arial.h>
#include <ST7735_t3_font_ArialBold.h>

#define TFT_RST    14   // chip reset
#define TFT_DC     9   // tells the display if you're sending data (D) or commands (C)   --> WR pin on TFT
#define TFT_MOSI   11  // Data out    (SPI standard)
#define TFT_SCLK   13  // Clock out   (SPI standard)
#define TFT_CS     10  // chip select (SPI standard)


ST7789_t3 tft = ST7789_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
int LCD_BL = 15;       // LCD back light control

typedef struct {
  unsigned long PGN;
  void (*Handler)(const tN2kMsg &N2kMsg); 
} tNMEA2000Handler;

void SystemTime(const tN2kMsg &N2kMsg);


void Wind(const tN2kMsg &N2kMsg);
void Temperature(const tN2kMsg &N2kMsg);


tNMEA2000Handler NMEA2000Handlers[]={
  {126992L,&SystemTime},
  {130312L,&Temperature},
  {130306L,&Wind},
  {0,0}
};

Stream *OutputStream;

void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);

void setup() {

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);  // Turn LCD backlight on
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLUE);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5,20);
  tft.setFont(Arial_18);

  tft.println("Teensy 4.0");
  tft.println("NMEA 2000 Demo"); 
  tft.println(" "); 
  tft.println("www.skpang.co.uk"); 

  delay(2000);
  Serial.println("Teensy 4.0 Temperature Gauge. Nov 2020 skpang.co.uk");

  tft.fillScreen(ST77XX_BLUE);

  tft.setFont(Arial_14);
  tft.setCursor(10,10);
  tft.print("Wind Speed");

  tft.setFont(Arial_14);
  tft.setCursor(10,90);
  tft.print("Wind Direction");
  
  tft.setFont(Arial_14);
  tft.setCursor(10,180);
  tft.print("Temperature");

  tft.setFont(Arial_16);
  tft.setCursor(140,53);
  tft.print("m/s");

  tft.setFont(Arial_16);
  tft.setCursor(140,110);
  tft.print("o");      

  tft.setFont(Arial_12);
  tft.setCursor(140,207);
  tft.print("o");
  tft.setFont(Arial_16);
  tft.setCursor(150,220);
  tft.print("C");
          
          
  OutputStream=&Serial;
   
//  NMEA2000.SetN2kCANReceiveFrameBufSize(50);
  // Do not forward bus messages at all
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);
  NMEA2000.SetForwardStream(OutputStream);
  // Set false below, if you do not want to see messages parsed to HEX withing library
  NMEA2000.EnableForward(false);
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
//  NMEA2000.SetN2kCANMsgBufSize(2);
  NMEA2000.Open();
  OutputStream->print("Running...");
}

//*****************************************************************************
template<typename T> void PrintLabelValWithConversionCheckUnDef(const char* label, T val, double (*ConvFunc)(double val)=0, bool AddLf=false ) {
  OutputStream->print(label);
  if (!N2kIsNA(val)) {
    if (ConvFunc) { OutputStream->print(ConvFunc(val)); } else { OutputStream->print(val); }
  } else OutputStream->print("not available");
  if (AddLf) OutputStream->println();
}
void Temperature(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    unsigned char TempInstance;
    tN2kTempSource TempSource;
    double ActualTemperature;
    double SetTemperature;

    if (ParseN2kTemperature(N2kMsg,SID,TempInstance,TempSource,ActualTemperature,SetTemperature) ) 
    {
               
        PrintLabelValWithConversionCheckUnDef(", actual temperature: ",ActualTemperature,&KelvinToC);
        tft.fillRect(10,200,130,42,ST77XX_BLUE);  
        tft.setFont(Arial_32_Bold);
        tft.setCursor(10,200);
        tft.print(ActualTemperature -273.15);



    } else {
      OutputStream->print("Failed to parse PGN: ");  OutputStream->println(N2kMsg.PGN);
    }
}
//*****************************************************************************
void SystemTime(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    uint16_t SystemDate;
    double SystemTime;
    tN2kTimeSource TimeSource;
    
    if (ParseN2kSystemTime(N2kMsg,SID,SystemDate,SystemTime,TimeSource) ) {
      PrintLabelValWithConversionCheckUnDef("System time: ",SID,0,true);
      PrintLabelValWithConversionCheckUnDef("  days since 1.1.1970: ",SystemDate,0,true);
      PrintLabelValWithConversionCheckUnDef("  seconds since midnight: ",SystemTime,0,true);
                        OutputStream->print("  time source: "); PrintN2kEnumType(TimeSource,OutputStream);
    } else {
      OutputStream->print("Failed to parse PGN: "); OutputStream->println(N2kMsg.PGN);
    }
}

//*****************************************************************************
void Wind(const tN2kMsg &N2kMsg) {
   double pi = 3.14;

   unsigned char SID;
   double WindSpeed;
   double WindAngle; 

   int WindAngle_degree;
   
   tN2kWindReference WindReference;
    
   if(ParseN2kWindSpeed(N2kMsg,SID,  WindSpeed, WindAngle, WindReference)) 
   {
      Serial.print(WindSpeed);
      Serial.print(" ");
      Serial.print(WindAngle);

      tft.fillRect(10,28,130,42,ST77XX_BLUE);
      tft.setFont(Arial_40_Bold);
      tft.setCursor(10,28);
      tft.print(WindSpeed);

      WindAngle_degree = WindAngle * (180/pi);
      Serial.print(" ");
      Serial.println(WindAngle_degree);

      tft.fillRect(10,108,130,42,ST77XX_BLUE);
      tft.setFont(Arial_40_Bold);
      tft.setCursor(10,108);
      tft.print(WindAngle_degree);
      
      
    } else {
      OutputStream->print("Failed to parse PGN: "); OutputStream->println(N2kMsg.PGN);
    }
}

//*****************************************************************************
void printLLNumber(Stream *OutputStream, unsigned long long n, uint8_t base=10)
{
  unsigned char buf[16 * sizeof(long)]; // Assumes 8-bit chars.
  unsigned long long i = 0;

  if (n == 0) {
    OutputStream->print('0');
    return;
  }

  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--)
    OutputStream->print((char) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
}

//*****************************************************************************
void BinaryStatusFull(const tN2kMsg &N2kMsg) {
    unsigned char BankInstance;
    tN2kBinaryStatus BankStatus;

    if (ParseN2kBinaryStatus(N2kMsg,BankInstance,BankStatus) ) {
      OutputStream->print("Binary status for bank "); OutputStream->print(BankInstance); OutputStream->println(":");
      OutputStream->print("  "); //printLLNumber(OutputStream,BankStatus,16);
      for (uint8_t i=1; i<=28; i++) {
        if (i>1) OutputStream->print(",");
        PrintN2kEnumType(N2kGetStatusOnBinaryStatus(BankStatus,i),OutputStream,false);
      }
      OutputStream->println();
    }
}

//*****************************************************************************
void BinaryStatus(const tN2kMsg &N2kMsg) {
    unsigned char BankInstance;
    tN2kOnOff Status1,Status2,Status3,Status4;

    if (ParseN2kBinaryStatus(N2kMsg,BankInstance,Status1,Status2,Status3,Status4) ) {
      if (BankInstance>2) { // note that this is only for testing different methods. MessageSender.ini sends 4 status for instace 2
        BinaryStatusFull(N2kMsg);
      } else {
        OutputStream->print("Binary status for bank "); OutputStream->print(BankInstance); OutputStream->println(":");
        OutputStream->print("  Status1=");PrintN2kEnumType(Status1,OutputStream,false);
        OutputStream->print(", Status2=");PrintN2kEnumType(Status2,OutputStream,false);
        OutputStream->print(", Status3=");PrintN2kEnumType(Status3,OutputStream,false);
        OutputStream->print(", Status4=");PrintN2kEnumType(Status4,OutputStream,false);
        OutputStream->println();
      }
    }
}



//*****************************************************************************
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  int iHandler;
  
  // Find handler
  //OutputStream->print("In Main Handler: "); OutputStream->println(N2kMsg.PGN);
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  
  if (NMEA2000Handlers[iHandler].PGN!=0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg); 
  }
}

//*****************************************************************************
void loop() 
{ 
  NMEA2000.ParseMessages();
}
