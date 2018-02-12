g// Nicu Florica (aka niq_ro) from http://www.tehnic.go.ro made a small change for nice tft for low frequence (bellow 100MHz)
// it use info from http://full-chip.net/arduino-proekty/97-cifrovoy-fm-priemnik-na-arduino-i-module-rda5807-s-graficheskim-displeem-i-funkciey-rds.html
// original look like is from http://seta43.hol.es/radiofm.html
// version 1 - store frequency and volume step in EEPROM memory

#include <EEPROM.h> // info -> http://tronixstuff.com/2011/03/16/tutorial-your-arduinos-inbuilt-eeprom/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
//#include <TFT_ILI9163C.h>
#include <Adafruit_SSD1306.h>
#include <RDA5807M.h>
#include <RDSParser.h>
#include <radio.h>
//extern const uint8_t icon_radio[];
//extern const uint8_t icon_volume[];
RDA5807M radio;    ///< Create an instance of a RDA5807 chip radio
#define DEBUG 0
#define OLED_RESET 4

Adafruit_SSD1306 tft(OLED_RESET);
RDSParser rds;

enum RADIO_STATE {
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC          ///< executing the command.
};

RADIO_STATE state; ///< The state variable is used for parsing input characters.


const int entrada = A0; 
int entradaV = 0; 

int menu;
#define MAXmenu  3
int menux;
#define MAXmenux  3
static char* menuS[]= {" ","MANUAL TUNE","VOLUME     ","AUTO TUNE","INFO        "};


int volumen, volumenOld=7;
int frecuencia,frecuenciaOld;
int signal_level;

unsigned int z,z1;
byte xfrecu,xfrecuOld;
unsigned int estado[6];

unsigned long time,time1,time2,time3;


char buffer[30];
unsigned int RDS[4];
char seg_RDS[8];
char seg_RDS1[64];
char indexRDS1;

char hora,minuto,grupo,versio;
unsigned long julian;

 int mezcla;
