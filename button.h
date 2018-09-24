#ifndef POINT_H
#define POINT_H

#include "point.h"
#include "SDL/SDL.h"
#include "SDL_basic.h"

class button
{
  public:
    point pos;
    SDL_Surface* image;
    int length, height;
    button () {};
    button (point, string, int, int);
    bool pressed(point);
};

#endif
