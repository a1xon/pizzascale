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

int weight = 0; // scroll this text from right to left
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

int measurmentStep = -1;

void setup(void)
{
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFontMode(0); // enable transparent mode, which is faster

  pinMode(SWITCH, INPUT_PULLUP);
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(219);
  if (myTimer.isTimeReached())
  { //check if execution time has been reached
    loadcell.tare();
  }
  /// loadcell.set_offset();
}

void loop(void)
{
  u8g2_uint_t x;

  int btnState = digitalRead(SWITCH);

  if (btnState == LOW && millis() - lastButtonPress > 1000)
  {
    if (measurmentStep == -1)
    {
      cupWeight = getWeight();
    }
    else
    {
      pizzaMeasures[measurmentStep++] = getWeight();
      gut = pizzaIngString[measurmentStep];
    }
    lastButtonPress = millis();
  }
  ////
  calculatePizza()

  u8g2.firstPage();
  do
  {
    drawScreen(u8g2);
  } while (u8g2.nextPage());
  // start over again
}

int calculatePizza()
{
  float wheat = measurmentStep == 0 ? (float)getWeight() : (float)pizzaMeasures[0];
  float salt = pizzaMeasures[1] != 0 ? (float)pizzaMeasures[1] : (wheat * pizzaIngRelative[1]);
  float water = pizzaMeasures[2] != 0 ? (float)pizzaMeasures[2] : (wheat * pizzaIngRelative[2]);
  dough = (int)round(wheat + salt + water);

  if (dough != 0)
  {
    for (int amountPizza = 1; amountPizza < 20; amountPizza++)
    {
      if ((dough / amountPizza) >= 250 && (dough / amountPizza) <= 325)
      {
        pizzaBlankGramm = (int)round(dough / amountPizza);
        amountPizzasTotal = amountPizza;
        return 0;
      }
    }
  }

  return 0;
}

int getWeight()
{
  float weightFloat = loadcell.get_units(5);

  int weight = (int)round(weightFloat) - cupWeight;
  for (int i; i < 3; i++)
  {
    weight += pizzaMeasures[i];
  }

  if (weight < 0)
  {
    weight = 0;
  }

  return weight;
}

void progressbar(U8G2 u8g2, int x, int y, int w, int h, float value)
{
  u8g2.drawFrame(x, y, w, h);
  u8g2.drawBox(x + 2, y + 2, (w - 4) * value, h - 3);
}

void drawScreen(U8G2 u8g2)
{
  u8g2.setFont(u8g2_font_profont29_mr); // set the target font
  //u8g2.drawUTF8(0, 22, getWeight(u8g2));     // draw the scolling text
  u8g2.setCursor(0, 22);
  u8g2.print(u8x8_u16toa(getWeight(), 4));
  u8g2.setFont(u8g2_font_profont12_mr); // draw the current pixel width
  u8g2.drawUTF8(0, 32, gut);
  u8g2.setFont(u8g2_font_profont10_tf);

  u8g2.setCursor(82, 14);
  u8g2.print(amountPizzasTotal);
  u8g2.drawUTF8(90, 14, "Pizzen");
  u8g2.drawUTF8(82, 22, "á");
  u8g2.setCursor(88, 22);
  u8g2.print(pizzaBlankGramm);

  progressbar(u8g2, 40, 27, 85, 5, progress);
}
