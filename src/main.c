/*
 *--------------------------------------
 * Program Name: Cerkel Snek
 * Author: Rodger "Iambian" Weisman
 * License: MIT
 * Description: Nom noms, sneak through doors, don't hit yourself or walls.
 *--------------------------------------
*/
#define VERSION_INFO "v0.2"

#define TRANSPARENT_COLOR 0xF8
#define GREETINGS_DIALOG_TEXT_COLOR 0xDF
#define DIALOG_BOX_COLOR 0x08

#define SNAKE_COLOR 0x00
#define CERKEL_COLOR 0xE0
#define WINNER_COLOR 0x87

#define WBORD_TOP 14
#define WBORD_LEFT 4
#define WINDOW_BORDER 2
#define WINDOW_HEIGHT (LCD_HEIGHT-WBORD_TOP-(WINDOW_BORDER*3))
#define WINDOW_WIDTH (LCD_WIDTH-WBORD_LEFT-(WINDOW_BORDER*2))

#define BASE_CIRCLES_PER_STAGE 10
#define ADDITIONAL_CIRCLES_PER_STAGE 3
#define SNAKE_GROWTH_RATE 64

#define BASE_POINTS_PER_CERKEL 6
#define CERKEL_POINTS_STAGE_FACTOR 1

#define GS_TITLE 0
#define GS_OPTIONS 1
#define GS_GAMEPLAY 2
#define GS_GAMEOVER 3
#define GS_ABOUT 4

#define F_HYPERUNLOCKED 0

#define GMBOX_X (LCD_WIDTH/4)
#define GMBOX_Y (LCD_HEIGHT/2-LCD_HEIGHT/8)
#define GMBOX_W (LCD_WIDTH/2)
#define GMBOX_H (LCD_HEIGHT/4)

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External library headers */
#include <debug.h>
#include <intce.h>
#include <keypadc.h>
#include <graphx.h>
#include <decompress.h>
#include <fileioc.h>

typedef union fp16_8 {
	int fp;
	struct {
		uint8_t fpart;
		int16_t ipart;
	} p;
} fp16_8;
fp16_8 curx,cury,dx,dy,maxspeed,gravity,thrust;

/*---------------------------------------------------------------------------*/
/* Put your function prototypes here */
 
void vsync();
void putaway();
void waitanykey();
void keywait();
void centerstr(char* s, uint8_t y);

void redrawplayfield();
void placetarget();
void drawthickcircle(int x,int y,uint8_t r,uint8_t c);
void updatescore();
void erasepixel(fp16_8 x,fp16_8 y,int8_t a);
void drawpixel(fp16_8 x,fp16_8 y,int8_t angle);
void drawcorners(uint8_t gap);

void drawtitle();
void loadstage();
void flashscreen();
void sethighscore();
void longwait();

void move_and_draw_menuopts(char **optarr,uint8_t numopts,uint8_t *curopts);
void drawnotice(char **strings,uint8_t numlines);

/*---------------------------------------------------------------------------*/
/* Put all your globals here */
char* title1 = "Cerkel Snek";
char* menuopt[] = {"Start Game","Change Speed","About","Quit Game"};
char* menudiff[] = {"Slow","Medium","Fast","Hyper","Nope Rope"};
char* unlockedhyper[] = {"Congratulations!","You unlocked:","Hyper speed mode"};
char* oobrecovery[] = {"You know you're not","supposed to beat a","stage that way, right?"};

char* gameoverdesc[] = {"Lol u died","Git gud nub","U maed snek sad","Sr. Booplesnoot!"};
//[cos(a),sin(a-16)]
//This table is twice the size required to allow removal
//of extra bitmath during internal use.
int8_t tt[] =	{-127,-126,-124,-121,-117,-112,-105,-98,-89,-80,-70,-59,-48,-36,-24,-12,
				0,12,24,36,48,59,70,80,89,98,105,112,117,121,124,126,
				127,126,124,121,117,112,105,98,89,80,70,59,48,36,24,12,
				0,-12,-24,-36,-48,-59,-70,-80,-89,-98,-105,-112,-117,-121,-124,-126,
				-127,-126,-124,-121,-117,-112,-105,-98,-89,-80,-70,-59,-48,-36,-24,-12,
				0,12,24,36,48,59,70,80,89,98,105,112,117,121,124,126,
				127,126,124,121,117,112,105,98,89,80,70,59,48,36,24,12,
				0,-12,-24,-36,-48,-59,-70,-80,-89,-98,-105,-112,-117,-121,-124,-126};
