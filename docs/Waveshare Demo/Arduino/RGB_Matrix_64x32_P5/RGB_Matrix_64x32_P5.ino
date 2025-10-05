#include "RGBmatrixPanel.h"
#include "bit_bmp.h"
#include "fonts.h"
#include <string.h>
#include <stdlib.h>
#define CLK 11 
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);

void setup()
{
  Serial.begin(115200);
  Reginit();
  matrix.begin();
  delay(500);
}

void loop()
{
  //Demo_0();
  Demo_1();
  //Demo_2();
}

void display_text(int x, int y, char *str, const GFXfont *f, int color, int pixels_size)
{
  matrix.setTextSize(pixels_size);// size 1 == 8 pixels high
  matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves
  matrix.setFont(f);      //set font
  matrix.setCursor(x, y);
  matrix.setTextColor(color);
  matrix.println(str);
}

//Clear screen
void screen_clear()
{
  matrix.fillRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(0, 0, 0));
}

void Demo_0()
{
  screen_clear();
  display_text(1, 1, "Wave", NULL, matrix.Color333(0, 100, 255), 2); // this text need to be printed slightly larger and over the 2 displays, not duplicated.
  delay(500);
  display_text(1, 16, "share", NULL, matrix.Color333(0, 100, 255), 2); // this text need to be printed slightly larger and over the 2 displays, not duplicated.
  delay(500);
  screen_clear();
  display_text(1, 1, "RGB", NULL, matrix.Color333(0, 100, 255), 2); // this text need to be printed slightly larger and over the 2 displays, not duplicated.
  delay(500);
  display_text(40, 1, "P5", NULL, matrix.Color333(0, 100, 255), 2); // this text need to be printed slightly larger and over the 2 displays, not duplicated.
  delay(500);
  display_text(1, 16, "64x32", NULL, matrix.Color333(0, 100, 255), 2); // this text need to be printed slightly larger and over the 2 displays, not duplicated.
  delay(500);
  screen_clear();
}

void Demo_1()
{
  // draw a pixel in solid white
  screen_clear();
  matrix.setFont(NULL);
  matrix.drawPixel(0, 0, matrix.Color333(7, 7, 7));
  delay(500);

    matrix.fillRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(0, 7, 0));
    delay(500);

    matrix.drawRect(0, 0, matrix.width(), matrix.height(), matrix.Color333(7, 7, 0));
    delay(500);

    // draw an 'X' in red
    matrix.drawLine(0, 0, matrix.width()-1, matrix.height()-1, matrix.Color333(7, 0, 0));
    matrix.drawLine(matrix.width()-1, 0, 0, matrix.height()-1, matrix.Color333(7, 0, 0));
    delay(500);

    // draw a blue circle
    matrix.drawCircle(10, 10, 10, matrix.Color333(0, 0, 7));
    delay(500);

    // fill a violet circle
    matrix.fillCircle(40, 21, 10, matrix.Color333(7, 0, 7));
    delay(500);

    // fill the screen with 'black'
    screen_clear();

    // draw some text!
    matrix.setTextSize(1);     // size 1 == 8 pixels high
    matrix.setTextWrap(false); // Don't wrap at end of line - will do ourselves

    matrix.setCursor(5, 0);    // start at top left, with 8 pixel of spacing
    uint8_t w = 0;
    char *str = "WaveshareElectronics";
    for (w=0; w<9; w++) {
      matrix.setTextColor(Wheel(w));
      matrix.print(str[w]);
    }
    matrix.setCursor(0, 8);    // next line
    for (w=9; w<20; w++) {
      matrix.setTextColor(Wheel(w));
      matrix.print(str[w]);
    }
    matrix.println();
    //matrix.setTextColor(matrix.Color333(4,4,4));
    //matrix.println("Industries");
    matrix.setTextColor(matrix.Color333(7,7,7));
    matrix.println("LED MATRIX!");

    // print each letter with a rainbow color
    matrix.setTextColor(matrix.Color333(7,0,0));
    matrix.print('6');
    matrix.setTextColor(matrix.Color333(7,4,0));
    matrix.print('4');
    matrix.setTextColor(matrix.Color333(7,7,0));
    matrix.print('x');
    matrix.setTextColor(matrix.Color333(4,7,0));
    matrix.print('3');
    matrix.setTextColor(matrix.Color333(0,7,0));
    matrix.print('2');
    matrix.setCursor(40, 24);

    matrix.setTextColor(matrix.Color333(0,4,7));
    matrix.print('R');
    matrix.setTextColor(matrix.Color333(0,0,7));
    matrix.print('G');
    matrix.setTextColor(matrix.Color333(4,0,7));
    matrix.print('B');
    delay(2000);
}

