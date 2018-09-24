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
using namespace std;

//Functions
SDL_Surface* loadimage(string file);
SDL_Surface* loadimageck(string file, int R, int G, int B);
void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip );
void End(int player);
void UpdateTurn();
void UpdateGoal();
void Setstick(point mouseloc, int power);
void Removestick();
string I2S(int number);

const int SCREEN_LENGTH = 1200;
const int SCREEN_HEIGHT = 880;
const int TABLE_LENGTH = 1068;
const int TABLE_HEIGHT = 577;
const point TABLE_POINT = point(66, 144);
const float CONSTFRICTION = -8;
const float PROPFRICTION = 0.09;
const float BOUNCE = 0.8;
const int NBALLS = 15;
int CALCS_PER_SEC = 500;

//Colours
const SDL_Color GREEN = {0, 255, 0};
const SDL_Color RED = {255, 0, 0};
const SDL_Color WHITE = {255, 255, 255};

//These act as constants but need to be set in main
int GMESSHEIGHT; 
int GMESSLENGTH;
int TMESSHEIGHT;
int TMESSLENGTH;

//Global Variables
SDL_Surface* screen;
SDL_Surface* background;
SDL_Surface* message; //Used for diplaying messages like number of goals
SDL_Event event;
bool quit = false;
int frame = 0, offset = 0;//offset is the amount of time was waited by SDL_Delay which is used to calculate the fps;
TTF_Font *font;

//Music stuff
Mix_Chunk* ball_collision, *pocket_in, *stick_hit, *wall_hit;
Mix_Music* song;

//stick stuff
SDL_Surface* stick, *stickrot;
point stickpos;
int stickangle;

//Player and goal stuff
int Nballs = 0;
int rball = -1;   //Shows which player owns red ball, if its 0 player 1 owns;  -1 means a colour has not been assigned to a pleyer yet
int goals[2];     //Records the number of goals scored
int player = 0;    //Shows which players turn it is
int whballin = false;   //Shows whether the white ball has been in so when the player turn is changed it can be moved
bool endturn = true;   //This is true when turn has ended so a player may shoot, it is false when balls are still moving after a shot
int goaliscored = false;   //This shows whether a gaol was scored so the player turn shouldn't be changed

 
unit* unit::ball[100];
unit* whball;
int unit::n = 0;