int8_t* costab;
//
uint8_t graydient[] = {	0xB5,0x4A,0x4A,0xB5,
						0x4A,0x00,0x00,0x4A,
						0x4A,0x00,0x00,0x4A,
						0xB5,0x4A,0x4A,0xB5};
ti_var_t slot;

#define CREDITS_LENGTH 7
char* creditstrings[] = { 	"Copyright 2017 Rodger \"Iambian\" Weisman",
							"Released under the MIT License",
							"Source on GitHub:",
							"https://github.com/Iambian/CerkelSnek",
							"Game inspired from \"Uncle Worm\" by Badja",
							"Special thanks to:",
							"Tim \"Geekboy1011\" Keller", };
uint8_t creditypos[] = { 80,90,100,110,130,150,160 };
							


struct { int score[5]; uint8_t difficulty; uint8_t flags;} highscore;
char* highscorefile = "CERSNDAT";
//
int8_t trail[16384];
int8_t angle;
int trailstart,trailend;
fp16_8 headx,heady;
fp16_8 tailx,taily;
uint8_t growthlength;

int targetx,targety;
uint8_t targetr;

int unsigned curscore;

uint8_t curstage;
uint8_t circlesremain;



/*---------------------------------------------------------------------------*/
/* Program starts here */
void main(void) {
    uint8_t i,j,maintimer,gamestate,tempcolor;
	uint8_t menuoption;
	int8_t tempangle,tx,ty;
	int8_t tailangle;
	int a,b,c,z;
	fp16_8 x,y;
	kb_key_t k;
	void* ptr;
	uint8_t *bytearray;
	uint8_t *cptr;
	char* tmp_str;
	char* gameoverstr[2];
	/* Put in variable initialization here */
	gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	gfx_SetClipRegion(0,0,320,240);
	gfx_SetTextTransparentColor(0xFE);
	memset(&highscore,0,sizeof(highscore));
	ti_CloseAll();
	slot = ti_Open(highscorefile,"r");
	if (slot) {
		ti_Read(&highscore,sizeof(highscore),1,slot);
	}
	ti_CloseAll();
	costab = tt+32;  //magic
	gameoverstr[1] = "Game Over";
	/* Main Loop */
	menuoption = maintimer = gamestate = 0;
	while (1) {
		kb_Scan();
		switch(gamestate) {
			case GS_TITLE:
				k = kb_Data[1];
				if (k==kb_2nd) {
					keywait();
					switch(menuoption) {
						case 0:
							//init game state here
							gfx_SetDrawScreen();
							curscore = 0;
							curstage = 1;
							loadstage();
							gamestate = GS_GAMEPLAY;
							break;
						case 1:
							highscore.difficulty++;
							j = (highscore.flags&(1<<F_HYPERUNLOCKED)) ? 3 : 2;
							if (highscore.difficulty>j)	highscore.difficulty = 0;
							break;
						case 2:
							keywait();
							gamestate = GS_ABOUT;
							break;
						case 3:
							putaway();
							break;
						default:
							break;
					}
					break;
				}
				if (k==kb_Mode) putaway();
				if (k==kb_Yequ) highscore.difficulty = 4;
				drawtitle();
				move_and_draw_menuopts(menuopt,4,&menuoption);
				gfx_SetTextXY(5,230);
				gfx_PrintString("High score ( ");
				gfx_PrintString(menudiff[highscore.difficulty]);
				gfx_PrintString(" ) : ");
				gfx_PrintUInt(highscore.score[highscore.difficulty],5);
				gfx_SwapDraw();
				break;
				
			case GS_GAMEPLAY:
				a = 0x0001;
				switch(highscore.difficulty) {
					case 0: vsync();
					case 1: vsync();
					case 2: vsync();
					case 3: a+=0x0800;
					case 4: ;
					for(;a>0;a--);
				}
				k = kb_Data[1];
				if (k&kb_Mode) {
					keywait();
					gamestate = GS_TITLE;
					gfx_SetDrawBuffer();
					sethighscore();
					break;
				}
				k = kb_Data[7];
				if (k&kb_Left) angle = (angle-1)&0x3F;
				if (k&kb_Right) angle = (angle+1)&0x3F;
				
				for (i=2;i>0;i--) {
				
					//Update head/tail points
					trail[trailend] = angle;
					tx = costab[angle];
					ty = costab[angle-16];
					headx.fp += tx*2;
					heady.fp += ty*2;
					x.fp = headx.fp + tx*3;
					y.fp = heady.fp + ty*3;
					//dbg_sprintf(dbgout,"tx %i, ty %i, hx %i, hy %i, x %i, y %i\n",tx,ty,headx.p.ipart,heady.p.ipart,x.p.ipart,y.p.ipart);
					tempcolor = gfx_GetPixel(x.p.ipart,y.p.ipart);
					if (tempcolor == CERKEL_COLOR) {
						curscore += BASE_POINTS_PER_CERKEL+CERKEL_POINTS_STAGE_FACTOR*curstage;
						growthlength += SNAKE_GROWTH_RATE;
						drawthickcircle(targetx,targety,targetr,0xFF);
						if (!--circlesremain) {
							drawcorners(1);
						} else {
							placetarget();
						}
						updatescore();
					} else if (tempcolor == WINNER_COLOR) {
						curstage++;
						// TODO: Add advance to next stage notice
						loadstage();
						break;
					} else if (x.p.ipart<1 || y.p.ipart<1 || x.p.ipart>319 || y.p.ipart>239) {
						curstage++;
						loadstage();
						gfx_SetColor(CERKEL_COLOR);
						gfx_SetTextBGColor(CERKEL_COLOR);
						drawnotice(oobrecovery,3);
						gfx_SetTextBGColor(0xFF);
						break;
					}	else if (tempcolor != 0xFF) {
						gamestate = GS_GAMEOVER;
						break;
					}
					if (growthlength) growthlength--;
					tailangle = trail[trailstart];
					//Update table locations
					trailend = (trailend+1)&0x3FFF;
					if (!growthlength) trailstart = (trailstart+1)&0x3FFF;
					//Update pixels
					gfx_SetColor(0xFF);
					if (!growthlength) erasepixel(tailx,taily,tailangle); ///###
					if (!growthlength) {
						tailx.fp += costab[tailangle]*2;
						taily.fp += costab[tailangle-16]*2;
					}
//					if (!growthlength) erasepixel(tailx,taily,tailangle); ///###
					gfx_SetColor(SNAKE_COLOR);
					drawpixel(headx,heady,angle); ///###
				}
				break;
				
			case GS_GAMEOVER:
				sethighscore();
				gfx_BlitScreen();
				gfx_SetDrawBuffer();
				flashscreen();
				// bytearray = *gfx_vbuffer;
				// for(a=320*240;a>0;a--) {
					// *bytearray ^= *bytearray;
					// bytearray++;
				// }
				for(i=0;i<3;i++) {
					gfx_SwapDraw();
					for(a=0xF000;a>0;a--);
				}
				gfx_SetColor(DIALOG_BOX_COLOR);  //xlibc dark blue
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				gfx_SetTextBGColor(DIALOG_BOX_COLOR);
				gameoverstr[0] = gameoverdesc[randInt(0,3)];
				drawnotice(gameoverstr,2); //### RENDER GAME OVER BOX
				gfx_SwapDraw();
				
				waitanykey();
				if (!(highscore.flags&(1<<F_HYPERUNLOCKED)) && (highscore.difficulty == 2) && (curscore > 1000)) {
					gfx_BlitScreen();
					highscore.flags |= 1<<F_HYPERUNLOCKED;
					drawnotice(unlockedhyper,3);  //### RENDER HYPERSPEED UNLOCK
					gfx_SwapDraw();
					longwait();
					waitanykey();
				}
				gfx_SetTextBGColor(0xFF);
				gamestate = GS_TITLE;
				break;
				
			case GS_ABOUT:
				drawtitle();
				/* Print credits */
				for (i=0;i<CREDITS_LENGTH;i++) {
					gfx_PrintStringXY(creditstrings[i],5,creditypos[i]);
				}
				gfx_SwapDraw();
				waitanykey();
				gamestate = GS_TITLE;

			default:
				break;
		}
	}
	putaway();
}

