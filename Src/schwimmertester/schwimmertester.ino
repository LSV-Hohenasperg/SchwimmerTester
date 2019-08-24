/*
 * ===========================================================================
 *                                    SCHWIMMERTESTER
 * 
 * Description: schwimmertester is a Arduino-Nano-Sketch, to control a vakuum pump,
 *              two valves and to read a pressure sensor. The board is equipped with
 *              a Touch-LCD screen. 
 * Author(s)  : Thorsten Klinkhammer (klinkhammer@gmail.com)
 *              Markus Kuhnla (markus@kuhnla.de)
 * Date       : August, 24th 2019
 * ===========================================================================
 */


/*+++++++++++++++++++++++++++++
 *          INCLUDES
 *+++++++++++++++++++++++++++++
 */

//Include TFT- and Touchscreen libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <MCUFRIEND_kbv.h>

//Include libc headers
#include <stdio.h>
#include <math.h>


/*+++++++++++++++++++++++++++++
 *          DEFINES 
 *+++++++++++++++++++++++++++++
 */
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// Pinout used for the TFT
//  LCD_D2 connects to D2
//  LCD_D3 connects to D3
//  LCD_D4 connects to D4
//  LCD_D5 connects to D4
//  LCD_D6 connects to D6
//  LCD_D7 connects to D7
//  LCD_D0 connects to D8
//  LCD_D1 connects to D9
//  
//  LCD_RST connects to A4
//  LCD_CS  connects to A3
//  LCD_RS  connects to A2
//  LCD_WR  connects to A1
//  LCD_RD  connects to A0


//Pinout used for the touch panel
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin


//Pinout used for sensors and valves
#define PIN_PRESSURE    A7
#define PIN_SUCCTION    10   //13=D10
#define PIN_VENTILATION 11   //14=D11
#define PIN_PUMP        12   //15=D12

//Parameters for the TouchScreen
#define TS_MINX 75
#define TS_MAXX 930
#define TS_MINY 130
#define TS_MAXY 905
#define MINPRESSURE 10
#define MAXPRESSURE 1000


#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Color definitions
#define BLACK       0x0000      /*   0,   0,   0 */
#define NAVY        0x000F      /*   0,   0, 128 */
#define DARKGREEN   0x03E0      /*   0, 128,   0 */
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define BLUE        0x001F      /*   0,   0, 255 */
#define GREEN       0x07E0      /*   0, 255,   0 */
#define CYAN        0x07FF      /*   0, 255, 255 */
#define RED         0xF800      /* 255,   0,   0 */
#define MAGENTA     0xF81F      /* 255,   0, 255 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define WHITE       0xFFFF      /* 255, 255, 255 */
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define PINK        0xF81F

#define BUTTON_W 80
#define BUTTON_H 30
#define BUTTON_TEXTSIZE 2

//Output number with 4 digits right aligned to the screen
#define NUMBER2SCREEN(n,x,y,s) { char numBuf[5];                                          \
                                 sprintf(numBuf,"%04d",n);                                \
                                 for (int i=0; i<=3 && numBuf[i]=='0';i++) numBuf[i]=' '; \
                                 s.setCursor(x,y);                                        \
                                 s.setTextColor(BLACK, WHITE); s.setTextSize(2);        \
                                 s.print(numBuf);                                         \
                               }


/*+++++++++++++++++++++++++++++
 *          GLOBALS 
 *+++++++++++++++++++++++++++++
 */

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

volatile int32_t menuTimeSinceLastUpdate;
volatile int32_t menuLastUpdate = 0;
volatile int32_t menuActualTime;
volatile uint32_t lcdTimeSinceLastUpdate;
volatile uint32_t lcdLastUpdate = 0;
volatile uint32_t lcdActualTime;

volatile uint8_t drawMenuPage = 1;
volatile uint8_t activeMenuPage = 1;

volatile uint8_t stateIntakeValve = 0;
volatile uint8_t stateVentValve = 0;
volatile uint8_t statePump = 0;

volatile int32_t touchXPos = 1;
volatile int32_t touchYPos = 1;

volatile int32_t pressureValue = 0;
volatile int32_t flightHeight = 0;
volatile int32_t altitudeHeigthM = 0;
volatile int32_t altitudeHeigthFT = 0;

#define FILTERFACTOR 0.6 //Damp input values by 60% 
#define TOLERANCE    10  //Accept measurements in a window of +- 10

volatile float currentPressure = 1013; //Current pressure value



