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

// g++ pool.cpp -lmingw32 -lSDLmain -lSDL -lSDL_image -lSDL_ttf -lSDL_mixer

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
int rball = -1; //Shows which player owns red ball, if its 0 player 1 owns;  -1 means a colour has not been assigned to a pleyer yet
int goals[2];   //Records the number of goals scored
int player = 0;  //Shows which players turn it is
int whballin = false; //Shows whether the white ball has been in so when the player turn is changed it can be moved
bool endturn = true; //This is true when turn has ended so a player may shoot, it is false when balls are still moving after a shot
int goaliscored = false; //This shows whether a gaol was scored so the player turn shouldn't be changed


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

button::button(point arg_pos, string arg_image, int arg_l, int arg_h)
{
    SDL_Rect box;
    box.x = arg_pos.x;   box.y = arg_pos.y;
    box.w = arg_l; box.h = arg_h;
    image = loadimage(arg_image);
    pos = arg_pos;
    length = arg_l;
    height = arg_h;
    apply_surface(pos.x, pos.y, image, screen, NULL);
}

bool button::pressed(point presspos)
{
    if ((presspos.x > pos.x && presspos.y < pos.x + length) && (presspos.y > pos.y && presspos.y < pos.y + height))
        return true;
}

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
    bool goalscored();     //This is an extention of update because it was pretty long, checks if goal scored and does stuff if it is, returns whether a gaol was scored
    bool setpos(point);    //Moves Unit AND checks to see if it collides with the edge of the table, if so it bounces
    //Consturctors
    unit () {ball[n] = this; Id = n; n++;};
    unit (point, string, int, string);
    //Destructor
    ~unit ();
 };
 
unit* unit::ball[100];
unit* whball;
int unit::n = 0;

unit::unit (point param_pos, string param_image, int param_collis, string param_type)
{
    type = param_type;
    collis = param_collis;
    pos = param_pos;
    vel = point(0,0);
    force = point(0,0);
    image = loadimageck(param_image, 0x0,0xFF,0xFF);
    apply_surface(pos.x - collis/2, pos.y - collis/2, image, screen, NULL);
    ball[n] = this;
    Id = n;
    n++;
}

bool unit::setpos (point param_pos)
{
    SDL_Rect box, mesbox;
    SDL_Rect posbox;
    bool collided = false;
    int i;
    
    box.x = prepos.x - collis/2; box.y = prepos.y - collis/2;
    box.w = collis; box.h = collis;
    posbox.x = param_pos.x; posbox.y = param_pos.y;
    posbox.w = collis; posbox.h = collis;
    if (param_pos.x < TABLE_POINT.x + collis/2 || param_pos.x > TABLE_POINT.x + TABLE_LENGTH - collis/2)    {
        vel.x = -vel.x;
        vel.y = vel.y*BOUNCE;
        collided = true;
    }
    if (param_pos.y < TABLE_POINT.y + collis/2 || param_pos.y > TABLE_POINT.y + TABLE_HEIGHT - collis/2)    {
        vel.y = -vel.y;
        vel.x = vel.x*BOUNCE;
        collided = true;
    }
    if (collided == false) {
        pos = param_pos;
        if (frame % 6 == 0)    {
            apply_surface(prepos.x - collis/2, prepos.y - collis/2, background, screen, &box);
            apply_surface(pos.x - collis/2, pos.y - collis/2, image, screen, NULL);
            prepos = pos;
        }
        return true;
    }
    Mix_Volume(Mix_PlayChannel(-1, wall_hit, 0), vel.abs()/10);
    return false;
}

unit::~unit()
{
     int i;
     SDL_Rect box;
     box.x = prepos.x - collis/2; box.y = prepos.y - collis/2;
     box.w = collis; box.h = collis;
     apply_surface(prepos.x - collis/2, prepos.y - collis/2, background, screen, &box);
     for (i = Id + 1; i < n; i++)    {
         ball[i-1]->Id--;
         ball[i-1] = ball[i];
     }
     n--;
}