//radio.debugEnable();
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}
void setup() 
{
  Wire.begin();   
  //Serial.begin(9600); 
  Serial.begin(57600);
 tft.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 radio.init();
 radio.debugEnable();
 tft.display();
  delay(20);

  // Clear the buffer.
  tft.clearDisplay();

// read value of last frequency
frecuencia = EEPROM.read(201);
volumen = EEPROM.read(202);

if (volumen < 0) volumen = 0;
if (volumen >15) volumen = 15;
if (frecuencia <0) frecuencia = 0;
if (frecuencia >210) frecuencia = 210;
  
  tft.setRotation(0);
  tft.setTextColor(WHITE);
 
tft.setCursor(95,0);
// tft.print("Nivel semnal :");
  tft.print(signal_level);tft.display();
   tft.print("/15 ");
   // draw an antenna
   tft.drawLine(103, 9, 103, 13, WHITE);
   tft.drawCircle(103,9,2,WHITE);

 
   WriteReg(0x02,0xC00d); // write 0xC00d into Reg.2 ( soft reset, enable,RDS, )
   WriteReg(0x05,0x84d8);  // write ,0x84d8 into Reg.3 
   
  time3=time2=time1=time = millis();
  menu=3;
  
  canal(frecuencia);
 // clearRDS;

  state = STATE_PARSECOMMAND;
 
  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(tftServiceName);
//  rds.attachTimeCallback(tftTime);
}
/*
void tftTime(uint8_t hour, uint8_t minute) {
  tft.setCursor(60, 100); 
  tft.setTextSize(2);    
  tft.setTextColor(GREENYELLOW, BLACK);
  if (hour < 10) tft.print('0');
  if (hour > 30) tft.print(' ');
  tft.print(hour);
  tft.print(':');
  if (minute < 10) tft.print('0');
  tft.println(minute); 
  tft.setTextColor(GREENYELLOW, BLACK);
} // tftTime()
*/
void tftServiceName(char *name)
{
  size_t len = strlen(name);
  tft.setCursor(0, 25);
  tft.setTextSize(1.5);    
  tft.setTextColor(WHITE, BLACK); 
  tft.print(name);tft.setTextColor(WHITE, BLACK);  tft.display();
  while (len < 8) {
    tft.print(' ');tft.setTextColor(WHITE, BLACK); tft.display();
    len++;  
  } // while
} // tftServiceName()
void loop() {
  int newPos;
  unsigned long now = millis();
 // static unsigned long nextFreqTime = 0;
  static unsigned long nextRadioInfoTime = 0;
  char c;
  entradaV = analogRead(entrada);
  
   #if DEBUG  
      Serial.print("sensor = " );  Serial.println(entradaV);delay(50);
   #endif
   
// Boton menu   
 if(entradaV>500 && entradaV<524)
   {
    Serial.print(entradaV);
    menu++;
    if(menu>MAXmenu)menu=1;
    Visualizar();

    #if DEBUG 
      Serial.print("menu = " );  Serial.println(menu); 
    #endif   
    while(1020>analogRead(entrada))delay(5);
   }
            
// Boton derecho
 if( entradaV<50)
   {
    Serial.print(entradaV);
    menux++;
    if(menux>MAXmenux)menux=MAXmenux;
    #if DEBUG 
      Serial.print("menux = " );  Serial.println(menux);
    #endif
    switch(menu)
      {
        case 1:
          frecuencia++;
          if(frecuencia>210)frecuencia=210; // верхняя граница частот
          delay(130);
        break;  
        case 2:
           volumen++;
           if(volumen>15)volumen=15;
           while(1020>analogRead(entrada))delay(5);
        break; 
        case 3:
           busqueda(0);
           while(1020>analogRead(entrada))delay(5);
        break; 
        case 4:
           // LcdClear();
            tft.clearDisplay();
           // visualPI();
            delay(3000);
           // LcdClear();
            tft.clearDisplay();
            frecuenciaOld=-1;
        break; 
      }              
   }
   
// Boton izquierdo
 if( entradaV<700 && entradaV>660)
 //if( entradaV<700 && entradaV>660)
   {
    Serial.print(entradaV);
    menux--;
    if(menux<1)menux=1; 
    #if DEBUG 
      Serial.print("menux = " );  Serial.println(menux);
    #endif   
    switch(menu)
      {
        case 1:
            frecuencia--;
            if(frecuencia<0)frecuencia=0;    
            delay(130);
        break;  
        case 2:
            volumen--;
            if(volumen<0)volumen=0;
            while(1020>analogRead(entrada))delay(5);
        break; 
        case 3:
            busqueda(1);
            while(1020>analogRead(entrada))delay(5);
        break; 
        case 4:
           // LcdClear();
          tft.clearDisplay();
            //tft.print(frecuencia);
            //visualPTY();
            delay(3000);
           // LcdClear();
            tft.clearDisplay();
            frecuenciaOld=-1;
        break; 
      }
    
   }
      
      if( millis()-time2>50)
          {
           ReadEstado();
           time1 = millis(); 
            //RDS   
           if ((estado[0] & 0x8000)!=0) {get_RDS();}
          }
     if( millis()-time3>500)
          {
            time3 = millis();
            Visualizar(); 

          }

    if( frecuencia!=frecuenciaOld)
          {  
            frecuenciaOld=frecuencia;                        
            z=870+frecuencia;
            EEPROM.write(201,frecuencia);
         #if DEBUG  
            Serial.print("Frecuencia = " );  Serial.println(frecuencia);
         #endif 
            sprintf(buffer,"%04d ",z);
  // tft.drawBitmap(80, 2,  icon_volume, 24, 10, WHITE);
  //tft.display();
            tft.setCursor(0,0); 
            tft.setTextSize(2);    
            tft.setTextColor(WHITE, BLACK); 
          for(z=0;z<5;z++)
               {
          if (z==0) {
          if (frecuencia < 130) tft.print(" ");
          else tft.print(buffer[0]);tft.setTextColor(WHITE, BLACK);
          
       
          }
                     
             if(z==3)  tft.print(",");tft.setTextColor(WHITE, BLACK); 
             
           if (z>0) tft.print(buffer[z]);tft.setTextColor(WHITE, BLACK); 
               }
               
               
       //   LcdString("MHz"); 
          tft.setCursor(65,5);
          tft.setTextSize(1);
          tft.setTextColor(WHITE, BLACK);  
          tft.print("MHz");
          tft.setTextColor(WHITE, BLACK); 
           tft.display();
        
          canal(frecuencia);
        //  clearRDS();
       }     

    //Cambio de volumen        
    if(volumen!=volumenOld)
        { 
          volumenOld=volumen;
          sprintf(buffer,"Volum %02d",volumen); tft.setCursor(62,25); tft.setTextSize(1); tft.setTextColor(WHITE, BLACK); tft.print(buffer);tft.print(" ");       
          tft.display();tft.print("/15");
          WriteReg(5, 0x84D0 | volumen);
          EEPROM.write(202,volumen);
        }    
         // check for RDS data
  radio.checkRDS();

  
}


void busqueda(byte direc)
{
  byte i;
  if(!direc) WriteReg(0x02,0xC30d); else  WriteReg(0x02,0xC10d);
  
  for(i=0;i<10;i++)
    {
      delay(200);      
      ReadEstado();      
      if(estado[0]&0x4000)
        {
          //Serial.println("Emisora encontrada");
          frecuencia=estado[0] & 0x03ff;  
          break;
        }       
    }
}

void Visualizar(void)
{ 
     
   //  tft.setCursor(3, 13); tft.setTextSize(2); tft.setTextColor(WHITE, BLACK); tft.print("FM"); 
    // tft.setCursor(27, 20); tft.setTextSize(1); tft.setTextColor(WHITE, BLACK); tft.print("RDA5807-"); 
      sprintf(buffer,"%s",menuS[menu]); tft.setCursor(5,17); tft.setTextSize(1); tft.setTextColor(WHITE,BLACK); tft.print(buffer); 
       //Detectar se&#241;al stereo
       tft.display();
       tft.setCursor(85,15);
       tft.setTextColor(BLACK, WHITE);
       if((estado[0] & 0x0400)==0)  tft.print("");   else     tft.setTextColor(BLACK, WHITE); tft.print("Stereo");  tft.setTextColor(BLACK, WHITE);      
       tft.display();
       //Se&#241;al 
     //  z=estado[1]>>10; sprintf(buffer,"S-%02d",z); tft.setCursor(58,0); tft.setTextColor(WHITE, BLACK); tft.print(buffer); tft.setCursor(25,0);tft.setTextColor(WHITE, BLACK);tft.print("Canal");
      sprintf(buffer,"Volum %02d",volumen); tft.setCursor(62,25);tft.setTextSize(1); tft.setTextColor(WHITE, BLACK); tft.print(buffer); //DUBLARE AFISARE VOLUM
       tft.display();tft.print("/15");
      frecuencia=estado[0] & 0x03ff;  
    
  }

