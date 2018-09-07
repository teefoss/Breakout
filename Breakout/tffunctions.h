//
//  tffunctions.h
//  Pong
//
//  Created by Thomas Foster on 7/27/18.
//  Copyright © 2018 Thomas Foster. All rights reserved.
//

#ifndef tffunctions_h
#define tffunctions_h

#include <stdio.h>
#include <SDL2/SDL.h>

typedef unsigned char byte;
typedef enum { false, true } bool;

typedef enum { none, horizontal, vertical } alignment_t;
typedef enum { na, above, below, rightside, leftside } position_t;

typedef struct
{
	int width, height;
} sizetype;

typedef struct
{
	int x,y;
} point_t;

typedef struct
{
	point_t origin;
	sizetype size;
} rect_t;

typedef struct
{
	int right, left, top, bottom;
} box_t;


point_t 	MakePoint(int x, int y);
sizetype	MakeSize(int w, int h);
rect_t 		MakeRect(int x, int y, int w, int h);
rect_t MakeRectFromPoint(point_t pt, sizetype size);

void 		TFRectToSDL(SDL_Rect *sdlrect, rect_t *tfrect);
SDL_Rect 	SDLRect(int x, int y, int w, int h);

void 	TFBoxFromRect(box_t *destbox, rect_t const *srcrect);
bool 	TFPointsAreEqual(point_t *pt1, point_t *pt2);

bool 	TFRectsCollide(rect_t *aRect, rect_t *bRect);
bool 	TFRectsCollide2(rect_t *aRect, rect_t *bRect);

alignment_t TFRectAlignment(rect_t *aRect, rect_t *bRect);
position_t TFRectPosition(rect_t *aRect, rect_t *bRect);

#endif /* tffunctions_h */