// logo un Menu Page 1
#define LOGO_X 0
#define LOGO_Y 0
#define LOGO_W 320
#define LOGO_H 35
#define LOGO_TSIZE 2

Adafruit_GFX_Button BTN_Main[9];
char mainButtonLabels[9][7] = {"20","20","3000","50",
                            "Start","Stop",
                            "Vlv1", "Vlv2", "Pump"};
                            
uint16_t mainButtonColors[9] = {DARKGREY, DARKGREY,DARKGREY,DARKGREY,
                            DARKGREEN, RED,
                            DARKGREY, DARKGREY, DARKGREY};

Adafruit_GFX_Button BTN_Edit[15];
char editButtonLabels[15][6] = {"Back","Clear","OK",
                                "1","2","3",
                                "4","5","6",
                                "7","8","9",
                                "+","0","-"};
                            
uint16_t editButtonColors[15] = {RED, DARKGREY,GREEN,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 PURPLE,DARKGREY,PURPLE,};

/*=========================================================================================
 * 
 *                    F U N C T I O N S
 * 
 ==========================================================================================*/

/*
===============================================================================
Function  : lowPass
Synopsis  : float lowPass(float factor, float measure)
Discussion: Filters measurements using a low pass filter. The smoothing
            factor is adjustable between 0 and 1. I.e: if you set the factor
            to 0.1, the old median will be used with 10% and the new reading
            has a weight of 90% (very low smoothing). 
Return    : New filtered measurement value (float)
*/
float lowPass(float factor, float measure)
{
    return (currentPressure*factor + measure) / (factor +1.0);
}

/*
===============================================================================
Function  : newMeasurement
Synopsis  : void newValue(void)
Discussion: This function is intended to be called by an interrupt. The
            interrupt should be setup on ValueChange at the measurement pin.
Return    : void
*/

void newMeasurement(void)
{
    float newMeasure = 1013-(analogRead(PIN_PRESSURE)/204.8-0.21)*250;
    if (newMeasure > 1013)
      newMeasure =1013;

    //Critical in
    currentPressure = lowPass(FILTERFACTOR,newMeasure);
    //Critical out
}

/*
===============================================================================
Function  : tolerance
Synopsis  : int tolerance(float measurement,float goal)
Discussion: Determine weather the current measurement is in the acceptable
            window
Return    : positive - Masurement is too high
            0        - Measurement is in tolerance window
            negative - Measurement is too low
*/
int tolerance(float measurement, float goal)
{
    float delta;
    
    delta = measurement - goal;
    
    if  (abs(delta) <= TOLERANCE)
    {
        return 0; //Measurement tolerable
    }

    if (measurement < goal-TOLERANCE)
    {
        return -1; //Measurement too low
    }
    
    return 1; //Measurement too high
}

/*
===============================================================================
Function  : climb
Synopsis  : int climb(float goal)
Discussion: Setup of all valves and relais to make the chamber climb
Return    : 0 - climbing not needed
            1 - climbing required
*/
int climb(float goal)
{
    if (tolerance(currentPressure,goal) != -1)
    {
        digitalWrite(PIN_PUMP,LOW);        //Switch on pump
        digitalWrite(PIN_SUCCTION,LOW);    //Switch on sucction valve
        digitalWrite(PIN_VENTILATION,LOW); //Switch on ventilation valve
        return 1;
    }
    return 0;
}

/*
===============================================================================
Function  : descend
Synopsis  : int descend(float goal)
Discussion: Setup of all valves and relais to make the chamber descend
Return    : 0 - Descending not needed
            1 - Descend required
*/
int descend(float goal)
{
    if (tolerance(currentPressure,goal) != 1)
    {
        digitalWrite(PIN_PUMP,HIGH);        //Switch off pump
        digitalWrite(PIN_SUCCTION,HIGH);    //Switch off sucction valve
        digitalWrite(PIN_VENTILATION,LOW);  //Switch on ventilation valve
        return 1;
    }
    return 0;
}

