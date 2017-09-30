/*
 *--------------------------------------
 * Program Name: Cerkel Snek
 * Author:
 * License:
 * Description:
 *--------------------------------------
*/
#define VERSION_INFO "v0.1"

#define TRANSPARENT_COLOR 0xF8
#define GREETINGS_DIALOG_TEXT_COLOR 0xDF

#define SNAKE_COLOR 0x62
#define CERKEL_COLOR 0xE0

#define WINDOW_BORDER 2
#define WINDOW_HEIGHT (LCD_HEIGHT-10-(WINDOW_BORDER*2))
#define WINDOW_WIDTH (LCD_WIDTH-(WINDOW_BORDER*2))

#define GS_TITLE 0
#define GS_OPTIONS 1
#define GS_GAMEPLAY 2
#define GS_GAMEOVER 3

#define F_HYPERUNLOCKED 0



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



/* Put all your globals here */
char* title1 = "Cerkel Snek";
char* menuopt[] = {"Start Game","Options","Quit Game"};
char* menudiff[] = {"Easy","Medium","Hard","Hyper"};
//[cos(a),sin(a-16)]
int8_t costab[] = {127,126,124,121,117,112,105,98,89,80,70,59,48,36,24,12,
				0,-12,-24,-36,-48,-59,-70,-80,-89,-98,-105,-112,-117,-121,-124,-126,
				-127,-126,-124,-121,-117,-112,-105,-98,-89,-80,-70,-59,-48,-36,-24,-12,
				0,12,24,36,48,59,70,80,89,98,105,112,117,121,124,126};
//
ti_var_t slot;
struct { int score[4]; uint8_t difficulty; uint8_t flags;} highscore;
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



/* Program starts here */
void main(void) {
    uint8_t i,j,maintimer,gamestate,tempcolor;
	int8_t tempangle;
	int a,b,c,x,y,z;
	kb_key_t k;
	void* ptr;
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
							curscore = 0;
							trailstart = 0;
							trailend = 16;
							growthlength = 128;
							angle = 0;
							headx = (32)<<8;
							heady = (LCD_HEIGHT/2)<<8;
							memset(&trail,0,sizeof(trail));
							gamestate = GS_GAMEPLAY;
							gfx_FillScreen(0xFF);
							gfx_SwapDraw();
							redrawplayfield();
							break;
						case 1:
							gamestate = GS_OPTIONS;
							menuoption = 0;
							break;
						case 2:
							putaway();
							break;
						default:
							break;
					}
				}
				if (k==kb_Mode) putaway();
				k = kb_Data[7];
				if (k&kb_Up && menuoption) menuoption--;
				if (k&kb_Down && menuoption<2) menuoption++;
				if (k&(kb_Up|kb_Down)) keywait();
				
				gfx_FillScreen(0xFF);
				gfx_SetTextFGColor(0x00);
				gfx_SetTextScale(3,3);
				centerstr(title1,5);
				gfx_SetTextScale(2,2);
				for(i=0;i<3;i++) {
					if (menuoption==i) gfx_SetTextFGColor(0x4F);  // SELECTED OPT COLOR
					centerstr(menuopt[i],i*24+84);
					gfx_SetTextFGColor(0x00);
				}
				gfx_SetTextScale(1,1);
				gfx_SetTextXY(5,230);
				gfx_PrintString("High score ( ");
				gfx_PrintString(menudiff[highscore.difficulty]);
				gfx_PrintString(" ) : ");
				gfx_PrintUInt(highscore.score[highscore.difficulty],5);
				gfx_PrintStringXY(VERSION_INFO,290,230);
				gfx_SwapDraw();
				break;
				
			case GS_OPTIONS:
				k = kb_Data[1];
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
				if (k==kb_Mode) gamestate = GS_TITLE;
				k = kb_Data[7];
				if (k&kb_Up && menuoption) menuoption--;
				if (k&kb_Down && menuoption<1) menuoption++;
				if (k&(kb_Up|kb_Down)) keywait();
				
				gfx_FillScreen(0xFF);
				gfx_SetTextFGColor(0x00);
				gfx_SetTextScale(3,3);
				centerstr(title1,5);
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
				vsync();
				gfx_BlitBuffer();
				
				k = kb_Data[1];
				if (k&kb_Mode) {
					keywait();
					gamestate = GS_TITLE;
					break;
				}
				k = kb_Data[7];
				if (k&kb_Left) angle = (angle-1)&0x3F;
				if (k&kb_Right) angle = (angle+1)&0x3F;
				
				
				//Update head/tail points
				trail[trailend] = angle;
				x = headx + costab[angle];
				y = heady + costab[(angle-16)&0x3F];
				if (!( ((x>>8)==(headx>>8)) && ((y>>8)==(heady>>8)) )) {
					//Do pixel-based collision detection here
					tempcolor = gfx_GetPixel(x>>8,y>>8);
					if (tempcolor == CERKEL_COLOR) {
						curscore += 7;
						growthlength += 16;
						drawthickcircle(targetx,targety,targetr,0xFF);
						placetarget();
						updatescore();
					} else if (tempcolor != 0xFF) {
						gamestate = GS_GAMEOVER;
						break;
					}
				}
				if (growthlength) growthlength--;
				headx = x;
				heady = y;
				j = trail[trailstart];
				if (!growthlength) {
					tailx += costab[j];
					taily += costab[(j-16)&0x3F];
				}

				//Update table locations
				trailend = (trailend+1)&0x3FFF;
				if (!growthlength) trailstart = (trailstart+1)&0x3FFF;
				//Update pixels
				gfx_SetColor(0xFF);
				if (!growthlength) gfx_SetPixel(tailx>>8,taily>>8);
				gfx_SetColor(SNAKE_COLOR);
				gfx_SetPixel(headx>>8,heady>>8);
				break;
				
			case GS_GAMEOVER:
				keywait();
				gamestate = GS_TITLE;
				break;
				
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
	while (kb_AnyKey());  //make sure key is released before advancing
}	

