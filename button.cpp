#include "button.h"

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