/* Put other functions here */



/* Use this only if you're going to go the single buffer route */
void vsync() {
	asm("ld hl, 0E30000h +  0028h");    //interrupt clear register
	asm("set 3,(hl)");                  //clear vcomp interrupt (write 1)
	asm("ld l, 0020h");                 //interrupt raw register
	asm("_vsync_loop");
	asm("bit 3,(hl)");                  //Wait until 1 (interrupt triggered)
	asm("jr z,_vsync_loop");
	asm("ret");
}

void putaway() {
	gfx_End();
	slot = ti_Open(highscorefile,"w");
	ti_Write(&highscore,sizeof highscore, 1, slot);
	ti_CloseAll();
	exit(0);
}

void waitanykey() {
	keywait();            //wait until all keys are released
	while (!kb_AnyKey()); //wait until a key has been pressed.
	keywait();            //make sure key is released before advancing
}	

void keywait() {
	while (kb_AnyKey());  //wait until all keys are released
}

void centerstr(char* s, uint8_t y) {
	gfx_PrintStringXY(s,(LCD_WIDTH-gfx_GetStringWidth(s))>>1,y);
}


void redrawplayfield() {
	int a,b;
	fp16_8 x,y;
	int8_t tempangle;
	gfx_SetDrawScreen();
	//Prepare draw area
	gfx_FillScreen(0xFF);
	//Render worm. The &0x16383 keeps A in bounds no matter if the result
	//would be negative due to twos-compliment magic and discarding sign.
	gfx_SetColor(SNAKE_COLOR);
	x = headx;
	y = heady;
	//dbg_sprintf(dbgout,"A: %i, B: %i, X: &i, Y: %i\n",(trailend-trailstart)&0x3FFF,b=trailend-1,headx,heady);
	for(a=(trailend-trailstart)&0x3FFF,b=trailend-1;a>0;a--,b=(b-1)&0x3FFF) {
		tempangle = (trail[b]+32)&0x3F;  //draw snake backwards
		x.fp += costab[tempangle]*2;
		y.fp += costab[tempangle-16]*2;
		drawpixel(x,y,tempangle);
		//dbg_sprintf(dbgout,"X,Y,X2,Y2 %i,%i,%i,%i\n",headx.p.ipart,heady.p.ipart,x.p.ipart,y.p.ipart);
	}
	tailx = x;
	taily = y;
	placetarget();
	updatescore();
	drawcorners(0);
	vsync();
}

