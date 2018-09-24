#ifndef POINT_H
#define POINT_H

#include <iostream>
#include <cmath>
#define PI 3.14159
using namespace std;

class point
{
    public:
    float x, y;
    point () {x = 0; y = 0;};
    point (float, float);
    float abs () {return sqrt(x*x + y*y);}
    point operator + (point);
    point operator - (point);
    point operator * (float);
    point operator / (float);
    float operator *= (point);
    bool operator != (point);
    bool operator == (point);
};

#endif

    
