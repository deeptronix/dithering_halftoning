
//#define USING_TEENSY

#include <Dither.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#include "nyan.h"
#define IMAGE image_data_nyan
const uint16_t img_width = 80;
const uint16_t img_height = 32;

Dither image(img_width, img_height);

uint8_t img_mod[img_width*img_height];  // Dithering can only happen on a RAM buffer; also, in this particular sketch, the image buffer is restored every time the dithering algorithm is changed
void setup(){
  
  delay(500);

  #ifdef USING_TEENSY
  Wire.setSDA(18);
  Wire.setSCL(19);
  #endif

  Serial.begin(115200);
  delay(10);
  Serial.println("Starting.");
  
  initDisplay();

  uint32_t t = micros();
  loadImageBuffer();
  t = micros() - t;
  
  Serial.println("Image downconverted in " + String(t) + "us. Now dithering...");
  
  
  display.clearDisplay();
  display.display();

  while(1){

    /* The following timings have been recorded for an 80x32 image using an ATMega2560 uC running @ Fc = 16MHz.
     *  
        Image dithered using algorithm # 0. Operation took 52044us.
        Image dithered using algorithm # 1. Operation took 266368us.
        Image dithered using algorithm # 2. Operation took 1024804us.
        Image dithered using algorithm # 3. Operation took 1024560us.
        Image dithered using algorithm # 4. Operation took 428068us.
        Image dithered using algorithm # 5. Operation took 578140us.
        Image dithered using algorithm # 6. Operation took 422832us.
        Image dithered using algorithm # 7. Operation took 208164us.
        Image dithered using algorithm # 8. Operation took 400244us.
        Image dithered using algorithm # 9. Operation took 3432us.
        Image dithered using algorithm # 10. Operation took 3432us.
        Image dithered using algorithm # 11. Operation took 3440us.
        Image dithered using algorithm # 12. Operation took 12540us.
        Image dithered using algorithm # 13. Operation took 4424us.
     */
    
    static uint8_t cnt = 0;
    loadImageBuffer();

    uint32_t t = micros();
    switch(cnt){
      case 0: image.randomDither(img_mod, false);  break;
      case 1: image.FSDither(img_mod);  break;
      case 2: image.JJNDither(img_mod);  break;
      case 3: image.StuckiDither(img_mod);  break;
      case 4: image.BurkesDither(img_mod);  break;
      case 5: image.Sierra3Dither(img_mod);  break;
      case 6: image.Sierra2Dither(img_mod);  break;
      case 7: image.Sierra24ADither(img_mod);  break;
      case 8: image.AtkinsonDither(img_mod);  break;
      case 9: image.thresholding(img_mod, 180);  break;
      case 10: image.thresholding(img_mod, 64);  break;
      case 11: image.thresholding(img_mod);  break;
      case 12: image.fastEDDither(img_mod);  break;
      case 13:{
        image.buildClusteredPattern();
        image.patternDither(img_mod);
      }  break;
      case 14:{
        image.buildBayerPattern();
        image.patternDither(img_mod);
      }  break;
    }
    
    t = micros() - t;
    Serial.println("Image dithered using algorithm # " + String(cnt) + ". Operation took " + String(t) + "us.");
    
    cnt = (cnt + 1) % 14;

    
    display.clearDisplay();
    showImg(img_mod);
    display.display();
    
    delay(2000);    // 2 seconds between each dithering algorithm
  }
  
}


void loop() {
  
}


void showImg(uint8_t *img){

  for(uint16_t y = 0; y < img_height; y++){
    for(uint16_t x = 0; x < img_width; x++){

      uint8_t color = img[image.index(x, y)];

      uint8_t pixel = image.colorGray256ToBool(color);

      display.drawPixel(x, y, pixel);
    }
  }
}


void initDisplay(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with I2C addr 0x3C
  //display.dim(1); // dim the display
  display.clearDisplay();
  display.display();
}


  //  Load image data into RAM buffer
void loadImageBuffer(){
  for(uint32_t i = 0; i < (img_height*img_width); i++){
    #ifdef USING_TEENSY
    img_mod[i] = IMAGE[i];
    #else
    img_mod[i] = pgm_read_byte_near(IMAGE + i);
    #endif
  }
}