void placetarget() {
	int x,y,xc,yc,dx,dy,dist;
	int unsigned t,w,x1,x2;
	uint8_t r,acceptable;
	//Current window dimensions
	t = (curstage<20)?curstage:20;
	w = LCD_WIDTH-10*t;
	x1 = (LCD_WIDTH-w)>>1;
	x2 = x1+w;
	//Current snake head
	x = headx.p.ipart;
	y = heady.p.ipart;
	acceptable = 0;
	
	while (!acceptable) {
		r = randInt(10,40);
		xc = randInt(x1+r,x2-r);
		yc = randInt(WBORD_TOP+r,WINDOW_HEIGHT-r);
		if ((dx = x-xc)<0) dx = xc-x;
		if ((dy = y-yc)<0) dy = yc-y;
		if ((dx*dx+dy*dy)>(r*r*40)) {
			// This runs if the target is far enough away from the head
			drawthickcircle(xc,yc,r,CERKEL_COLOR);
			targetx = xc;
			targety = yc;
			targetr = r;
			acceptable = 1;
		}
	}
}

void updatescore() {
	gfx_SetTextScale(1,1);
	gfx_SetTextXY(1,1);
	gfx_PrintString("Score: ");
	gfx_PrintUInt(curscore,5);
}

void drawthickcircle(int x,int y,uint8_t r,uint8_t c) {
	uint8_t i;
	gfx_SetColor(c);
	for (i=0;i<3;i++,r--) {
		gfx_Circle_NoClip(x,y,r);
	}
}

#define SIZEOF_ERASEARRAY (12*2)
int8_t erasearray[] = {					 0,-2,
								-1,-1,	 0,-1,	 1,-1,
						-2, 0,	-1, 0,	 		 1, 0,	 2, 0,
								-1, 1,	 0, 1,	 1, 1,
										 0, 2,					};
										 
void erasepixel(fp16_8 x,fp16_8 y,int8_t a) {
	uint8_t i;
	gfx_SetColor(0xFF);
	for(i=0;i<SIZEOF_ERASEARRAY;i+=2) {
		gfx_SetPixel(x.p.ipart+erasearray[i+0],y.p.ipart+erasearray[i+1]);
	}
}

// Graydient is 0x00, 0x4A, 0xB5, 0xFF
void drawpixel(fp16_8 x,fp16_8 y,int8_t a) {
	uint8_t c,i,t,newc;
	fp16_8 xt,yt;
	gfx_SetColor(0x00);
	gfx_SetPixel(x.p.ipart,y.p.ipart);
	for (i=0;i<8;i++,a=(a+8)&0x3F) {
		xt.fp = x.fp + (costab[a+0 ]*2);
		yt.fp = y.fp + (costab[a-16]*2);
		c = gfx_GetPixel(xt.p.ipart,yt.p.ipart);
		t = (uint8_t) (((xt.p.fpart>>6)&0x03)|((yt.p.fpart>>4)&(0x03<<2)))&0x0F;
		t = graydient[t];
		//dbg_sprintf(dbgout,"CC %i, MathC %i\n",c,t);
		switch (c) {
			case 0x00:
			case 0x4A:
				newc = 0x00;
				break;
			case 0xB5:
				if (t==0xB5) { newc = 0x4A; }
				else { newc = 0x00;}
				break;
			case 0xFF:
				newc = t;
				break;
			default:
				newc = c;
				break;
		}
		gfx_SetColor(newc);
		gfx_SetPixel(xt.p.ipart,yt.p.ipart);
	}
}


