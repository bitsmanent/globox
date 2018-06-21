/* See LICENSE file for copyright and license details. */

Object objects[] = {
	/* symbol   z-index   flags                                  ontick        argument */
	{ VACUUM,        0,   OF_OPEN,                               falling,      {0} },
[1] = 	{'o',            8,   OF_PLAYER|OF_OPEN|OF_FALL,             NULL,         {.i = 2} },
[2] = 	{'a',            8,   OF_PLAYER|OF_OPEN|OF_FALL,             NULL,         {.i = 2} },
	{'_',            1,   OF_OPEN|OF_STICK|OF_JUMPFROM,          NULL,         {0} },
	{'|',            1,   0,                                     NULL,         {0} },
	{'*',            2,   OF_OPEN,                               earnenergy,   {.i = 2} },
	{')',            9,   OF_OPENLEFT|OF_STICK,                  finish,       {0} },
	{'(',            9,   OF_OPENRIGHT|OF_STICK,                 finish,       {0} },
	{'t',            1,   0,                                     cannon,       {.v = &objects[11]} },
	{'j',            1,   0,                                     cannon,       {.v = &objects[12]} },
[11] =	{'.',            2,   OF_OPEN|OF_AI,                         cannonball,   {.i = +1} },
[12] =	{',',            2,   OF_OPEN|OF_AI,                         cannonball,   {.i = -1} },
	{'#',            2,   OF_FALL|OF_PUSHABLE,                   NULL,         {0} },
	{'@',            2,   OF_FALL|OF_PUSHABLE,                   NULL,         {0} },
	{'x',            8,   OF_PLAYER|OF_AI|OF_OPEN|OF_FALL,       zombie,       {.i = 8} },
};

#include "levels.h"

Level levels[] = {
	{ "Reach the exit", lev0 },
	{ "Reach the exit", lev1 },
	{ "Reach the exit", lev2 },
	{ "Run", lev3 },
	{ "Climb up", lev4 },
	{ "Steep fall", lev5 },
	{ "Welcome to the arena", arena },
};

/* key definitions */
static Key keys[] = {
	/* key         function     argument */
        { 'q',         quit,        {.i = 1} },
        { CTRL('c'),   quit,        {.i = 0} },
        { 'h',         walkleft,    {.v = &objects[1]} },
        { 'n',         walkright,   {.v = &objects[1]} },
        { 'c',         jump,        {.v = &objects[1]} },
        { 'a',         walkleft,    {.v = &objects[2]} },
        { 'e',         walkright,   {.v = &objects[2]} },
        { ',',         jump,        {.v = &objects[2]} },
        { 'r',         restart,     {0} },
};