void canal( int canal)
     {
       byte numeroH,numeroL;
       
       numeroH=  canal>>2;
       numeroL = ((canal&3)<<6 | 0x10); 
       Wire.beginTransmission(0x11);
       Wire.write(0x03);
         Wire.write(numeroH);                     // write frequency into bits 15:6, set tune bit         
         Wire.write(numeroL);
         Wire.endTransmission();
       }

//________________________ 
//RDA5807_adrr=0x11;       
// I2C-Address RDA Chip for random      Access
void WriteReg(byte reg,unsigned int valor)
{
  Wire.beginTransmission(0x11);
  Wire.write(reg); Wire.write(valor >> 8); Wire.write(valor & 0xFF);
  Wire.endTransmission();
  //delay(50);
}

//RDA5807_adrs=0x10;
// I2C-Address RDA Chip for sequential  Access
int ReadEstado()
{
 Wire.requestFrom(0x10, 12); 
 for (int i=0; i<6; i++) { estado[i] = 256*Wire.read ()+Wire.read(); }
 Wire.endTransmission();

}

//READ RDS  Direccion 0x11 for random access
void ReadW()
{
  // Wire.beginTransmission(0x11);            // Device 0x11 for random access
  // Wire.write(0x0C);                                // Start at Register 0x0C
  // Wire.endTransmission(0);                         // restart condition
  // Wire.requestFrom(0x11,8, 1);       // Retransmit device address with READ, followed by 8 bytes
  // for (int i=0; i<4; i++) {RDS[i]=256*Wire.read()+Wire.read();}        // Read Data into Array of Unsigned Ints
  // Wire.endTransmission();                  
 } 

 void get_RDS()
 {    
 /* int i;
  ReadW();      
  grupo=(RDS[1]>>12)&0xf;
      if(RDS[1]&0x0800) versio=1; else versio=0;  //Version A=0  Version B=1   
      if(versio==0)
      {
       #if DEBUG             
       sprintf(buffer,"Version=%d  Grupo=%02d ",versio,grupo); Serial.print(buffer);
   
    #endif 
    switch(grupo)
    {
     case 0:              
      #if DEBUG 
      Serial.print("_RDS0__");     
      #endif
      i=(RDS[1] & 3) <<1;
      seg_RDS[i]=(RDS[3]>>8);       
      seg_RDS[i+1]=(RDS[3]&0xFF);
      //tft.setCursor(20,100);tft.setTextColor(BLUE, BLACK); 
      for (i=0;i<8;i++)
      {
        #if DEBUG 
        Serial.write(seg_RDS[i]);   
        #endif
        
       // if(seg_RDS[i]>31 && seg_RDS[i]<128)
      //  tft.print(seg_RDS[i]);
       // else
      //  tft.setTextColor(BLUE, BLACK); 
        //tft.clearScreen();
    
      }  
       
      
      #if DEBUG                 
      Serial.println("---");
      #endif
      break;
     case 2:
      i=(RDS[1] & 15) <<2;              
      seg_RDS1[i]=(RDS[2]>>8);       
      seg_RDS1[i+1]=(RDS[2]&0xFF);
      seg_RDS1[i+2]=(RDS[3]>>8);       
      seg_RDS1[i+3]=(RDS[3]&0xFF);
      #if DEBUG 
      Serial.println("_RADIOTEXTO_");
              
              for (i=0;i<32;i++)  Serial.write(seg_RDS1[i]);                                    
              Serial.println("-TXT-");
              #endif      
              break;
              case 4:             
              i=RDS[3]& 0x003f;
              minuto=(RDS[3]>>6)& 0x003f;
              hora=(RDS[3]>>12)& 0x000f;
              if(RDS[2]&1) hora+=16;
              hora+=i;        
              z=RDS[2]>>1;
              julian=z;
              
              if(RDS[1]&1) julian+=32768;
              if(RDS[1]&2) julian+=65536;
              #if DEBUG 
              Serial.print("_DATE_");
              Serial.print(" Juliano=");Serial.print(julian);
             // sprintf(buffer," %02d:%02d ",hora,minuto); tft.setCursor(50,110); tft.setTextColor(CYAN, BLACK); tft.print(buffer); 
              Serial.println(buffer); 
              #endif            
              break;
              default:
              #if DEBUG 
              Serial.println("__"); 
              #endif    
              ;        
            }                        
          }                   
   */      } 
        