void keywait() {
	while (kb_AnyKey());  //wait until all keys are released
}

void centerstr(char* s, uint8_t y) {
	gfx_PrintStringXY(s,(LCD_WIDTH-gfx_GetStringWidth(s))>>1,y);
}


void redrawplayfield() {
	int x,y,a,b;
	int8_t tempangle;
	//Prepare draw area
	gfx_FillScreen(0xFF);
	//Render worm. The &0x16383 keeps A in bounds no matter if the result
	//would be negative due to twos-compliment magic and discarding sign.
	gfx_SetColor(SNAKE_COLOR);
	x = headx;
	y = heady;
	gfx_SetPixel(x>>8,y>>8);
	//dbg_sprintf(dbgout,"A: %i, B: %i, X: &i, Y: %i\n",(trailend-trailstart)&0x3FFF,b=trailend-1,headx,heady);
	for(a=(trailend-trailstart)&0x3FFF,b=trailend-1;a>0;a--,b=(b-1)&0x3FFF) {
		tempangle = (trail[b]+32)&0x3F;  //draw snake backwards
		x += costab[tempangle];
		y += costab[(tempangle-16)&0x3F];
		//dbg_sprintf(dbgout,"X,Y,X2,Y2 %i,%i,%i,%i\n",headx>>8,heady>>8,x>>8,y>>8);
		gfx_SetPixel(x>>8,y>>8);
	}
	tailx = x;
	taily = y;
	placetarget();
	updatescore();
	vsync();
	gfx_BlitBuffer();
}

void placetarget() {
	int x,y,xc,yc,dx,dy,dist;
	uint8_t r,acceptable;
	x = headx>>8;
	y = heady>>8;
	acceptable = 0;
	
	while (!acceptable) {
		r = randInt(10,40);
		xc = randInt(0+r,LCD_WIDTH-r);
		yc = randInt(0+r,LCD_HEIGHT-r);
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


