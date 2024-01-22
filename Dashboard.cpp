#include "TFT_eSPI.h"
#include <stdlib.h>
#include "Dashboard.h"

unsigned int compassX;
unsigned int compassY;

unsigned int dirColor;
unsigned int circleColor;
unsigned int tempColor;

/*
A static variable is a special kind of variable; it is allocated memory 'statically'.
Its lifetime is the entire run of the program. It is specific to a function, i.e., only the function that defined it can access it.
However, it doesn't get destroyed after the function call ends.
It preserves its value between successive function calls. It is created and initialized the first time a function is called.
In the next function call, it is not created again. It just exists.
*/

//calculates x y points for compass letters and dots
coord calcCompassPoints(int x, int y, int diameter) {
  int radius = diameter / 2;
  coord letterLoc;

  letterLoc.xW = x - radius;
  letterLoc.yW = y;

  letterLoc.xN = x;
  letterLoc.yN = y - radius;

  letterLoc.xE = x + radius;
  letterLoc.yE = y;

  letterLoc.xS = x;
  letterLoc.yS = y + radius;

  //dots will be placed at 45 deg (sin(45) ~0.707) in relation to x axis
  //so same distance can be used from x and y axis

  int dotsDistance = radius * 0.707 + 0.5;
  letterLoc.xNW = x - dotsDistance;
  letterLoc.yNW = y - dotsDistance;

  letterLoc.xNE = x + dotsDistance;
  letterLoc.yNE = y - dotsDistance;

  letterLoc.xSW = x - dotsDistance;
  letterLoc.ySW = y + dotsDistance;

  letterLoc.xSE = x + dotsDistance;
  letterLoc.ySE = y + dotsDistance;

  return letterLoc;
}

//calculate triangle coordinates with center of triangle given
triangleCoord calcTriangleCoord(int x0, int y0) {
  triangleCoord coord;
  coord.x1 = x0;
  //coord.y1 = y0 + arrowHeight / 6;
  coord.y1 = y0;

  coord.x2 = x0 - arrowBaseWidth / 2;
  coord.y2 = y0 - arrowHeight / 2;

  coord.x3 = x0 + arrowBaseWidth / 2;
  coord.y3 = y0 - arrowHeight / 2;

  return coord;
}

void showDash(TFT_eSprite *sprite, float temp, float speed, char *date) {

  circleColor = TFT_SKYBLUE;
  tempColor = TFT_WHITE;

  static float T = 0;
  static float S = 0;

  T = temp;
  S = speed;

  if (S == -1) { circleColor = TFT_DARKGREY; }
  if (T == -99) { tempColor = TFT_DARKGREY; }

  //will put comapss letters in top left corner and add 7 points not to cut letters
  compassX = compassD / 2 + 7;
  compassY = compassD / 2 + 7 + 20;

  //---------------------
  //Show temperature gadget
  //---------------------
  sprite->createSprite(SCREEN_W, SCREEN_H);
  sprite->fillSprite(TFT_BLACK);
  //sprite->setFreeFont(&FreeSansBold56pt7b);
  sprite->setTextFont(FONT8);
  sprite->setTextColor(tempColor);
  int textH = sprite->fontHeight();
  sprite->setTextDatum(CC_DATUM);
  int textW = sprite->drawFloat(T, 1, compassD + (SCREEN_W - compassD) / 2, SCREEN_H / 2);

  //---------------------
  //Show windspeed gadget
  //---------------------
  sprite->setTextFont(FONT6);
  sprite->setTextColor(circleColor);
  sprite->setTextDatum(TC_DATUM);
  sprite->drawFloat(abs(S), 1, compassX, compassY + compassD / 2 + 20);

  //
  //Show refresh time gadget
  //
  sprite->setTextFont(FONT2);
  sprite->setTextDatum(BC_DATUM);
  sprite->drawString(date, compassD + (SCREEN_W - compassD) / 2, SCREEN_H);
}

//---------------------
//Show arrow gadget
//---------------------
void showDirection(TFT_eSprite *arrowSprite, TFT_eSprite *dashSprite, int D) {
  dirColor = TFT_ORANGE;
  circleColor = TFT_SKYBLUE;

  if (D == -1){
    dirColor = TFT_DARKGREY;
    circleColor = TFT_DARKGREY;
  }

  //---------------------
  //Show compass circle gadget
  //---------------------
  dashSprite->setTextFont(FONT2);
  dashSprite->setTextColor(circleColor);
  coord letterCoord = calcCompassPoints(compassX, compassY, compassD);
  dashSprite->setTextDatum(CC_DATUM);

  dashSprite->drawString("N", letterCoord.xN, letterCoord.yN);
  dashSprite->drawString("S", letterCoord.xS, letterCoord.yS);
  dashSprite->drawString("W", letterCoord.xW, letterCoord.yW);
  dashSprite->drawString("E", letterCoord.xE, letterCoord.yE);

  dashSprite->fillCircle(letterCoord.xNW, letterCoord.yNW, 2, circleColor);
  dashSprite->fillCircle(letterCoord.xNE, letterCoord.yNE, 2, circleColor);
  dashSprite->fillCircle(letterCoord.xSW, letterCoord.ySW, 2, circleColor);
  dashSprite->fillCircle(letterCoord.xSE, letterCoord.ySE, 2, circleColor);

  arrowSprite->createSprite(arrowBaseWidth, arrowHeight);
  arrowSprite->fillSprite(TFT_BLACK);
  triangleCoord arrow = calcTriangleCoord(arrowBaseWidth / 2, arrowHeight / 2);
  arrowSprite->fillTriangle(arrow.x1, arrow.y1, arrow.x2, arrow.y2, arrow.x3, arrow.y3, dirColor);
  dashSprite->setPivot(compassX, compassY);
  arrowSprite->pushRotated(dashSprite, D, TFT_BLACK);
}