bool unit::goalscored()
{
    SDL_Rect box;
    SDL_Event event;
    int i;
    bool collided = false;
    if ((pos - TABLE_POINT).abs() < 20 + collis/2 || (pos - (point(TABLE_LENGTH, 0) + TABLE_POINT)).abs() < 20 + collis/2
    || (pos - (point(0, TABLE_HEIGHT) + TABLE_POINT)).abs() < 20 + collis/2
    || (pos - (point(TABLE_LENGTH, TABLE_HEIGHT) + TABLE_POINT)).abs() < 20 + collis/2
    || (pos - (point((TABLE_LENGTH)/2, 0) + TABLE_POINT)).abs() < 10 + collis/2
    || (pos - (point((TABLE_LENGTH)/2, TABLE_HEIGHT) + TABLE_POINT)).abs() < 10 + collis/2)
    //Checks whether goal was scored
    {
        Mix_Volume(Mix_PlayChannel(-1, pocket_in, 0), 50);
        if (type == "whball")    {
            whballin = true;
            goaliscored = false;
            vel = point(0, 0);
            //Removes the white ball's image from the screen
            box.x = prepos.x - collis/2;  box.y = prepos.y - collis/2;
            box.w = collis; box.h = collis;
            apply_surface(box.x, box.y, background, screen, &box);
            pos = point(0, 0); //So it doesn't intefere with collisions with other balls
            return 1;
        }
        else {
             //Removes the previous text from the screen
             box.x = 200; box.y = 800;
             box.w = GMESSLENGTH; box.h = GMESSHEIGHT;
             apply_surface(box.x, box.y, background, screen, &box);
             box.x = 800;
             apply_surface(box.x, box.y, background, screen, &box);
             if (rball == -1)    { //If a color ball wasn't assigned to a player yet
                 if (type == "ball_r")
                     rball = player;
                 else
                     rball = (!player);
             }
             if (type == "ball_r") 
                 goals[rball]++;   //Player which owns red ball N of goals increased
             else if (type == "ball_g")
                 goals[(!rball)]++;
             else if (type == "blball" && goals[player] >= (NBALLS/2))    {  //Player has won
                 End(player);
                 return 1;
             }
             else    {
                 End(!player);
                 return 1;
             }
             UpdateGoal();
             if (((type == "ball_r" && rball == player) || (type == "ball_g" && rball == (!player))) && goaliscored != -1)
                 goaliscored = true; //So when the player's turn is updated when the balls stop moving, it will be changed again so it will be the same player's turn
             else 
                 goaliscored = -1; //In case a player scores their ball and scores some one else's ball after that
            delete this;
        }

    } 
}

void unit::update()
{
    SDL_Rect box;
    point uvect, postpos, postposA, prepoint;
    float b;
    int i, a;
    bool moving = false, collided = false;
    for (i = 0; i < n; i++)    {
        postpos = ball[i]->pos + (ball[i]->vel/CALCS_PER_SEC);
        //Friction
        if (ball[i]->vel.abs() != 0)    {
            uvect = ball[i]->vel/ball[i]->vel.abs();
            ball[i]->vel = ball[i]->vel + uvect*((ball[i]->vel.abs()*PROPFRICTION)*CONSTFRICTION)/CALCS_PER_SEC;
        }
        if (ball[i]->vel.abs() < abs(CONSTFRICTION))
            ball[i]->vel = point(0,0);
        //Collision
        for (a = i + 1; a < n; a++)    { 
            postposA = ball[a]->pos + (ball[a]->vel/CALCS_PER_SEC);
            if ((postpos - postposA).abs() < (ball[i]->collis/2 + ball[a]->collis/2))    {  
                Mix_Volume(Mix_PlayChannel(-1, ball_collision, 0), (ball[i]->vel.abs() + ball[a]->vel.abs())/10);
                uvect = (postposA - postpos)/(postposA - postpos).abs();
                b = uvect *= (ball[a]->vel - ball[i]->vel);
                ball[a]->vel = ball[a]->vel - (uvect*b);
                ball[i]->vel = ball[i]->vel + (uvect*b);
            }
        }        
        //Moving
        ball[i]->setpos(ball[i]->pos + ball[i]->vel/CALCS_PER_SEC);
        if (ball[i]->goalscored() == 1)
            return;
        //If ball moved
        if (ball[i]->vel != point(0,0)) 
           moving = true;
    }

    //Update Player's turn
    if (endturn == false && moving == false)    { 
    printf("%i     %i\n", 360 - AngBetPoints(whball->pos, point(event.motion.x, event.motion.y)), stickangle);
        if (goaliscored != true && whballin == false)    {
            player = (!player);
            UpdateTurn();
        } 
        goaliscored = false;
        if (whballin == true)    {
            //Applies instruction to move the ball
            message = TTF_RenderText_Solid(font, "Please click in the area to place your ball:", WHITE);
            apply_surface(SCREEN_LENGTH/2 - message->w/2, 50, message, screen, NULL);
            whballin = false;
            SDL_Flip(screen);
            //Moves the ball to the cursor's pos and starts the game again when the player presses a button
            while (quit == false)    {
                if (SDL_PollEvent(&event))     {
                    if (event.type == SDL_MOUSEMOTION)    {
                        for (i = 0; i < n; i++)    {
                            if ((ball[i]->pos - point(event.motion.x, event.motion.y)).abs() < whball->collis && ((ball[i]) != (whball)))
                                collided = true;
                            }
                            if (collided == false)
                                whball->setpos(point(event.motion.x, event.motion.y));
                            else
                                collided = false;
                            SDL_Flip(screen);
                        }
                        if (event.type == SDL_MOUSEBUTTONUP)    {
                            for (i = 0; i < n; i++)    {
                                if ((ball[i]->pos - point(event.motion.x, event.motion.y)).abs() < whball->collis && ((ball[i]) != (whball)))
                                    collided = true;
                                }
                            if (collided == false)    {
                                whball->setpos(point(event.button.x, event.button.y));
                                break;
                            }
                        }
                        if (event.type == SDL_QUIT)    {
                        quit = true;
                        }
                    }
                    frame++;
                    if (int(1000/CALCS_PER_SEC) == 0)
                        SDL_Delay(1);
                    else
                        SDL_Delay(1000/CALCS_PER_SEC);
                }
            }
            
        //Removes the instructions to move the ball
        box.x = SCREEN_LENGTH/2 - message->w/2; box.y = 50;
        box.w = message->w; box.h = message->h;
        apply_surface(box.x, box.y, background, screen, &box);
        endturn = true;
    }
            
}

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
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