/*
===============================================================================
Function  : level
Synopsis  : void level(float pressure)
Discussion: Maintain the current pressure level
Return    : void
*/
void level(float pressure)
{
    if ( !climb(pressure) && !descend(pressure))
    {
        digitalWrite(PIN_PUMP,HIGH);        //Switch off pump
        digitalWrite(PIN_SUCCTION,HIGH);    //Switch off sucction valve
        digitalWrite(PIN_VENTILATION,HIGH); //Switch off ventilation valve
    }
}

 
void setup(void) {
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));

  tft.reset();
  tft.begin(tft.readID());
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  pinMode(PIN_PRESSURE, INPUT);
  pinMode(PIN_SUCCTION, OUTPUT);
  pinMode(PIN_VENTILATION, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_SUCCTION,HIGH);
  digitalWrite(PIN_VENTILATION,HIGH);
  digitalWrite(PIN_PUMP,HIGH);

  //Now setup timer0 to generate an interrupt every 16 ms
  TCCR0B|=(1<<CS02);    //Set the prescaler to 1024
  TCCR0B|=(1<<CS00);

  
  TCCR0A=(1<<WGM01);  //Set timer mode to Count and Clear (CTC)   
  OCR0A=0xFF;         //The Formula for OCR is: OCR=16MHz/Prescaler*desiredSeconds -1
 
  TIMSK0|=(1<<OCIE0A);   //Set the interrupt request
  sei();                 //Enable interrupt

}

ISR(TIMER0_COMPA_vect){    //This is the interrupt service routine for timer0
  //Measure the pressure
  newMeasurement();
}


