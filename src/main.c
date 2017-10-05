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
void erasepixel(int x,int y,int8_t a);
void ploterase(fp16_8* x, fp16_8* y,uint8_t a);
void drawpixel(int x,int y,int8_t angle);
void plotpixel(fp16_8* x, fp16_8* y,uint8_t a);
void drawcorners(uint8_t gap);

void drawtitle();
void loadstage();
void flashscreen();
void sethighscore();
void longwait();


/* Put all your globals here */
char* title1 = "Cerkel Snek";
char* menuopt[] = {"Start Game","Options","About","Quit Game"};
char* menudiff[] = {"Slow","Medium","Fast","Hyper","Nope Rope"};

char* gameoverdesc[] = {"Lol u died","Git gud nub","U maed snek sad","Sr. Booplesnoot!"};
//[cos(a),sin(a-16)]
int8_t costab[] = {127,126,124,121,117,112,105,98,89,80,70,59,48,36,24,12,
				0,-12,-24,-36,-48,-59,-70,-80,-89,-98,-105,-112,-117,-121,-124,-126,
				-127,-126,-124,-121,-117,-112,-105,-98,-89,-80,-70,-59,-48,-36,-24,-12,
				0,12,24,36,48,59,70,80,89,98,105,112,117,121,124,126};
//
uint8_t graydient[] = {	0xB5,0x4A,0x4A,0xB5,
						0x4A,0x00,0x00,0x4A,
						0x4A,0x00,0x00,0x4A,
						0xB5,0x4A,0x4A,0xB5};
ti_var_t slot;
struct { int score[5]; uint8_t difficulty; uint8_t flags;} highscore;
char* highscorefile = "CERSNDAT";
//
uint8_t menuoption;
// If somebody decided to TAS this game, this should theoretically contain the entire
// screen given play area 320x240 and circle radius is (64/pi) where 64 angles
// of unit 2 composes each circle. Calc round up, plus one to handle nibbling yourself.
// And bumped up to the nearest power of two to make us a circular buffer.
int8_t trail[16384];
int8_t angle;
int trailstart,trailend;
int headx,heady;
int tailx,taily;
uint8_t growthlength;

int targetx,targety;
uint8_t targetr;

int unsigned curscore;

uint8_t curstage;
uint8_t circlesremain;