string D2S(double number)
{
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
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
    // SDL_Rect box;
   //  box.x = stickpos.x; box.y = stickpos.y;
   //  box.w = stickrot->w; box.h = stickrot->h;
   //  apply_surface(stickpos.x, stickpos.y, background, screen, &box);
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

    //Init Stuff
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    screen = SDL_SetVideoMode(SCREEN_LENGTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE );//| SDL_FULLSCREEN);
    background = loadimage("Images/pool_table.png");
    apply_surface(0, 0, background, screen, NULL);
    SDL_WM_SetCaption( "Pool", NULL ); 
    SDL_Flip(screen);
    
    //Music Stuff
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
    
    //TTF stuff
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
     /*       if (event.type == SDL_MOUSEMOTION)    {
                if (endturn == true)    {
                    if (clickt != -1)    {
                        if ((SDL_GetTicks() - clickt) > 2000)
                            clickt = SDL_GetTicks() - 2000;
                        Removestick();
                        Setstick(point(event.motion.x, event.motion.y), (SDL_GetTicks() - clickt)*0.04);
                    }
                    else    {
                        Removestick();
                        Setstick(point(event.motion.x, event.motion.y), 0);
                    }
                }
            }*/
            if (event.type == SDL_QUIT)
                quit = true;
        }
        
     //   if ((int)(CALCS_PER_SEC/100) != 0)
      //      if (frame % ((int)(CALCS_PER_SEC/100)) == 0) {
      //         SDL_Flip(screen);
      //         test++;
      //         }
     //   if (test % 100 == 0) {
      //      printf("Test is %i\n\n\n\n\n", SDL_GetTicks());
      //      test = 0;}
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
  /*      if (frame >= int(display))    {
            test++;
            SDL_Flip(screen);
            display += double(CALCS_PER_SEC)/100;
           // printf((D2S(double(CALCS_PER_SEC)/100) + "\n").c_str());
        } */
        if (frame % CALCS_PER_SEC == 0)    {
           SDL_WM_SetCaption( ("Pool  cps-" + I2S(CALCS_PER_SEC)).c_str(), NULL );
        //   printf("%i\n", test);
          // printf((D2S(display) + "\n").c_str());
           test = 0;
           //printf((D2S((double)SDL_GetTicks()/(double)frame) + '\n').c_str()); 
            //printf((I2S(SDL_GetTicks())  + '\t'+ I2S(ft) + '\n').c_str());
            //printf((I2S((Uint32)(1000/CALCS_PER_SEC)) + '\n').c_str());
           // printf((D2S(CALCS_PER_SEC) + '\n').c_str());
          // printf("%i\n", (int)CALCS_PER_SEC);
        }
        frame++;
        CALCS_PER_SEC = ((double)frame/(double)(SDL_GetTicks() - offset)*1000)/1;
        
        //SDL_Delay((int)(1000/(CALCS_PER_SEC*3)));
        //offset += (int)(1000/(CALCS_PER_SEC*3));
    }  
    for (i = 0; i < unit::n; i++)
        SDL_FreeSurface(unit::ball[i]->image);
    SDL_Quit();
    TTF_Quit();
}
