/* See LICENSE file for copyright and license details. */

Object objects[] = {
	/* symbol   flags                                  ontick        argument */
	{ VACUUM,   OF_OPEN,                               falling,      {0} },
[1] = 	{L'o',      OF_PLAYER|OF_OPEN|OF_FALL,             NULL,         {.i = 2} },
[2] = 	{L'a',      OF_PLAYER|OF_OPEN|OF_FALL,             NULL,         {.i = 2} },
	{L'_',      OF_OPEN|OF_STICK|OF_JUMPFROM,          NULL,         {0} },
	{L'|',      0,                                     NULL,         {0} },
	{L'*',      OF_OPEN,                               earnenergy,   {.i = 2} },
	{L')',      OF_OPENLEFT|OF_STICK,                  finish,       {0} },
	{L'(',      OF_OPENRIGHT|OF_STICK,                 finish,       {0} },
	{L't',      0,                                     cannon,       {.v = &objects[11]} },
	{L'j',      0,                                     cannon,       {.v = &objects[12]} },
[11] =	{L'⋅',      OF_OPEN,                               cannonball,   {.i = +1} },
[12] =	{L'.',      OF_OPEN,                               cannonball,   {.i = -1} },
	{L'#',      OF_FALL|OF_PUSHABLE,                   NULL,         {0} },
	{L'@',      OF_FALL|OF_PUSHABLE,                   NULL,         {0} },

	/*
	{L'>', route, NULL, NULL },
	{L'<', route, NULL, NULL },
	{L'^', route, NULL, NULL },
	{L'v', route, NULL, NULL },
	{L'?', objrand, NULL, NULL },
	{L'%', jumpglue, NULL, NULL },
	{L'~', spring, NULL, NULL },
	{L'-', bump, NULL, NULL },
	{L',', hurt, NULL, {.i = 2}},
	{L':', secret, NULL, NULL },
	*/
};

#include "levels.h"

Level levels[] = {
	{ "Reach the exit", lev0 },
	{ "Reach the exit", lev1 },
	{ "Reach the exit", lev2 },
	{ "Run", lev3 },
	{ "Climb up", lev4 },
	{ "Welcome to the arena", arena },
};

/* key definitions */
static Key keys[] = {
	/* key         function     argument */
        { 'q',         quit,        {.i = 1} },
        { CTRL('c'),   quit,        {.i = 0} },
        { 'r',         restart,     {0} },
        { 'a',         walkleft,    {.v = &objects[1]} },
        { 'd',         walkright,   {.v = &objects[1]} },
        { 'w',         jump,        {.v = &objects[1]} },
        { 'j',         walkleft,    {.v = &objects[2]} },
        { 'l',         walkright,   {.v = &objects[2]} },
        { 'i',         jump,        {.v = &objects[2]} },
};