/* Program starts here */
void main(void) {
    uint8_t i,j,maintimer,gamestate,tempcolor;
	int8_t tempangle;
	int8_t tailangle;
	int a,b,c,x,y,z;
	kb_key_t k;
	void* ptr;
	uint8_t *bytearray;
	uint8_t *cptr;
	int16_t *xs;
	int16_t *ys;
	int16_t *hxs;
	int16_t *hys;
	int16_t *txs;
	int16_t *tys;
	char* tmp_str;
	uint8_t frameskip = 0;
	cptr = (uint8_t*) &x; xs = (int16_t*) (cptr+1);
	cptr = (uint8_t*) &y; ys = (int16_t*) (cptr+1);
	cptr = (uint8_t*) &headx; hxs = (int16_t*) (cptr+1);
	cptr = (uint8_t*) &heady; hys = (int16_t*) (cptr+1);
	cptr = (uint8_t*) &tailx; txs = (int16_t*) (cptr+1);
	cptr = (uint8_t*) &taily; tys = (int16_t*) (cptr+1);
	
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
							gamestate = GS_OPTIONS;
							menuoption = 0;
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
				k = kb_Data[7];
				if (k&kb_Up && menuoption) menuoption--;
				if (k&kb_Down && menuoption<3) menuoption++;
				if (k&(kb_Up|kb_Down)) keywait();
				
				drawtitle();
				gfx_SetTextScale(2,2);
				for(i=0;i<4;i++) {
					if (menuoption==i) gfx_SetTextFGColor(0x4F);  // SELECTED OPT COLOR
					centerstr(menuopt[i],i*24+72);
					gfx_SetTextFGColor(0x00);
				}
				gfx_SetTextScale(1,1);
				gfx_SetTextXY(5,230);
				gfx_PrintString("High score ( ");
				gfx_PrintString(menudiff[highscore.difficulty]);
				gfx_PrintString(" ) : ");
				gfx_PrintUInt(highscore.score[highscore.difficulty],5);
				gfx_SwapDraw();
				break;
				
			case GS_OPTIONS:
				k = kb_Data[1];
				if (k==kb_Yequ) highscore.difficulty = 4;
				if (k==kb_2nd) {
					keywait();
					switch(menuoption) {
						case 0:
							highscore.difficulty++;
							j = (highscore.flags&(1<<F_HYPERUNLOCKED)) ? 3 : 2;
							if (highscore.difficulty>j)	highscore.difficulty = 0;
							break;
						case 1:
							gamestate = GS_TITLE;
							menuoption = 1;
							break;
						default:
							break;
					}
				}
				if (k==kb_Mode) {
					keywait();
					gamestate = GS_TITLE;
					break;
				}
				k = kb_Data[7];
				if (k&kb_Up && menuoption) menuoption--;
				if (k&kb_Down && menuoption<1) menuoption++;
				if (k&(kb_Up|kb_Down)) keywait();
				
				drawtitle();
				gfx_SetTextScale(2,2);			
				
				if (menuoption==0) gfx_SetTextFGColor(0x4F);
				centerstr("Change difficulty",(240-(2*24))/2+(0*24));
				gfx_SetTextFGColor(0x00);
				
				if (menuoption==1) gfx_SetTextFGColor(0x4F);
				centerstr("Go back to main menu",(240-(2*24))/2+(1*24));
				gfx_SetTextFGColor(0x00);
				
				gfx_SetTextScale(1,1);
				gfx_SetTextXY(5,230);
				gfx_PrintString("Current difficulty: ");
				gfx_PrintString(menudiff[highscore.difficulty]);
				
				gfx_SwapDraw();
				break;
				
			case GS_GAMEPLAY:
				i = 1;
				a = 0x0001;
				switch(highscore.difficulty) {
					case 0: vsync();vsync();break;
					case 1: vsync();break;
					case 2: a+=0x0800;
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
					x = headx + costab[angle]*2;
					y = heady + costab[(angle-16)&0x3F]*2;
					if (!( (*xs==*hxs) && (*ys==*hys) )) {
						//Do pixel-based collision detection here
						//Just a little further out to avoid self
						a = x + costab[angle]*3;
						b = y + costab[(angle-16)&0x3F]*3;
						tempcolor = gfx_GetPixel(a>>8,b>>8);
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
							//
							// Advance to next stage notice
							//
							loadstage();
							break;
						} else if (x<(1<<8) || y<(1<<8) || x>(319<<8) || y>(239<<8)) {
							curstage++;
							loadstage();
							gfx_SetColor(CERKEL_COLOR);
							gfx_SetTextBGColor(CERKEL_COLOR);
							gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
							centerstr("You know you're not",GMBOX_Y+10);
							centerstr("supposed to beat a",GMBOX_Y+25);
							centerstr("stage that way, right?",GMBOX_Y+40);
							gfx_SetTextBGColor(0xFF);
							break;
						}	else if (tempcolor != 0xFF) {
							gamestate = GS_GAMEOVER;
							break;
						}
					}
					if (growthlength) growthlength--;
					headx = x;
					heady = y;
					tailangle = trail[trailstart];
					//Update table locations
					trailend = (trailend+1)&0x3FFF;
					if (!growthlength) trailstart = (trailstart+1)&0x3FFF;
					//Update pixels
					gfx_SetColor(0xFF);
					if (!growthlength) erasepixel(tailx,taily,tailangle); ///###
					if (!growthlength) {
						tailx += costab[tailangle]*2;
						taily += costab[(tailangle-16)&0x3F]*2;
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
				gfx_SetTextScale(1,1);
				gfx_SetColor(DIALOG_BOX_COLOR);  //xlibc dark blue
				gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
				tmp_str = gameoverdesc[randInt(0,3)];
				gfx_GetStringWidth(tmp_str);
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				gfx_SetTextBGColor(DIALOG_BOX_COLOR);
				centerstr(tmp_str,GMBOX_Y+15);
				centerstr("Game over",GMBOX_Y+35);
				gfx_SwapDraw();
				waitanykey();
				if (!(highscore.flags&(1<<F_HYPERUNLOCKED)) && (highscore.difficulty == 2) && (curscore > 1000)) {
					gfx_BlitScreen();
					highscore.flags |= 1<<F_HYPERUNLOCKED;
					gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
					centerstr("Congratulations!",GMBOX_Y+10);
					centerstr("You unlocked:",GMBOX_Y+25);
					centerstr("Hyper speed mode",GMBOX_Y+40);
					gfx_SwapDraw();
					longwait();
					waitanykey();
				}
				gfx_SetTextBGColor(0xFF);
				gamestate = GS_TITLE;
				break;
				
			case GS_ABOUT:
				k = kb_Data[1];
				if (k) {
					keywait();
					gamestate = GS_TITLE;
					break;
				}
				drawtitle();
				gfx_PrintStringXY("Copyright 2017 Rodger \"Iambian\" Weisman",5,80);
				gfx_PrintStringXY("Released under the MIT License",5,90);
				gfx_PrintStringXY("Source on GitHub:",5,100);
				gfx_PrintStringXY("https://github.com/Iambian/CerkelSnek",5,110);
				gfx_PrintStringXY("Game inspired from \"Uncle Worm\" by Badja",5,130);
				gfx_PrintStringXY("Special thanks to:",5,150);
				gfx_PrintStringXY("Tim \"Geekboy1011\" Keller",5,160);
				gfx_SwapDraw();

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
	int x,y,a,b;
	int8_t tempangle=0;
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
		x += costab[tempangle]*2;
		y += costab[(tempangle-16)&0x3F]*2;
		drawpixel(x,y,tempangle);
		//dbg_sprintf(dbgout,"X,Y,X2,Y2 %i,%i,%i,%i\n",headx>>8,heady>>8,x>>8,y>>8);
	}
	tailx = x ;
	taily = y ;
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
	x = headx>>8;
	y = heady>>8;
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


void erasepixel(int x,int y,int8_t a) {
	uint8_t i;
	fp16_8 *xu = (fp16_8*) &x;
	fp16_8 *yu = (fp16_8*) &y;
//		ploterase(xu,yu,a);
		a += 16;
		x += costab[(a+0 )&0x3F];
		y += costab[(a-16)&0x3F];
		ploterase(xu,yu,a);
		a -= 32;
		x += costab[(a+0 )&0x3F];
		y += costab[(a-16)&0x3F];
		ploterase(xu,yu,a);
}

void ploterase(fp16_8* x, fp16_8* y,uint8_t a) {
	uint8_t c,i,t,newc;
	fp16_8 xs,ys;
	gfx_SetColor(0xFF);
	gfx_SetPixel(x->p.ipart,y->p.ipart);
	for (i=0;i<8;i++,a=(a+8)&0x3F) {
		xs.fp = x->fp + costab[(a+0 )&0x3F]*2;
		ys.fp = y->fp + costab[(a-16)&0x3F]*2;
		newc = 0xFF; //SAFETY DEBUG: ERASE ALL PIXELS. LATER, DO SUBTRACTIVE SHADING
		gfx_SetColor(newc);
		gfx_SetPixel(xs.p.ipart,ys.p.ipart);
	}
}

//This assumes that gfx_SetColor() is appropriately set before use
void drawpixel(int x,int y,int8_t a) {
	fp16_8 *xu = (fp16_8*) &x;
	fp16_8 *yu = (fp16_8*) &y;
	plotpixel(xu,yu,a);
}
// Graydient is 0x00, 0x4A, 0xB5, 0xFF
void plotpixel(fp16_8* x, fp16_8* y,uint8_t a) {
	uint8_t c,i,t,newc;
	fp16_8 xs,ys;
	gfx_SetColor(0x00);
	//gfx_SetPixel(x->p.ipart,y->p.ipart);
	for (i=0;i<8;i++,a=(a+8)&0x3F) {
		xs.fp = x->fp + costab[(a+0 )&0x3F];
		ys.fp = y->fp + costab[(a-16)&0x3F];
		c = gfx_GetPixel(xs.p.ipart,ys.p.ipart);
		t = (uint8_t) (((xs.p.fpart>>6)&0x03)|((ys.p.fpart>>4)&(0x03<<2)))&0x0F;
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
		gfx_SetPixel(xs.p.ipart,ys.p.ipart);
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

void drawcorners(uint8_t gap) {
	int unsigned x1,x2,w,t;
	gfx_SetColor(0x00);
	gfx_Line_NoClip(0,10,0,239);      //left side
	gfx_Line_NoClip(1,10,1,239);      //left side
	gfx_Line_NoClip(319,10,319,239);  //right side
	gfx_Line_NoClip(318,10,318,239); //right side

	gfx_Line_NoClip(0,10,319,10);      //top side
	gfx_Line_NoClip(0,11,319,11);      //top side
	gfx_Line_NoClip(0,239,319,239);  //bottom side
	gfx_Line_NoClip(0,238,319,238);  //bottom side
	
	t = (curstage<20)?curstage:20;
	w = LCD_WIDTH-10*t+3;
	x1 = (LCD_WIDTH-w)>>1;
	x2 = x1+w;
	
	gfx_Line_NoClip(x1,13,x1,236);  //Left side
	gfx_Line_NoClip(x2,13,x2,236);  //Right side
	gfx_Line_NoClip(x1,13,x2,13);   //Top side
	gfx_Line_NoClip(x1,236,x2,236); //Bottom side
	
	if (gap) {
		switch (highscore.difficulty) {
			case 0: w = 100; break;
			case 1: w = 80; break;
			case 2: w = 70; break;
			case 3: w = 50; break;
			default: w = 10; break;
		}
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
	headx = (LCD_WIDTH/2)<<8;
	heady = (WINDOW_HEIGHT)<<8;
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