void drawtitle() {
	gfx_FillScreen(0xFF);
	gfx_SetTextFGColor(0x00);
	gfx_SetTextScale(3,3);
	centerstr(title1,5);
	gfx_SetTextScale(1,1);
	gfx_PrintStringXY(VERSION_INFO,290,230);
}

int16_t walldimx[] =	{0,0,1,1,319,319,318,318,0,319,0,319,0,319,0,319};
int8_t walldimy[] =	{10,239,10,239,10,239,10,239,10,10,11,11,239,239,238,238};
int8_t wallwidth[] = {100,80,70,50,30,10};

void drawcorners(uint8_t gap) {
	uint8_t i;
	int unsigned x1,x2,t,w;
	gfx_SetColor(0x00);
	//Draw wall main borders
	for(i=0;i<16;i+=2) gfx_Line_NoClip(walldimx[i],walldimy[i],walldimx[i+1],walldimy[i+1]);
	
	t = (curstage<20)?curstage:20;
	w = LCD_WIDTH-10*t+3;
	x1 = (LCD_WIDTH-w)>>1;
	x2 = x1+w;
	
	gfx_Line_NoClip(x1,13,x1,236);  //Left side
	gfx_Line_NoClip(x2,13,x2,236);  //Right side
	gfx_Line_NoClip(x1,13,x2,13);   //Top side
	gfx_Line_NoClip(x1,236,x2,236); //Bottom side
	
	if (gap) {
		w = wallwidth[highscore.difficulty];
		x1 = (LCD_WIDTH-w)>>1;
		x2 = x1+w;
	gfx_SetColor(0xFF);
	gfx_Line_NoClip(x1,11,x2,11);      //top side
	gfx_Line_NoClip(x1,10,x2,10);      //top side
	gfx_SetColor(WINNER_COLOR);
	gfx_Line_NoClip(x1,12,x2,12);      //top side
	gfx_Line_NoClip(x1,13,x2,13);      //top side
	}
}

void loadstage() {
	trailstart = 0;
	trailend = 1;
	growthlength = 64;
	angle = 64-16;
	headx.fp = (LCD_WIDTH/2)<<8;
	heady.fp = (WINDOW_HEIGHT)<<8;
	memset(&trail,64-16,sizeof(trail));
	circlesremain = BASE_CIRCLES_PER_STAGE+curstage*ADDITIONAL_CIRCLES_PER_STAGE;
	redrawplayfield();
}

void flashscreen() {
	asm("ld bc,76800");
	asm("ld de,(0E30014h)");
	asm("_seizure_loop:");
	asm("ld a,(de)");
	asm("cpl");
	asm("ld (de),a");
	asm("inc de");
	asm("dec bc");
	asm("ld hl,000000h");
	asm("or a,a");
	asm("sbc hl,bc");
	asm("jr nz,_seizure_loop");
}

void sethighscore() {
	if (curscore > highscore.score[highscore.difficulty]) {
		highscore.score[highscore.difficulty] = curscore;
	}
}

void longwait() {
	int a;
	for(a=0x080000;a>0;a--);
}

void move_and_draw_menuopts(char **optarr,uint8_t numopts,uint8_t *curopt) {
	uint8_t i,y,k,n;
	k = kb_Data[7];
	i = *curopt;
	n = numopts-1;
	if (k&kb_Up && i) i--;
	if (k&kb_Down && i<n) i++;
	if (k&(kb_Up|kb_Down)) {
		keywait();
		*curopt = i;
	}
	y = (240-24*numopts)>>1;
	gfx_SetTextScale(2,2);
	for(i=0;i<numopts;i++,y+=24) {
		if (i == *curopt) gfx_SetTextFGColor(0x4F);
		centerstr(optarr[i],y);
		gfx_SetTextFGColor(0x00);
	}
	gfx_SetTextScale(1,1);
}

void drawnotice(char **strings,uint8_t numlines) {
	uint8_t gap,y,i;
	if (numlines==2) { y=GMBOX_Y+15; gap=20; }
	else             { y=GMBOX_Y+10; gap=15; }
	gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
	gfx_SetTextScale(1,1);
	for(i=0;i<numlines;i++,y+=gap) centerstr(strings[i],y);
}
