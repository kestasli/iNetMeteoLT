#include "TFT_eSPI.h"
#include <stdlib.h>
#include "Dashboard.h"

unsigned int compassX;
unsigned int compassY;

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

void showDash(TFT_eSprite *sprite, float T, float S) {
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
  sprite->setTextColor(TFT_WHITE);
  int textH = sprite->fontHeight();
  sprite->setTextDatum(CC_DATUM);
  int textW = sprite->drawFloat(T, 1, compassD + (SCREEN_W - compassD) / 2, SCREEN_H / 2);

  //---------------------
  //Show compass directions gadget
  //---------------------
  sprite->setTextFont(FONT2);
  sprite->setTextColor(TFT_SKYBLUE);
  coord letterCoord = calcCompassPoints(compassX, compassY, compassD);
  sprite->setTextDatum(CC_DATUM);

  sprite->drawString("N", letterCoord.xN, letterCoord.yN);
  sprite->drawString("S", letterCoord.xS, letterCoord.yS);
  sprite->drawString("W", letterCoord.xW, letterCoord.yW);
  sprite->drawString("E", letterCoord.xE, letterCoord.yE);

  sprite->fillCircle(letterCoord.xNW, letterCoord.yNW, 2, TFT_SKYBLUE);
  sprite->fillCircle(letterCoord.xNE, letterCoord.yNE, 2, TFT_SKYBLUE);
  sprite->fillCircle(letterCoord.xSW, letterCoord.ySW, 2, TFT_SKYBLUE);
  sprite->fillCircle(letterCoord.xSE, letterCoord.ySE, 2, TFT_SKYBLUE);

  //---------------------
  //Show windspeed gadget
  //---------------------
  sprite->setTextFont(FONT6);
  sprite->setTextColor(TFT_SKYBLUE);
  sprite->setTextDatum(TC_DATUM);
  sprite->drawFloat(abs(S), 1, compassX, compassY + compassD / 2 + 20);
}

//---------------------
//Show arrow gadget
//---------------------
void showDirection(TFT_eSprite *arrowSprite, TFT_eSprite *dashSprite, int D) {
  arrowSprite->createSprite(arrowBaseWidth, arrowHeight);
  arrowSprite->fillSprite(TFT_BLACK);
  triangleCoord arrow = calcTriangleCoord(arrowBaseWidth / 2, arrowHeight / 2);
  arrowSprite->fillTriangle(arrow.x1, arrow.y1, arrow.x2, arrow.y2, arrow.x3, arrow.y3, TFT_GREEN);
  dashSprite->setPivot(compassX, compassY);
  arrowSprite->pushRotated(dashSprite, D, TFT_BLACK);
}