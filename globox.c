/* See LICENSE file for copyright and license details.
 *
 * globox is a platform game for the terminal.
 *
 * Each symbol displayed on the screen is called a block, including players.
 * Blocks are organized in a linked block list which is filled with the objects
 * found on the level.
 *
 * Keys and objects are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
*/

#define _BSD_SOURCE
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

#include "arg.h"
char *argv0;

/* macros */
#define LENGTH(X)	(sizeof X / sizeof X[0])
#define DELAY(B, K, D)  (++(B)->delays[(K)] < (D) ? 1 : ((B)->delays[(K)] = 0))
#define ISSET(F, B)     ((F) & (B))
#define TICK		64000
#define VACUUM          ' '
#define LINESEP         '\n'

/* VT100 escape sequences */
#define CLEAR           "\33[2J"
#define CLEARLN         "\33[2K"
#define CLEARRIGHT      "\33[0K"
#define CURPOS          "\33[%d;%dH"
#define CURSON          "\33[?25h"
#define CURSOFF         "\33[?25l"

#if defined CTRL && defined _AIX
  #undef CTRL
#endif
#ifndef CTRL
  #define CTRL(k)   ((k) & 0x1F)
#endif
#define CTRL_ALT(k) ((k) + (129 - 'a'))

/* object flags */
#define OF_OPENUP       1<<1
#define OF_OPENRIGHT    1<<2
#define OF_OPENDOWN     1<<3
#define OF_OPENLEFT     1<<4
#define OF_JUMPFROM     1<<5
#define OF_PLAYER       1<<6
#define OF_STICK        1<<7
#define OF_FALL         1<<8
#define OF_PUSHABLE     1<<9
#define OF_OPEN         (OF_OPENUP|OF_OPENRIGHT|OF_OPENDOWN|OF_OPENLEFT)

/* enums */
enum { KeyUp = -50, KeyDown, KeyRight, KeyLeft, KeyHome, KeyEnd, KeyDel, KeyPgUp, KeyPgDw };
enum { DelayCannon, DelayCannonBall, DelayFalling, DelayZombie, DelayMax };

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	char *name;
	char *map;
} Level;

typedef struct Object Object;
struct Object {
	char sym;
	unsigned int flags;
	int (*ontick)(Object *);
	const Arg arg;
};

