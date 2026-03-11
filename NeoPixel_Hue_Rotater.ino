/*
   NeoPixel Hue Rotater
   by Max Parisi (@tinkerer9)

   Credit to Adrianotiger for the NeoPixel code (generated from https://adrianotiger.github.io/Neopixel-Effect-Generator/) and everyone on Arduino Fourm for helping with problems along the way.

   GitHub repository on https://github.com/tinkerer9/NeoPixel_Hue_Rotater.
*/

// INCLUDE LIBRARIES
#include <Adafruit_NeoPixel.h> // for NeoPixels
#include <dht.h> // for DHT

// FOR NEOPIXEL RAINBOW
class Strip
{
  public:
    uint8_t   effect;
    uint8_t   effects;
    uint16_t  effStep;
    unsigned long effStart;
    Adafruit_NeoPixel strip;
    Strip(uint16_t leds, uint8_t pin, uint8_t toteffects, uint16_t striptype) : strip(leds, pin, striptype) {
      effect = -1;
      effects = toteffects;
      Reset();
    }
    void Reset() {
      effStep = 0;
      effect = (effect + 1) % effects;
      effStart = millis();
    }
};
struct Loop
{
  uint8_t currentChild;
  uint8_t childs;
  bool timeBased;
  uint16_t cycles;
  uint16_t currentTime;
  Loop(uint8_t totchilds, bool timebased, uint16_t tottime) {
    currentTime = 0;
    currentChild = 0;
    childs = totchilds;
    timeBased = timebased;
    cycles = tottime;
  }
};

// DEFINE VARIABLES
const int neoLeds = 50; // LEDs on Neopixel strip 1
const int neoPin = 8; // pin of Neopixel strip 1
const int neo2Leds = 5; // LEDs on Neopixel strip 2 (generally number of modes)
const int neo2Pin = 7; // pin of Neopixel strip 2
const int btnPin = 2; // pin of button
const int brightnessPin = 0; // pin of brightness pot
const int pot2Pin = 1; // pin of LED brightness/hue pot
const int ledPin = 5; // pin of LED MOSFET
const int buzzPin = 3; // pin of buzzer
const int dhtPin = 4; // pin of DHT
const int sLedPin = LED_BUILTIN; // pin of switch LED transistor (LED_BUILTIN is pin # of built in LED)
const int hueSpeed = 2.8; // speed of mode 3

int brightnessState, pot2State, brightness, neoSpeed, pixelNum, chk, prePixelNum, hue, i, inputI;
int mode = 1;
byte rgb[3];
float temp, rh, preTemp, preRh, inputF;
unsigned long millisOffset;
boolean btnState;

// SET UP COMPONENTS
Strip NeoPixel(neoLeds, neoPin, neoLeds, NEO_GRB + NEO_KHZ800);
struct Loop strip0loop0(1, false, 1);
Strip NeoPixel2(neo2Leds, neo2Pin, neo2Leds, NEO_GRB + NEO_KHZ800);
dht DHT;

