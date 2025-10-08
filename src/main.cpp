#include <Arduino.h>
#include <RGBMatrix.h>


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  RGBMatrix::init();
}

void loop() {
  // put your main code here, to run repeatedly:
  RGBMatrix::setOrientation(180);
  RGBMatrix::clear();
  RGBMatrix::demo();
  delay(1000);
  
  RGBMatrix::setOrientation(0);
  RGBMatrix::clear();
  RGBMatrix::demo();
  delay(1000);

}
