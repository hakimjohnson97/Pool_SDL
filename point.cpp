#include "point.h"

point::point (float a, float b)
{
    x = a;
    y = b;
} 

point point::operator+ (point param)
{
    return point(x + param.x, y + param.y);
}

point point::operator- (point param)
{
    return point(x - param.x, y - param.y);
}

point point::operator* (float param)
{
    return point(x * param, y * param);
}

point point::operator/ (float param)
{
    return point(x / param, y / param);
}

float point::operator*= (point param)
{
    return x * param.x + y * param.y;
}

bool point::operator!= (point param)
{
     if (x == param.x && y == param.y)
         return false;
     return true;
}

bool point::operator== (point param)
{
     if (x == param.x && y == param.y)
         return true;
     return false;
}

int AngBetPoints(point pos1, point pos2)
{
    return (int((atan2(pos2.y - pos1.y, pos2.x - pos1.x)*180/PI) + 360) % 360) ;
}

point PolarProjection(point origin, int dist, float angle)
{
      angle = angle*PI/180;
      return point(origin.x + dist*cos(angle), origin.y + dist*sin(angle));
}
