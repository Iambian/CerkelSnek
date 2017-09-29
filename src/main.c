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
#define BULLET_TABLE_SIZE 512
#define ENEMY_TABLE_SIZE 32

#define GS_TITLE 0
#define GS_OPTIONS 1
#define GS_GAMEPLAY 2



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



/* Put all your globals here */
char* title1 = "Cerkel Snek";
char* menuopt[] = {"Start Game","Options","Quit Game"};
char* menudiff[] = {"Easy","Medium","Hard","Hyper"};

ti_var_t slot;
struct { int score[4]; uint8_t difficulty; uint8_t flags;} highscore;
char* highscorefile = "CERSNDAT";

uint8_t menuoption;




void main(void) {
    uint8_t i,j,maintimer,gamestate;
	int a,b,c,x,y,z;
	kb_key_t k;
	void* ptr;
	/* Put in variable initialization here */
	gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	gfx_SetClipRegion(0,0,320,240);
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
					if (menuoption == 0) {
						// Init game state
						gamestate = GS_GAMEPLAY;
						break;
					}
					if (menuoption == 2) putaway();
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
				break;
			case GS_GAMEPLAY:
				putaway();
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