void Demo_2()
{
    screen_clear();
    matrix.DrawString_CN( 1   , 0, "微", &Font16CN, matrix.Color333(7, 0, 0));
    matrix.DrawString_CN( 1+16, 0, "雪", &Font16CN, matrix.Color333(6, 1, 0));
    matrix.DrawString_CN( 1+32, 0, "电", &Font16CN, matrix.Color333(5, 2, 7));
    matrix.DrawString_CN( 1+48, 0, "子", &Font16CN, matrix.Color333(0, 7, 0));   
    matrix.DrawString_CN( 1    ,16, "欢", &Font16CN, matrix.Color333(0, 6, 1));
    matrix.DrawString_CN( 1+16, 16, "迎", &Font16CN, matrix.Color333(0, 5, 2));
    matrix.DrawString_CN( 1+32, 16, "您", &Font16CN, matrix.Color333(0, 0, 7));
    matrix.DrawString_CN( 1+48, 16, "！", &Font16CN, matrix.Color333(1, 0, 6));
    delay(2000);
    screen_clear();
    matrix.DrawString_CN( 1   ,  0, "微", &Font32CN, matrix.Color333(0, 7, 7));
    matrix.DrawString_CN( 1+32,  0, "雪", &Font32CN, matrix.Color333(0, 7, 7));
    delay(1000);
    screen_clear();
    matrix.DrawString_CN( 1   ,  0,"电", &Font32CN, matrix.Color333(0, 7, 7));
    matrix.DrawString_CN( 1+32,  0,"子", &Font32CN, matrix.Color333(0, 7, 7));
    delay(1000);
    screen_clear();
    matrix.DrawString_CN( 1   ,   0, "欢", &Font32CN, matrix.Color333(0, 7, 7));
    matrix.DrawString_CN( 1+32,   0, "迎", &Font32CN, matrix.Color333(0, 7, 7));
    delay(1000);
    screen_clear();
    matrix.DrawString_CN( 1   ,   0,"您", &Font32CN, matrix.Color333(0, 7, 7));
    matrix.DrawString_CN( 1+37,   0,"！", &Font32CN, matrix.Color333(0, 7, 7));
    delay(1000);
    screen_clear();
  
}

// Input a value 0 to 7 to get a color value.
// The colours are a transition r - g - b - back to r.
uint16_t Wheel(byte WheelPos) {
  if(WheelPos < 8) {
   return matrix.Color333(7 - WheelPos, WheelPos, 0);
  } else if(WheelPos < 16) {
   WheelPos -= 8;
   return matrix.Color333(0, 7-WheelPos, WheelPos);
  } else {
   WheelPos -= 16;
   return matrix.Color333(WheelPos, 0, 7 - WheelPos);
  }
}

void Reginit()
{
    pinMode(24, OUTPUT); //R1
    pinMode(25, OUTPUT); //G1
    pinMode(26, OUTPUT); //B1
    pinMode(27, OUTPUT); //R2
    pinMode(28, OUTPUT); //G2
    pinMode(29, OUTPUT); //B2
    pinMode(CLK, OUTPUT);
    pinMode(OE, OUTPUT);
    pinMode(LAT, OUTPUT);

    digitalWrite(OE, HIGH);
    digitalWrite(LAT, LOW);
    digitalWrite(CLK, LOW);
    int MaxLed = 64;

    int C12[16] = {0, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1};
    int C13[16] = {0, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0,  0, 0, 0, 0, 0};

    for (int l = 0; l < MaxLed; l++)
    {
        int y = l % 16;
        digitalWrite(24, LOW);
        digitalWrite(25, LOW);
        digitalWrite(26, LOW);
        digitalWrite(27, LOW);
        digitalWrite(28, LOW);
        digitalWrite(29, LOW);
        if (C12[y] == 1)
        {
          digitalWrite(24, HIGH);
          digitalWrite(25, HIGH);
          digitalWrite(26, HIGH);
          digitalWrite(27, HIGH);
          digitalWrite(28, HIGH);
          digitalWrite(29, HIGH);
        }
        if (l > MaxLed - 12)
        {
            digitalWrite(LAT, HIGH);
        }
        else
        {
            digitalWrite(LAT, LOW);
        }
        digitalWrite(CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(CLK, LOW);
    }
    digitalWrite(LAT, LOW);
    digitalWrite(CLK, LOW);

    // Send Data to control register 12
    for (int l = 0; l < MaxLed; l++)
    {
        int y = l % 16;
        digitalWrite(24, LOW);
        digitalWrite(25, LOW);
        digitalWrite(26, LOW);
        digitalWrite(27, LOW);
        digitalWrite(28, LOW);
        digitalWrite(29, LOW);
        if (C13[y] == 1)
        {
            digitalWrite(24, HIGH);
            digitalWrite(25, HIGH);
            digitalWrite(26, HIGH);
            digitalWrite(27, HIGH);
            digitalWrite(28, HIGH);
            digitalWrite(29, HIGH);
        }
        if (l > MaxLed - 13)
        {
            digitalWrite(LAT, HIGH);
        }
        else
        {
            digitalWrite(LAT, LOW);
        }
        digitalWrite(CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(CLK, LOW);
    }
    digitalWrite(LAT, LOW);
    digitalWrite(CLK, LOW);
}