// SETUP CODE
void setup() {
  // SET PINMODES
  pinMode(btnPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(sLedPin, OUTPUT);

  digitalWrite(sLedPin, HIGH); // turn on switch LED

  Serial.begin(9600); // begin Serial port at 9600 baud rate
  Serial.println("Power On");

  buzz(500);

  // BEGIN NEOPIXEL STRIPS
  NeoPixel.strip.begin();
  NeoPixel2.strip.begin();

  // TURN OFF NEOPIXEL STRIPS AND LED
  strip_off();
  strip2_off();
  led(0);

  // RUN TEST IF BUTTON HELD DOWN DURING STARTUP
  btnState = !digitalRead(btnPin);
  if (btnState) {
    delay(200);
    test();
  } else {
    delay(1000);
  }

  pixel2_up_to(1, 0, 0, 255); // set NeoPixel strip 2 to correct mode #

  digitalWrite(sLedPin, LOW); // turn off switch LED
}

// LOOP CODE
void loop() {
  // SET DHT VARIABLES
  chk = DHT.read11(dhtPin);
  temp = DHT.temperature;
  rh = DHT.humidity; // rh = humidity

  // SET BTN/POT VARIABLES
  brightnessState = pot_to_value(analogRead(brightnessPin));
  pot2State = pot_to_value(analogRead(pot2Pin));
  btnState = !digitalRead(btnPin);

  // CODE TO SWITCH MODE
  if (btnState) { // if button pressed
    while (btnState) { // wait until released
      btnState = !digitalRead(btnPin);
    }
    mode++; // increment mode
    while (mode > 5) { // if mode is greater than 6
      mode -= 5; // subtract 5
    }

    // UPDATE STRIP 2
    strip2_off();
    pixel2_up_to(mode, 0, 0, 255);

    // UPDATES FOR CERTAIN MODES
    if (mode == 3) {
      hue = 0;
    }
    if (mode == 5) {
      preTemp = temp + 1;
      preRh = rh + 1;
    }

    buzz(500); // buzz
  }

  // MODE SWITCH
  switch (mode) {
    case 1: // MODE 1
      all(255, 255, 255); // NeoPixel strip 1 all white
      led(value_to_led(pot2State)); // LED at value of LED/hue pot
      break;
    case 2: // MODE 2
      led(0); // LED off
      hue_to_rgb(value_to_hue(pot2State), rgb); // convert LED/hue to hue value to rgb value
      all(rgb[0], rgb[1], rgb[2]); // Set strip 1 all at rgb value
      break;
    case 3: // MODE 3
      while (hue > 359) { // make sure hue ranges from 0-359, not going above 360
        hue -= 360;
      }

      led(value_to_led(pot2State)); // LED at value of LED/hue pot
      hue_to_rgb(hue, rgb); // convert hue value to rgb value
      all(rgb[0], rgb[1], rgb[2]); // Set strip 1 all at rgb value

      hue += hueSpeed; // increment hue by hueSpeed
      break;
    case 4: // MODE 4
      strips_loop(); // run rainbow code
      led(value_to_led(pot2State)); // LED at value of LED/hue pot
      break;
    case 5: // MODE 5
      led(value_to_led(pot2State)); // LED at value of LED/hue pot
      if (!(temp == preTemp) || !(rh == preRh)) { // if temp or humidity changed
        all(5, 5, 0); // turn strip 1 all at dim yellow (background)
        pixel(temp_to_pixel(temp), 255, 0, 0); // set corresponding light red for temp
        pixel(rh_to_pixel(rh), 0, 0, 255); // set corresponding light blue for humidity

        // UPDATE PRETEMP AND PRERH
        preTemp = temp;
        preRh = rh;
      }
      break;
    default:
      error("The variable 'mode' went over maximum value."); // if mode > 5 than show error message
      break;
  }

  // SET NEOPIXEL BRIGHTNESS AND UPDATE NEOPIXELS
  brightness = value_to_brightness(brightnessState);
  NeoPixel.strip.setBrightness(brightness);
  NeoPixel.strip.show();
  if (brightness <= 0) { // make sure strip 1 is always on
    NeoPixel2.strip.setBrightness(1);
  } else {
    NeoPixel2.strip.setBrightness(brightness);
  }
  NeoPixel2.strip.show();
}

// FOR NEOPIXEL RAINBOW
void strips_loop() {
  if (strip0_loop0() & 0x01)
    NeoPixel.strip.show();
}
uint8_t strip0_loop0() {
  uint8_t ret = 0x00;
  switch (strip0loop0.currentChild) {
    case 0:
      ret = strip0_loop0_eff0(); break;
  }
  if (ret & 0x02) {
    ret &= 0xfd;
    if (strip0loop0.currentChild + 1 >= strip0loop0.childs) {
      strip0loop0.currentChild = 0;
      if (++strip0loop0.currentTime >= strip0loop0.cycles) {
        strip0loop0.currentTime = 0;
        ret |= 0x02;
      }
    }
    else {
      strip0loop0.currentChild++;
    }
  };
  return ret;
}
uint8_t strip0_loop0_eff0() {
  if (millis() - NeoPixel.effStart < neoLeds * (NeoPixel.effStep)) return 0x00;
  float factor1, factor2;
  uint16_t ind;
  for (uint16_t j = 0; j < 52; j++) {
    ind = NeoPixel.effStep + j * 1;
    switch ((int)((ind % 52) / 17.333333333333332)) {
      case 0: factor1 = 1.0 - ((float)(ind % 52 - 0 * 17.333333333333332) / 17.333333333333332);
        factor2 = (float)((int)(ind - 0) % 52) / 17.333333333333332;
        NeoPixel.strip.setPixelColor(j, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2);
        break;
      case 1: factor1 = 1.0 - ((float)(ind % 52 - 1 * 17.333333333333332) / 17.333333333333332);
        factor2 = (float)((int)(ind - 17.333333333333332) % 52) / 17.333333333333332;
        NeoPixel.strip.setPixelColor(j, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2);
        break;
      case 2: factor1 = 1.0 - ((float)(ind % 52 - 2 * 17.333333333333332) / 17.333333333333332);
        factor2 = (float)((int)(ind - 34.666666666666664) % 52) / 17.333333333333332;
        NeoPixel.strip.setPixelColor(j, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2);
        break;
    }
  }
  if (NeoPixel.effStep >= 52) {
    NeoPixel.Reset();
    return 0x03;
  }
  else NeoPixel.effStep++;
  return 0x01;
}

// MAP VALUE TO BRIGHTNESS
int value_to_brightness(int value) {
  if (value >= 1000) { // make full and 0 brightness "sticky"
    return 255;
  } else if (value <= 24) {
    return 0;
  } else {
    int brightness = map(value, 0, 1024, 0, 100);

    if (brightness < 0) {
      error("In 'value_to_brightness()', 'brightness' is less than 0.");
    }
    if (brightness > 255) {
      error("In 'value_to_brightness()', 'brightness' is greater than 255.");
    }

    return brightness;
  }
}

// MAP POT VALUE TO ARTIFICIAL VALUE
// my pot value ranges from 200 to 800, not 0 to 1024
// change to your pot values
int pot_to_value(int pot) {
  int value = map(pot, 200, 800, 0, 1024); // change 200 and 800 to your pot max. and min. values

  if (value < 0) { // make sure value is in range
    value = 0;
  } else if (value > 1024) {
    value = 1024;
  }

  return value;
}

// MAP VALUE TO LED BRIGHTNESS
int value_to_led(int value) {
  return map(value, 0, 1024, 0, 100);
}

// MAP VALUE TO HUE
int value_to_hue(int value) {
  return map(value, 0, 1024, 0, 360);
}

// TURN HUE TO RGB VALUE
int hue_to_rgb(float H, byte rgb[]) {
  float C = 1;
  float X = 1 - abs(fmod(H / 60.0, 2) - 1);
  float m = 0;
  float r, g, b;
  if (H >= 0 && H < 60) {
    r = C, g = X, b = 0;
  } else if (H >= 60 && H < 120) {
    r = X, g = C, b = 0;
  } else if (H >= 120 && H < 180) {
    r = 0, g = C, b = X;
  } else if (H >= 180 && H < 240) {
    r = 0, g = X, b = C;
  } else if (H >= 240 && H < 300) {
    r = X, g = 0, b = C;
  } else {
    r = C, g = 0, b = X;
  }

  r = r * 255;
  g = g * 255;
  b = b * 255;

  if (r < 0) {
    error("In 'value_to_rgb()', 'r' is less than 0.");
  }
  if (r > 255) {
    error("In 'value_to_rgb()', 'r' is greater than 255.");
  }
  if (g < 0) {
    error("In 'value_to_rgb()', 'r' is less than 0.");
  }
  if (g > 255) {
    error("In 'value_to_rgb()', 'r' is greater than 255.");
  }
  if (b < 0) {
    error("In 'value_to_rgb()', 'r' is less than 0.");
  }
  if (b > 255) {
    error("In 'value_to_rgb()', 'r' is greater than 255.");
  }

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

// MAP TEMP TO PIXEL #
int temp_to_pixel(int temp) {
  int pixel = map(temp, 15, 40, 0, (neoLeds - 1));

  if (pixel < 0) { // make sure value is in range
    pixel = 0;
  } else if (pixel > (neoLeds - 1)) {
    pixel = neoLeds - 1;
  }

  return pixel;
}

// MAP HUMIDITY (RH) TO PIXEL #
int rh_to_pixel(int rh) {
  int pixel = map(rh, 30, 90, 0, (neoLeds - 1));

  if (pixel < 0) { // make sure value is in range
    pixel = 0;
  } else if (pixel > (neoLeds - 1)) {
    pixel = neoLeds - 1;
  }

  return pixel;
}

// SET PIXEL CODE FOR STRIP 1
int pixel(int pixel, int r, int g, int b) {
  if (pixel < 0) { // detect error
    error("On function 'pixel()' first parameter (the pixel #) was above 0.");
  } else if (pixel > (neoLeds - 1)) {
    error("On function 'pixel()' first parameter (the pixel #) was above LEDs on the NeoPixel Strip (minus 1 because of zero indexing).");
  }

  NeoPixel.strip.setPixelColor(pixel, NeoPixel.strip.Color(r, g, b));
  NeoPixel.strip.show();
}

// SET PIXEL CODE FOR STRIP 2
int pixel2(int pixel, int r, int g, int b) {
  if (pixel < 0) { // detect error
    error("On function 'pixel2()' first parameter (the pixel #) was above 0.");
  } else if (pixel > (neo2Leds - 1)) {
    error("On function 'pixel2()' first parameter (the pixel #) was above LEDs on the NeoPixel Strip #2 (minus 1 because of zero indexing).");
  }

  NeoPixel2.strip.setPixelColor(pixel, NeoPixel2.strip.Color(r, g, b));
  NeoPixel2.strip.show();
}

// SET PIXEL UP TO CODE FOR STRIP 1
int pixel_up_to(int pixels, int r, int g, int b) {
  if (pixel < 0) { // detect error
    error("On function 'pixel_up_to()' first parameter (the pixel #) was above 0.");
  } else if (pixel > (neoLeds - 1)) {
    error("On function 'pixel_up_to()' first parameter (the pixel #) was above LEDs on the NeoPixel Strip #2 (minus 1 because of zero indexing).");
  }

  for (i = 0; i < pixels; i++) {
    NeoPixel.strip.setPixelColor(i, NeoPixel.strip.Color(r, g, b));
  }
  NeoPixel.strip.show();
}

// SET PIXEL UP TO CODE FOR STRIP 2
int pixel2_up_to(int pixels, int r, int g, int b) {
  for (i = 0; i < pixels; i++) {
    NeoPixel2.strip.setPixelColor(i, NeoPixel2.strip.Color(r, g, b));
  }
  NeoPixel2.strip.show();
}

// BUZZ CODE AT 100 HZ
int buzz(int duration) {
  if (duration < 0) {
    error("On function 'buzz()', 'duration' is less than 0..");
  }
  tone(buzzPin, 100, duration);
}

// ERROR CODE
int error(String info) {
  // SET STRIP AT MAX. BRIGHTNESS
  NeoPixel.strip.setBrightness(255);
  NeoPixel2.strip.setBrightness(255);

  led(0); // turn LED off

  // PRINT ERROR MESSAGE
  Serial.print("ERROR: ");
  Serial.println(info);

  while (!btnState) { // run until button pressed
    btnState = !digitalRead(btnPin); // update btnState
    buzz(1000); // buzz for 1 second

    // ALL STRIPS RED
    all(255, 0, 0);
    all2(255, 0, 0);

    digitalWrite(sLedPin, HIGH); // switch LED on
    delay(1000); // wait 1 second

    // ALL STRIPS OFF
    strip_off();
    strip2_off();

    digitalWrite(sLedPin, LOW); // switch LED off
    delay(1000); // wait 1 second
  }
  Serial.println("Error Ignored"); // when button pressed, continue
}

// STRIP 1 OFF CODE
void strip_off() {
  all(0, 0, 0);
  NeoPixel.strip.show();
}

// SET ALL PIXELS CODE FOR STRIP 1
int all(int r, int g, int b) {
  for (i = 0; i < neoLeds; i++) {
    NeoPixel.strip.setPixelColor(i, NeoPixel.strip.Color(r, g, b));
  }
  NeoPixel.strip.show();
}

// STRIP 2 OFF CODE
void strip2_off() {
  all2(0, 0, 0);
  NeoPixel2.strip.show();
}

// SET ALL PIXELS CODE FOR STRIP 2
int all2(int r, int g, int b) {
  for (i = 0; i < neo2Leds; i++) {
    NeoPixel2.strip.setPixelColor(i, NeoPixel2.strip.Color(r, g, b));
  }
  NeoPixel2.strip.show();
}

// SET LED BRIGHTNESS CODE
int led(int dutyCycle) {
  analogWrite(ledPin, map(dutyCycle, 0, 100, 0, 255));
}

// STRIP TEST CODE
void test() {
  Serial.println("~~~ Light Test ~~~");

  // SET BOTH STRIPS TO FULL BRIGHTNESS
  NeoPixel.strip.setBrightness(255);
  NeoPixel2.strip.setBrightness(255);

  buzz(500); // buzz

  Serial.println("  Red");
  for (i = 0; i < neoLeds; i++) { // "swipe" up red
    pixel(i, 255, 0, 0);
    delay(500 / neoLeds);
  }
  delay(250);
  buzz(500); // buzz

  Serial.println("  Green");
  for (i = 0; i < neoLeds; i++) { // "swipe" up green
    pixel(i, 0, 255, 0);
    delay(500 / neoLeds);
  }
  delay(250);
  buzz(500); // buzz
  Serial.println("  Blue");
  for (i = 0; i < neoLeds; i++) { // "swipe" up blue
    pixel(i, 0, 0, 255);
    delay(500 / neoLeds);
  }
  delay(250);
  buzz(500);
  Serial.println("  Yellow");
  for (i = 0; i < neoLeds; i++) { // "swipe" up yellow
    pixel(i, 255, 255, 0);
    delay(500 / neoLeds);
  }
  delay(250);
  buzz(500); // buzz
  Serial.println("  White w/ LED");
  for (i = 0; (i < neoLeds); i++) { // "swipe" up white and change brightness of LED accordingly
    pixel(i, 255, 255, 255);
    led(map(i, 0, neoLeds, 0, 100));
    delay(500 / neoLeds);
  }
  led(100); // make LED fully on
  delay(1000);

  // SET BOTH STRIPS OFF AND TURN LED OFF
  strip_off();
  strip2_off();
  led(0);

  buzz(500); // buzz
  delay(1000);
  Serial.println("Done with test");
}