void loop()
{
  touchYPos = 0;
  touchXPos = 0;
  menuActualTime = micros();
  menuTimeSinceLastUpdate = menuActualTime - menuLastUpdate;
  if (menuTimeSinceLastUpdate >= 5000){
    menuLastUpdate = menuActualTime;
    
    TSPoint p = ts.getPoint();

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      // scale from 0->1023 to tft.width 
     touchYPos = (tft.height() - map(p.x, TS_MINX, TS_MAXX, tft.height(), 0));
     touchXPos = (tft.width()-map(p.y, TS_MAXY, TS_MINY, tft.width(), 0));
    }
    
    if (activeMenuPage == 1){
      if (drawMenuPage == 1){
        drawMenuPage = 0;
        tft.fillScreen(WHITE);
        tft.fillRect(LOGO_X, LOGO_Y, LOGO_W, LOGO_H, LIGHTGREY);
        tft.setCursor(LOGO_X + 35, LOGO_Y+10);
        tft.setTextColor(BLACK);
        tft.setTextSize(LOGO_TSIZE);
        tft.print("Float Gauge Test Unit");

        tft.setTextColor(BLACK); 
        tft.setTextSize(2);
        tft.setCursor(9, 50);
        tft.print("Actual Pres.:        mbar");
        tft.setCursor(9, 70);
        tft.print("Comp. Alt.:          m");
        tft.setCursor(9, 90);
        tft.print("Comp. Alt.:          ft");
        tft.setCursor(9, 110);
        tft.print("Actual Cycle:        cyc");
        tft.setCursor(9, 150);
        tft.print("Time GND [sec]");
        tft.setCursor(9, 200);
        tft.print("Time FLIGHT [sec]"); 
        tft.setCursor(9, 250);
        tft.print("Max Altitude [ft]"); 
        tft.setCursor(9, 300);
        tft.print("No. of CYC [cyc]"); 
           
        tft.fillRect(0, 445, 320, 35, NAVY);
        tft.setCursor(LOGO_X + 30, 445+10);
        tft.setTextColor(WHITE);
        tft.setTextSize(LOGO_TSIZE);
        tft.print("by LSV-Hohenasperg e.V.");

        // create buttons
        BTN_Main[0].initButton(&tft, 275,155, BUTTON_W, BUTTON_H, BLACK, 
                              mainButtonColors[0], WHITE,
                              mainButtonLabels[0], BUTTON_TEXTSIZE); 
        BTN_Main[0].drawButton();
        BTN_Main[1].initButton(&tft, 275,205, BUTTON_W, BUTTON_H, BLACK, 
                              mainButtonColors[1], WHITE,
                              mainButtonLabels[1], BUTTON_TEXTSIZE); 
        BTN_Main[1].drawButton();
        BTN_Main[2].initButton(&tft, 275,255, BUTTON_W, BUTTON_H, BLACK, 
                              mainButtonColors[2], WHITE,
                              mainButtonLabels[2], BUTTON_TEXTSIZE); 
        BTN_Main[2].drawButton();
        BTN_Main[3].initButton(&tft, 275,305, BUTTON_W, BUTTON_H, BLACK, 
                              mainButtonColors[3], WHITE,
                              mainButtonLabels[3], BUTTON_TEXTSIZE); 
        BTN_Main[3].drawButton();
        
        BTN_Main[4].initButton(&tft, 85, 355, 130, BUTTON_H, BLACK, 
                              mainButtonColors[4], WHITE,
                              mainButtonLabels[4], BUTTON_TEXTSIZE); 
        BTN_Main[4].drawButton();
        BTN_Main[5].initButton(&tft, 235,355, 130, BUTTON_H, BLACK, 
                              mainButtonColors[5], WHITE,
                              mainButtonLabels[5], BUTTON_TEXTSIZE); 
        BTN_Main[5].drawButton();
        
        BTN_Main[6].initButton(&tft, 60,420, 80, BUTTON_H, BLACK, 
                              mainButtonColors[6], WHITE,
                              mainButtonLabels[6], BUTTON_TEXTSIZE); 
        BTN_Main[6].drawButton();
        
        BTN_Main[7].initButton(&tft, 160, 420, 80, BUTTON_H, BLACK, 
                              mainButtonColors[7], WHITE,
                              mainButtonLabels[7], BUTTON_TEXTSIZE); 
        BTN_Main[7].drawButton();
        BTN_Main[8].initButton(&tft, 260,420, 80, BUTTON_H, BLACK, 
                              mainButtonColors[8], WHITE,
                              mainButtonLabels[8], BUTTON_TEXTSIZE); 
        BTN_Main[8].drawButton();

      } // end: drawMenuPage == 1
      
      // go thru all the buttons, checking if they were pressed
      for (uint8_t b=0; b<9; b++) {
        if (BTN_Main[b].contains(touchXPos,touchYPos)) {
          //Serial.print("Pressing: "); Serial.println(b);
          BTN_Main[b].press(true);  // tell the button it is pressed
          menuLastUpdate = menuLastUpdate+300000;
          break;
        } else {
        BTN_Main[b].press(false);  // tell the button it is NOT pressed
        }
        /*
        if (BTN_Main[b].justReleased()) {
          Serial.print("Released: "); Serial.println(b);
          BTN_Main[b].drawButton();  // draw normal
          }
        */
      }

      // now we can ask the buttons if their state has changed
      for (uint8_t b=0; b<9; b++) {
        if (BTN_Main[b].justPressed()) {
          
          if (b == 4) {
            Serial.print("Start");
          }
          
          if (b == 5) {
            Serial.print("Stop");
          }
          
          if (b == 6) {
            if (stateIntakeValve == 0){
              stateIntakeValve = 1;
              digitalWrite(PIN_SUCCTION,LOW);
              BTN_Main[b].drawButton(true);  // draw invert!
            } else {
              stateIntakeValve = 0;
              digitalWrite(PIN_SUCCTION,HIGH);
              BTN_Main[b].drawButton();  // draw invert! 
            }
          }
          
          if (b == 7) {
            if (stateVentValve == 0){
              stateVentValve = 1;
              digitalWrite(PIN_VENTILATION,LOW);
              BTN_Main[b].drawButton(true);  // draw invert!
            } else {
              stateVentValve = 0;
              digitalWrite(PIN_VENTILATION,HIGH);
              BTN_Main[b].drawButton();  // draw invert! 
            }
          }
          
          if (b == 8) {
            if (statePump == 0){
              statePump = 1;
              digitalWrite(PIN_PUMP,LOW);
              BTN_Main[b].drawButton(true);  // draw invert!
            } else {
              statePump = 0;
              digitalWrite(PIN_PUMP,HIGH);
              BTN_Main[b].drawButton();  // draw invert! 
            }
          }

        }
      }
              
    } // end: activeMenuPage == 1
  } // end: menuTimeSinceLastUpdate >= 5000

  
    /*---------------------update measured values on menu page -------------------------------*/
  if (activeMenuPage == 1){
    lcdActualTime = micros();
    lcdTimeSinceLastUpdate = lcdActualTime - lcdLastUpdate;
    if (lcdTimeSinceLastUpdate >= 500000){
      lcdLastUpdate = lcdActualTime;
      NUMBER2SCREEN(currentPressure,200,50,tft);
    
      altitudeHeigthM = (288.15/0.0065) * (1-pow((float) currentPressure / 1013,(1/5.255)));
      NUMBER2SCREEN(altitudeHeigthM,188,70,tft);

      altitudeHeigthFT = altitudeHeigthM*3.28084;
      NUMBER2SCREEN(altitudeHeigthFT,188,90,tft);
    }
  }
  
 

}