typedef struct {
	const int key;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct Block Block;
struct Block {
	Object *o;
	int delays[DelayMax];
	int x, y;
	int energy;
	int _draw; /* used to optimize draw() */
	Block *next;
};

typedef struct {
	Block *blocks;
	int w, h;
} Scene;

/* function declarations */
void attach(Block *b);
int cannon(Object *o);
int cannonball(Object *o);
void checkgame(void);
char choose(char *opts, const char *msgstr, ...);
void cleanup(void);
void detach(Block *b);
void die(const char *errstr, ...);
void draw(void);
void drawbar(void);
int earnenergy(Object *o);
void *ecalloc(size_t nmemb, size_t size);
int falling(Object *o);
int finish(Object *o);
void flow(void);
void freescene(void);
int getkey(void);
void ioblock(int block);
void jump(const Arg *arg);
void keypress(void);
void level(int lev);
int mvprintf(int x, int y, char *fmt, ...);
Object *objbysym(char sym);
void objwalk(Object *o, int offset);
void quit(const Arg *arg);
void resize(int x, int y);
void restart(const Arg *arg);
void run(void);
void setup(void);
void sigwinch(int unused);
int sleepu(double usec);
void usage(void);
void walk(Block *blk, int offset);
void walkleft(const Arg *arg);
void walkright(const Arg *arg);
int zombie(Object *o);

/* variables */
Scene *scene;
struct termios origti;
int running = 1, lev;
int rows, cols;

/* configuration, allows nested code to access above variables */
#include "config.h"

/* function implementations */
void
attach(Block *b) {
	b->next = scene->blocks;
	scene->blocks = b;
}

int
cannon(Object *o) {
	Block *c, *nb;

	for(c = scene->blocks; c; c = c->next) {
		if(c->o->ontick != cannon || DELAY(c, DelayCannon, 128))
			continue;
		nb = ecalloc(1, sizeof(Block));
		nb->o = ((Object *)(c->o->arg.v));
		nb->x = c->x;
		nb->y = c->y;
		attach(nb);
	}
	return 0;
}

int
cannonball(Object *o) {
	Block *cb, *b, *p, *fb = NULL;
	int nx, nblk, offset;

	for(cb = scene->blocks; cb; cb = cb->next) {
		if(cb->o->ontick != cannonball || DELAY(cb, DelayCannonBall, 2))
			continue;
		if(fb) {
			detach(fb);
			free(fb);
			fb = NULL;
		}
		offset = cb->o->arg.i;
		nx = cb->x + offset;
		nblk = 0;
		p = NULL;
		for(b = scene->blocks; b; b = b->next) {
			if(b->x == nx && b->y == cb->y) {
				++nblk;
				if(!ISSET(b->o->flags, offset > 0 ? OF_OPENLEFT : OF_OPENRIGHT))
					fb = cb;
			}
			if(b->x == cb->x && b->y == cb->y && ISSET(b->o->flags, OF_PLAYER))
				p = b;
		}
		if(!nblk) {
			fb = cb;
			continue;
		}
		if(p) {
			fb = cb;
			p->energy -= 2;
		}
		cb->x = nx;
	}
	if(fb) {
		detach(fb);
		free(fb);
		fb = NULL;
	}
	return 0;
}

void
checkgame(void) {
	Block *fp = NULL, *p;
	int np = 0;

	for(p = scene->blocks; p;) {
		if(!ISSET(p->o->flags, OF_PLAYER)) {
			p = p->next;
			continue;
		}
		if(p->energy > 0) {
			++np;
			p = p->next;
		}
		else {
			fp = p;
			p = p->next;
			detach(fp);
			free(fp);
		}
	}
	if(np)
		return;
	if(choose("yn", "Level failed. Play again ([y]/n)?") == 'y')
		level(lev);
	else
		running = 0;
}

char
choose(char *opts, const char *msgstr, ...) {
	va_list ap;
	int c;
	char *o = NULL;

	va_start(ap, msgstr);
	fprintf(stdout, CURPOS, cols - 1, 0);
	vfprintf(stdout, msgstr, ap);
	va_end(ap);

	ioblock(1);
	while(!(o && *o) && (c = getkey()) != EOF) {
		if(c == '\n')
			o = &opts[0];
		else
			for(o = opts; *o; ++o)
				if(c == *o)
					break;
	}
	fprintf(stdout, CURPOS CLEARLN, cols - 1, 0);
	ioblock(0);
	return *(o ? o : opts);
}

void
cleanup(void) {
	freescene();
	tcsetattr(0, TCSANOW, &origti);
	printf(CURSON);
}

void
detach(Block *b) {
	Block **tb;

	for (tb = &scene->blocks; *tb && *tb != b; tb = &(*tb)->next);
	*tb = b->next;
}

void
die(const char *errstr, ...) {
	va_list ap;

	if(rows)
		cleanup();
	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

void
draw(void) {
	Block *b, *b2, *t;

	for(b = scene->blocks; b; b = b->next)
		b->_draw = 1;
	for(b = scene->blocks; b; b = b->next) {
		if(!b->_draw)
			continue;
		t = NULL;
		for(b2 = scene->blocks; b2; b2 = b2->next)
			if(ISSET(b2->o->flags, OF_PLAYER) && b->x == b2->x && b->y == b2->y && (t = b2))
				break;
		if(!t)
			for(b2 = scene->blocks; b2; b2 = b2->next)
				if(!ISSET(b2->o->flags, OF_OPEN) && b->x == b2->x && b->y == b2->y && (t = b2))
					break;
		if(!t)
			t = b;
		for(b2 = scene->blocks; b2; b2 = b2->next)
			if(b2 != t && t->x == b2->x && t->y == b2->y)
				b2->_draw = 0;
	}
	for(b = scene->blocks; b; b = b->next)
		if(b->_draw)
			mvprintf(b->x, b->y + 1, "%c", b->o->sym);
	drawbar();
}

void
drawbar(void) {
	Block *p;
	int i, len;

	len = mvprintf(1, 1, "[%d|%dx%d] %s ::", lev, scene->w, scene->h, levels[lev].name);
	for(p = scene->blocks; p; p = p->next) {
		if(!(p->o->flags & OF_PLAYER))
			continue;
		i = mvprintf(len+1, 1, " %c(*%d)(%dx%d)",
			p->o->sym, p->energy, p->x, p->y);
		len += i;
	}
	mvprintf(len+1, 1, "%s", CLEARRIGHT);
}

int
earnenergy(Object *o) {
	Block *b, *p;

	for(b = scene->blocks; b; b = b->next) {
		if(b->o != o)
			continue;
		for(p = scene->blocks; p; p = p->next)
			if(ISSET(p->o->flags, OF_PLAYER) && p->x == b->x && p->y == b->y)
				break;
		if(!p)
			continue;
		b->o = objbysym(VACUUM);
		p->energy += o->arg.i;
		break;
	}
	return 0;
}

void *
ecalloc(size_t nmemb, size_t size) {
	void *p;

	if(!(p = calloc(nmemb, size)))
		die("Cannot allocate memory.\n");
	return p;
}

int
falling(Object *o) {
	Block *b, *p;
	int ny, valid, skip;

	for(p = scene->blocks; p; p = p->next) {
		if(!ISSET(p->o->flags, OF_FALL) || DELAY(p, DelayFalling, 4))
			continue;
		valid = 0;
		skip = 0;
		ny = p->y + 1;
		for(b = scene->blocks; b; b = b->next) {
			if(b == p || b->x != p->x)
				continue;
			if(b->y == p->y) {
				if(ISSET(b->o->flags, OF_STICK))
					skip = 1;
			}
			else if(b->y == ny) {
				valid = 1;
				if(!ISSET(b->o->flags, OF_OPENUP))
					skip = 1;
			}
		}
		if(skip)
			continue;
		if(!valid) {
			p->energy = 0;
			continue;
		}
		p->y = ny;
	}
	return 0;
}

int
finish(Object *o) {
	Block *b, *p;
	int np = 0;

	for(p = scene->blocks; p; p = p->next) {
		if(!ISSET(p->o->flags, OF_PLAYER) || p->energy <= 0)
			continue;
		++np;
		for(b = scene->blocks; b; b = b->next)
			if(b->o->ontick == finish && b->x == p->x && b->y == p->y)
				break;
		if(!b)
			return 0;
	}
	if(!np)
		return 0;
	if(++lev >= LENGTH(levels)) {
		if(choose("yn", "The game has finished. Play again ([y]/n)?") == 'n') {
			running = 0;
			return 1;
		}
		lev = 0;
	}
	else if(choose("yn", "Level completed. Play next ([y]/n)?") == 'n') {
		running = 0;
		return 1;
	}
	level(lev);
	return 1;
}

void
flow(void) {
	int i;

	for(i = 0; i < LENGTH(objects); ++i)
		if(objects[i].ontick && objects[i].ontick(&objects[i]))
			return;
}

void
freescene(void) {
	Block *b;

	if(!scene)
		return;
	while(scene->blocks) {
		b = scene->blocks;
		scene->blocks = scene->blocks->next;
		free(b);
	}
	free(scene);
}

/* XXX quick'n dirty implementation */
int
getkey(void) {
	int key = getchar(), c;

	if(key != '\x1b' || getchar() != '[')
		return key;
	switch((c = getchar())) {
	case 'A': key = KeyUp; break;
	case 'B': key = KeyDown; break;
	case 'C': key = KeyRight; break;
	case 'D': key = KeyLeft; break;
	case 'H': key = KeyHome; break;
	case 'F': key = KeyEnd; break;
	case '1': key = KeyHome; break;
	case '3': key = KeyDel; break;
	case '4': key = KeyEnd; break;
	case '5': key = KeyPgUp; break;
	case '6': key = KeyPgDw; break;
	case '7': key = KeyHome; break;
	case '8': key = KeyEnd; break;
	default:
		/* debug */
		mvprintf(1, rows, "Unknown char: %c (%d)", c, c);
		break;
	}
	return key;
}

void
ioblock(int block) {
	struct termios ti;

	tcgetattr(0, &ti);
	ti.c_cc[VMIN] = block;
	tcsetattr(0, TCSANOW, &ti);
}

void
jump(const Arg *arg) {
	Block *b, *p;
	int up, down, canjump, upclose, dwclose;

	for(p = scene->blocks; p; p = p->next) {
		if(p->o != arg->v)
			continue;
		up = p->y - 1;
		down = p->y + 1;
		canjump = 0;
		dwclose = 0;
		upclose = -1;
		for(b = scene->blocks; b; b = b->next) {
			if(b->x != p->x)
				continue;
			if(b->y == p->y) {
				if(ISSET(b->o->flags, OF_JUMPFROM))
					canjump = 1;
			}
			else if(b->y == up) {
				if(!ISSET(b->o->flags, OF_OPENDOWN))
					upclose = 1;
				else if(upclose == -1)
					upclose = 0;
			}
			else if(b->y == down) {
				if(!ISSET(b->o->flags, OF_OPENUP))
					dwclose = 1;
			}
		}
		if(upclose || !(canjump || dwclose))
			continue;
		p->y = up;
		/* reset delays */
		for(up = 0; up < DelayMax; ++up)
			p->delays[up] = 0;
	}
}

void
keypress(void) {
	int key = getkey(), i;

	for(i = 0; i < LENGTH(keys); ++i)
		if(keys[i].key == key)
			keys[i].func(&keys[i].arg);
	while(getkey() != EOF); /* discard remaining input */
}

void
level(int num) {
	Object *o;
	Block *b;
	int i, len, x, y;
	char *map;

	if(num >= LENGTH(levels))
		die("%s: level %d does not exists.\n", argv0, num);
	freescene();
	scene = ecalloc(1, sizeof(Scene));
	map = levels[num].map;
	len = strlen(map);
	x = y = 1;
	for(i = 0; i < len; ++i) {
		if(map[i] == LINESEP) {
			++y;
			if(x > scene->w)
				scene->w = x;
			x = 1;
			continue;
		}
		o = objbysym(map[i]);
		if(!o)
			die("%s: unknown object '%c' at %dx%d.\n", argv0, map[i], x, y);
		b = ecalloc(1, sizeof(Block));
		b->o = o;
		b->x = x;
		b->y = y;
		attach(b);
		if(ISSET(o->flags, OF_PLAYER | OF_FALL)) {
			o = objbysym(VACUUM);
			if(o) {
				b = ecalloc(1, sizeof(Block));
				b->o = o;
				b->x = x;
				b->y = y;
				attach(b);
			}
		}
		++x;
	}
	scene->h = y + 1;
	/* revive the zombies players */
	for(b = scene->blocks; b; b = b->next)
		if(ISSET(b->o->flags, OF_PLAYER) && !b->energy)
			b->energy = b->o->arg.i;
	printf(CLEAR);
}

int
mvprintf(int x, int y, char *fmt, ...) {
	va_list ap;
	int len;

	printf(CURPOS, y, x);
	va_start(ap, fmt);
	len = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return len;
}

Object *
objbysym(char sym) {
	int i;

	for(i = 0; i < LENGTH(objects); ++i)
		if(objects[i].sym == sym)
			return &objects[i];
	return NULL;
}

void
objwalk(Object *o, int offset) {
	Block *b;

	for(b = scene->blocks; b; b = b->next)
		if(b->o == o)
			walk(b, offset);
}

void
quit(const Arg *arg) {
	if(!arg->i || choose("ny", "Are you sure (y/[n])?") == 'y')
		running = 0;
}

void
resize(int x, int y) {
	rows = x;
	cols = y;
}

void
restart(const Arg *arg) {
	if(choose("ny", "Restart the level (y/[n]?") == 'y')
		level(lev);
}

void
run(void) {
	ioblock(0);
	while(running) {
		while(cols < scene->w || rows < scene->h) {
			mvprintf(1, 1, "Terminal too small.");
			sleepu(10000);
		}
		draw();
		checkgame();
		keypress();
		flow();
		if(sleepu(TICK))
			die("%s: error while sleeping\n", argv0);
	}
	ioblock(1);
	mvprintf(1, cols, "%s", CLEARLN);
}

void
setup(void) {
	struct termios ti;
	struct sigaction sa;
	struct winsize ws;

	setlocale(LC_CTYPE, "");
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigwinch;
	sigaction(SIGWINCH, &sa, NULL);

	tcgetattr(0, &origti);
	cfmakeraw(&ti);
	ti.c_iflag |= ICRNL;
	ti.c_cc[VMIN] = 0;
	ti.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &ti);
	printf(CURSOFF);

	ioctl(0, TIOCGWINSZ, &ws);
	resize(ws.ws_row, ws.ws_col);
}

void
sigwinch(int unused) {
	struct winsize ws;

	ioctl(0, TIOCGWINSZ, &ws);
	resize(ws.ws_row, ws.ws_col);
	printf(CLEAR);
}

int
sleepu(double usec) {
        struct timespec req, rem;
        int r;

        req.tv_sec = 0;
        req.tv_nsec = usec * 1000;
        while((r = nanosleep(&req, &rem)) == -1 && errno == EINTR)
                req = rem;
        return r;
}

void
usage(void) {
	die("Usage: %s [-v] [-n <level>]\n", argv0);
}

void
walk(Block *blk, int offset) {
	Block *b;
	int nx, nblk;

	nx = blk->x + offset;
	nblk = 0;
	for(b = scene->blocks; b; b = b->next) {
		if(b->x != nx || b->y != blk->y)
			continue;
		++nblk;
		if(!ISSET(b->o->flags, offset > 0 ? OF_OPENLEFT : OF_OPENRIGHT)) {
			if(ISSET(b->o->flags, OF_PUSHABLE)) {
				walk(b, offset);
				if(b->x != nx)
					continue;
			}
			return;
		}
	}
	if(!nblk)
		return;
	blk->x = nx;
}

void
walkleft(const Arg *arg) {
	objwalk((Object *)arg->v, -1);
}

void
walkright(const Arg *arg) {
	objwalk((Object *)arg->v, +1);
}

int
zombie(Object *o) {
	Block *c;
	int t;

	for(c = scene->blocks; c; c = c->next) {
		if(c->o->ontick != zombie || DELAY(c, DelayZombie, 4))
			continue;
		t = rand() % 3;
		if(t == 2)
			t = -1;
		objwalk(c->o, t);
	}
	return 0;
}

int
main(int argc, char *argv[]) {
	ARGBEGIN {
	case 'n': lev = atoi(EARGF(usage())); break;
	case 'v': die("globox-"VERSION"\n");
	default: usage();
	} ARGEND;
	srand(time(NULL));
	setup();
	level(lev);
	run();
	cleanup();
	return 0;
}
