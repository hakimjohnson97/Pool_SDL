#include "unit.h"

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