void End(int player)
{
    SDL_Event event;
    SDL_Delay(1000);
    SDL_FillRect(screen, NULL, 0x000000);
    font = TTF_OpenFont("lazy.ttf", 56);
    message = TTF_RenderText_Solid(font, ("Player " + I2S(player + 1) + " has won!").c_str(), WHITE);
    apply_surface(SCREEN_LENGTH/2 - message->w/2, SCREEN_HEIGHT/2 - message->h/2, message, screen, NULL);
    SDL_Flip(screen);
    while (quit == false)
        if (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                quit = true;
}

void UpdateGoal()
{
    if (rball == 0)    {    //Writes the new text on the screen
        message = TTF_RenderText_Solid(font, ("Player 1 Goals: " + I2S(goals[0])).c_str(), RED);
        apply_surface(200, 800, message, screen, NULL);
        message = TTF_RenderText_Solid(font, ("Player 2 Goals: " + I2S(goals[1])).c_str(), GREEN);
        apply_surface(800, 800, message, screen, NULL);
    }
    else if (rball == 1)   {
        message = TTF_RenderText_Solid(font, ("Player 1 Goals: " + I2S(goals[0])).c_str(), GREEN);
        apply_surface(200, 800, message, screen, NULL);
        message = TTF_RenderText_Solid(font, ("Player 2 Goals: " + I2S(goals[1])).c_str(), RED);
        apply_surface(800, 800, message, screen, NULL);
    }
    else    { //Colour of ball has not been set
        message = TTF_RenderText_Solid(font, ("Player 1 Goals: " + I2S(goals[0])).c_str(), WHITE);
        apply_surface(200, 800, message, screen, NULL);
        message = TTF_RenderText_Solid(font, ("Player 2 Goals: " + I2S(goals[1])).c_str(), WHITE);
        apply_surface(800, 800, message, screen, NULL);
    }
}

void UpdateTurn()
{
    SDL_Rect box;
    message = TTF_RenderText_Solid(font, ("Player " + I2S(player+1) + "'s Turn").c_str(), WHITE);
    box.x = SCREEN_LENGTH/2 - message->w/2; box.y = 100;
    box.w = TMESSLENGTH; box.h = TMESSHEIGHT;
    apply_surface(box.x, box.y, background, screen, &box);
    apply_surface(SCREEN_LENGTH/2 - message->w/2, 100, message, screen, NULL);
}
     

string I2S(int number)
{
   stringstream ss;  //create a stringstream
   ss << number;  //add number to the stream
   return ss.str();  //return a string with the contents of the stream
}

string D2S(double number)
{
   stringstream ss;  //create a stringstream
   ss << number;  //add number to the stream
   return ss.str();  //return a string with the contents of the stream
}

void apply_surface_rotated(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip, int angle)
{
     SDL_Surface* dot;
     dot = loadimage("dot.png");
     int i, n, m, row = 1, ang;
     float dist;
     point cent, pos, loc;
     Uint32* screenpix, *sourcepix;
     Uint32 pix = SDL_MapRGB(destination->format, 0, 0, 0);
     sourcepix = (Uint32*)source->pixels;
     screenpix = (Uint32*)destination->pixels;
     n = 0;
     i = x + (y*destination->w)-1;
     cent = point((int)source->w/2, (int)source->h/2);
     while (n < (source->w*source->h-1) && i < (destination->w*destination->h-1))    {
         dist = (point((int)n/source->w, n % source->w) - cent).abs();
         pos = point(i % destination->w, int(i/destination->w));
         ang = AngBetPoints(cent, point(n % source->w, int(n/source->w)));
         loc = PolarProjection(pos, dist, (ang + 180)%360);
         pos = PolarProjection(loc, dist, (ang + angle) % 360);
         pos.x = (int)pos.x; pos.y = (int)pos.y;
         screenpix[int(pos.y*destination->w + pos.x)] = sourcepix[n];
         n++;
         i++;
         if (n % source->w == 0)     {
             i = (x + ((row+y)*destination->w));
             row++;
         }
     }
}

void Removestick()
{
     SDL_Rect box;
     box.x = stickpos.x; box.y = stickpos.y;
     box.w = stickrot->w; box.h = stickrot->h;
     apply_surface(stickpos.x, stickpos.y, background, screen, &box);
}

void Setstick(point mousepos, int power)
{
    point pos;

    if (stickangle != (360 - AngBetPoints(mousepos, whball->pos) + 180))
        stickrot = rotozoomSurface(stick, stickangle, 1, SMOOTHING_ON);
    stickangle = 360 - AngBetPoints(whball->pos, mousepos);
    pos = PolarProjection(point(stickrot->w/2, stickrot->h/2), 100, (360 - stickangle));
    stickpos = PolarProjection(whball->pos, 50 + power, 360 - (stickangle - 180)) - pos;
    apply_surface(stickpos.x, stickpos.y, stickrot, screen, NULL);
}

int main( int argc, char* args[] )
{
    unit* temp;
    button exit;
    point uvect;
    SDL_Surface* wallimg;
    int i, n, x, y, ft, test = 0; //offset is the amount of time was waited by SDL_Delay which is used to calculate the fps
    double clickt = -1, display = 0;

    //Init
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    screen = SDL_SetVideoMode(SCREEN_LENGTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE );//| SDL_FULLSCREEN);
    background = loadimage("Images/pool_table.png");
    apply_surface(0, 0, background, screen, NULL);
    SDL_WM_SetCaption( "Pool", NULL ); 
    SDL_Flip(screen);
    
    //Music Init
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
    int audio_channels = 2;
    int audio_buffers = 1024;
    Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers);
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
    Mix_AllocateChannels(16);
    ball_collision = Mix_LoadWAV("Sounds/ball_collision.wav");
    pocket_in = Mix_LoadWAV("Sounds/pocket_in.wav");
    stick_hit = Mix_LoadWAV("Sounds/pool_stick_hit.wav");
    wall_hit = Mix_LoadWAV("Sounds/wall_hit.wav");
    
    //TTF Init
    font = TTF_OpenFont("lazy.ttf", 28);
    message = TTF_RenderText_Solid(font, "Player 1 Goals: 0", WHITE);
    apply_surface(200, 800, message, screen, NULL);
    message = TTF_RenderText_Solid(font, "Player 2 Goals: 0", WHITE);
    apply_surface(800, 800, message, screen, NULL);
    GMESSHEIGHT = message->h;
    GMESSLENGTH = message->w;
    message = TTF_RenderText_Solid(font, "Player 1's Turn", WHITE);
    apply_surface(SCREEN_LENGTH/2 - message->w/2, 100, message, screen, NULL);
    TMESSHEIGHT = message->h;
    TMESSLENGTH = message->w;
    //Creating Units
    whball = new unit(point(400,TABLE_POINT.y + TABLE_HEIGHT/2), "Images/wh_ball.png", 40, "whball");
    stick = loadimageck("Images/pool_stick.png", 128, 64, 0);
    stickrot = stick;
    exit = button(point(SCREEN_LENGTH - 100, SCREEN_HEIGHT - 50), "Images/Exit.png", 100, 50);
    i = 1;
    n = 0;
    x = 700;
    while (i <= NBALLS)    {
        for (y = n*-20; y <= n*20; y += 40)    {
            if (i == 5)
                temp = new unit(point(x, y + TABLE_POINT.y + TABLE_HEIGHT/2), "Images/bl_ball.png", 40, "blball");
            else if (i % 2 == 0)
                temp = new unit(point(x, y + TABLE_POINT.y + TABLE_HEIGHT/2), "Images/pool_ball_r.png", 40, "ball_r");
            else 
                temp = new unit(point(x, y + TABLE_POINT.y + TABLE_HEIGHT/2), "Images/pool_ball_g.png", 40, "ball_g");
            i++;
            
        }     
        x += 40;   
        n++;
    }
    //Main loop
    while (quit == false)    {
        ft = SDL_GetTicks();
        if (frame % 6 == 0)
            Removestick();
        unit::update();     
        if (SDL_PollEvent(&event))    {
            if (event.type == SDL_MOUSEBUTTONDOWN)    {
                if (event.button.button == SDL_BUTTON_LEFT)
                    clickt = SDL_GetTicks();
                if (event.button.button == SDL_BUTTON_RIGHT)
                    clickt = -1;
            }
            if (event.type == SDL_MOUSEBUTTONUP)    {
                if (event.button.button == SDL_BUTTON_LEFT)    {
                    if (exit.pressed(point(event.button.x, event.button.y)))
                        quit = true;
                    if (endturn == true && clickt != -1)     {  //Right click was not pressed
                        if ((SDL_GetTicks() - clickt) > 2000)
                            clickt = SDL_GetTicks() - 2000;
                        uvect = (point(event.button.x , event.button.y) - whball->pos)/((point(event.button.x , event.button.y) - whball->pos).abs());
                        whball->vel = uvect*(SDL_GetTicks() - clickt);
                        Mix_Volume(Mix_PlayChannel(-1, stick_hit, 0), (SDL_GetTicks() - clickt)/50);
                        clickt = SDL_GetTicks() - clickt;
                        while (clickt > -50)    {
                            Removestick();
                            Setstick(point(event.motion.x, event.motion.y), (clickt)*0.04);
                            SDL_Flip(screen);
                            clickt -= 100;
                        }
                        clickt = -1;
                        Removestick();
                        endturn = false;
                   }
                }
            }

            if (event.type == SDL_QUIT)
                quit = true;
        }
        

        if (frame % 6 == 0)    {
            UpdateTurn();
            UpdateGoal();
            if (endturn == true)    {
                if (clickt != -1)    {
                    if ((SDL_GetTicks() - clickt) > 2000)
                        clickt = SDL_GetTicks() - 2000;
                    Setstick(point(event.motion.x, event.motion.y), (SDL_GetTicks() - clickt)*0.04);
                }
                else    {
                    Setstick(point(event.motion.x, event.motion.y), 0);
                }
            }
            SDL_Flip(screen);
        }

        if (frame % CALCS_PER_SEC == 0)    {
           SDL_WM_SetCaption( ("Pool  cps-" + I2S(CALCS_PER_SEC)).c_str(), NULL );

           test = 0;

        }
        frame++;
        CALCS_PER_SEC = ((double)frame/(double)(SDL_GetTicks() - offset)*1000)/1;
        

    }  
    for (i = 0; i < unit::n; i++)
        SDL_FreeSurface(unit::ball[i]->image);
    SDL_Quit();
    TTF_Quit();
}
