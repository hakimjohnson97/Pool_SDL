#ifndef UNIT_H
#define UNIT_H

#include "point.h"
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_mixer.h"
#include "SDL_basic.h"
#include <string>
#include <sstream>
#include <cmath>
#include "SDL_rotozoomh.h"
#include "button.h"
#include "unit.h"

class unit
{
  public:
    //Data
    point pos, prepos, vel, force; //Prepos is the position the unit was last applied onto the screen
    SDL_Surface* image;
    int collis, Id;
    string type;
    static unit* ball[]; //Pointer to ALL units created so static function update can access all of them
    static int n;        //The count of how many units have been creted
    //Functions
    static void update();  //Updates ALL units by moving them and checking for collision
    bool goalscored();     //This is an extention of update because it was pretty long, checks if goal scored and does stuff if it is, returns whether a goal was scored
    bool setpos(point);    //Moves Unit AND checks to see if it collides with the edge of the table, if so it bounces
    //Consturctors
    unit () {ball[n] = this; Id = n; n++;};
    unit (point, string, int, string);
    //Destructor
    ~unit ();
};

#endif
