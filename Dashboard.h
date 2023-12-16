#include <sys/_types.h>
#include <sys/_intsup.h>
#include <cmath>
#ifndef NUMBER_DISPLAY
#define NUMBER_DISPLAY

#define SCREEN_W 320
#define SCREEN_H 172

#include <TFT_eSPI.h>  // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h"

const unsigned int arrowHeight = 50;
const unsigned int arrowBaseWidth = 17;
const unsigned int compassD = 80;

struct coord {
  int xW, yW, xN, yN, xE, yE, xS, yS;
  int xNE, yNE, xNW, yNW, xSW, ySW, xSE, ySE;
};

struct triangleCoord{
  int x1, y1, x2, y2, x3, y3;
};

coord calcCompassPoints(int x, int y, int diameter);
triangleCoord calcTriangleCoord(int x0, int y0);

void showDash(TFT_eSprite *sprite, float T, float S);
void showDirection(TFT_eSprite *arrowSprite, TFT_eSprite * dashSprite, int D);


#endif