#include <dmtimer.h>
#include "HX711.h"
#include <U8g2lib.h>

//U8g2 Contructor
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/16, /* clock=*/5, /* data=*/4);

DMTimer myTimer(100000);

HX711 loadcell;
// Hardware Setup

const int LOADCELL_DOUT_PIN = 13;
const int LOADCELL_SCK_PIN = 12;
const int SWITCH = 3;

unsigned long lastButtonPress = 0;

const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

int weight = 0;
int aimWeight = 0;
int cupWeight = 0;
const char *gut = "Container";
float progress = 0;

char *pizzaIngString[] = {"Mehl", "Salz", "Wasser"};
float pizzaIngRelative[] = {0, 0.02, 0.63};
int pizzaMeasures[] = {0, 0, 0};
int dough = 0;
int pizzaBlank = 300;

/// how many, how heavy?
int amountPizzasTotal = 0;
int pizzaBlankGramm = 0;
int currentWeight = 0;
int measurmentStep = -1;

void setup(void) {
  Serial.begin(115200);
  u8g2.begin();
  // enable transparent mode, which is faster
  u8g2.setFontMode(0); 

  pinMode(SWITCH, INPUT_PULLUP);
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(219);
  /// wait for timer to tare scale
  if (myTimer.isTimeReached()) {
    loadcell.tare();
  }
}

void loop(void) {
  u8g2_uint_t x;

  currentWeight = getWeight();
  /// get progress for weight

  int btnState = digitalRead(SWITCH);

  /// debounced click
  if (btnState == LOW && millis() - lastButtonPress > 500) {
    if (measurmentStep == -1) {
      cupWeight = getWeight();
    } else {
      /// calcutalte pizza after each weighing step
      pizzaMeasures[measurmentStep] = currentWeight;
      calculatePizza();
    }
    measurmentStep++;
    gut = pizzaIngString[measurmentStep];
    lastButtonPress = millis();
  }
  
  /// calculate pizza live when measuring flour
  if (measurmentStep == 0) {
    calculatePizza();
  } else if (measurmentStep >= 1) {
    /// calcutlate progress when measuring salt and water
    progress = currentWeight / (pizzaMeasures[0] * pizzaIngRelative[measurmentStep]);
  }

  u8g2.firstPage();
  do {
    drawScreen(u8g2);
  } while (u8g2.nextPage());
  // start over again
}

int calculatePizza() {
  float wheat = measurmentStep == 0 ? (float)currentWeight : (float)pizzaMeasures[0];
  float salt = pizzaMeasures[1] != 0 ? (float)pizzaMeasures[1] : (wheat * pizzaIngRelative[1]);
  float water = pizzaMeasures[2] != 0 ? (float)pizzaMeasures[2] : (wheat * pizzaIngRelative[2]);
  dough = (int)round(wheat + salt + water);

  if (dough != 0) {
    for (int amountPizza = 1; amountPizza < 20; amountPizza++) {
      if ((dough / amountPizza) >= 250 && (dough / amountPizza) <= 325) {
        pizzaBlankGramm = (int)round(dough / amountPizza);
        amountPizzasTotal = amountPizza;
        return amountPizza;
      }
    }
  }

  return 0;
}

int getWeight()
{
  float weightFloat = loadcell.get_units(5);
  /// substract cup container from weight
  int weight = (int)round(weightFloat) - cupWeight;
  
  for (int i = 0; i < 3; i++) {
    weight -= pizzaMeasures[i];
  }

  if (weight < 0) {
    weight = 0;
  }

  return weight;
}

void progressbar(U8G2 u8g2, int x, int y, int w, int h, float value)
{
  u8g2.drawFrame(x, y, w, h);
  u8g2.drawBox(x + 2, y + 2, (w - 4) * value, h - 3);
}

void drawScreen(U8G2 u8g2) {
  u8g2.setFont(u8g2_font_profont29_mr); // set the target font
  u8g2.setCursor(0, 22);
  if (measurmentStep <= 0) {
      u8g2.print(u8x8_u16toa(currentWeight, 4));
  } else {
      u8g2.print(u8x8_u16toa( (pizzaMeasures[0] * pizzaIngRelative[measurmentStep]) - currentWeight, 4));
  }

  
  u8g2.setFont(u8g2_font_profont12_mr);
  u8g2.drawUTF8(65, 20, "g");
  
  /// gut
  u8g2.drawUTF8(0, 32, gut);

  if (measurmentStep >= 0) {
    u8g2.setFont(u8g2_font_profont10_tf);
    u8g2.setCursor(82, 14);
    u8g2.print(amountPizzasTotal);
    u8g2.drawUTF8(90, 14, "Pizzen");
    u8g2.drawUTF8(82, 22, "á");
    u8g2.setCursor(88, 22);
    u8g2.print(pizzaBlankGramm);
  }

  if (measurmentStep >= 1) {
    progressbar(u8g2, 40, 27, 85, 5, progress);
  }
}
