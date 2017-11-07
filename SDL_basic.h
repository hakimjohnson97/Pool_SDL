#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include <string>
using namespace std;

//Functions
SDL_Surface* loadimage(string file);
SDL_Surface* loadimageck(string file, int R, int G, int B);
void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip );
Uint32 get_pixel32( SDL_Surface *surface, int x, int y );
void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel );

SDL_Rect SDL_MkRect(int x, int y, int width, int height)
{
    SDL_Rect box;
    box.x = x;
    box.y = y;
    box.w = width;
    box.h = height;
    return box;
}

SDL_Surface* loadimage(string file)
{
    SDL_Surface* image24;
    SDL_Surface* image32;
    image24 = IMG_Load(file.c_str());
    if (image24 != NULL)    {
        image32 = SDL_DisplayFormat(image24);
        SDL_FreeSurface(image24);
    } 
    return image32;
}

SDL_Surface* loadimageck(string file, int R, int G, int B)
{
    SDL_Surface* image24;
    SDL_Surface* image32;
    image24 = IMG_Load(file.c_str());
    if (image24 != NULL)    {
        image32 = SDL_DisplayFormat(image24);
        SDL_FreeSurface(image24);
    }
    Uint32 colorkey = SDL_MapRGB( image32->format, R, G, B); 
    SDL_SetColorKey( image32, SDL_SRCCOLORKEY, colorkey ); 
    return image32;
}

void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip )
{
     SDL_Rect offset;
     offset.x = x;
     offset.y = y;
     SDL_BlitSurface(source, clip, destination, &offset);
}

Uint32 get_pixel32( SDL_Surface *surface, int x, int y ) 
{
    //Convert the pixels to 32 bit 
    Uint32 *pixels = (Uint32 *)surface->pixels; 
    //Get the requested pixel 
    return pixels[ ( y * surface->w ) + x ];
} 
    
void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel ) 
{ 
     //Convert the pixels to 32 bit 
     Uint32 *pixels = (Uint32 *)surface->pixels; 
     //Set the pixel 
     pixels[ ( y * surface->w ) + x ] = pixel; 
} 

int getbits(int x, int lsb, int msb)
{
    return (x >> lsb) & ~(~0 << (msb-lsb+1));
}
