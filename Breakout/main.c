//
//  main.c
//  Breakout
//
//  Created by Thomas Foster on 8/9/18.
//  Copyright © 2018 Thomas Foster. All rights reserved.
//

#include <stdio.h>
#include "sdl.h"
#include "main.h"
#include "levels.h"

#define PADDLEHEIGHT	16
#define PADDLE_Y		SCREENHEIGHT-PADDLEHEIGHT*2

#define BALLSIZE		8 // it's 8!

#define BLOCKWIDTH		32
#define BLOCKHEIGHT		16

#define MINX			128
#define MAXX			SCREENWIDTH-MINX

typedef struct
{
	rect_t 	frame;
	int		dx;
} paddle_t;


typedef struct
{
	rect_t 	frame;
	int 	dx, dy;
} ball_t;


typedef struct
{
	rect_t 	frame;
	int		type;
} block_t;

#define MAXBLOCKS	ROWS*COLS

int 		ticks;
bool		started;
paddle_t	paddle;
ball_t		ball;
sizetype	ballsize;

int			numblocks;
int 		blocksleft;
block_t		blocks[MAXBLOCKS];

int 		score;
int 		lives;
int 		level;
bool		gameover;

// labels
texture_t	scoretitlelabel;
texture_t	scorelabel;
texture_t	livestitlelabel;
texture_t	liveslabel;
texture_t	gameoverlabel;

// sounds
Mix_Chunk	*hit;
Mix_Chunk	*sidehit;
Mix_Chunk	*lose;

void InitGame (void)
{
	ticks = 0;
	started = false;
	gameover = false;
	lives = 3;
	level = 1;
	ballsize = MakeSize(BALLSIZE, BALLSIZE);

	// position mid x
	paddle.frame.size.width = 64;
	paddle.frame.size.height = PADDLEHEIGHT; // constant
	paddle.frame.origin.x = SCREENWIDTH/2-paddle.frame.size.width/2;
	paddle.frame.origin.y = PADDLE_Y;
	paddle.dx = 0;
	
	// position on middle of paddle
	ball.frame.origin.x = paddle.frame.origin.x+paddle.frame.size.width/2;
	ball.frame.origin.y = PADDLE_Y-BALLSIZE;
	ball.frame.size = ballsize; // constant
	ball.dx = ball.dy = 0;

#define VOLUME 8
	sidehit = Mix_LoadWAV("tophit.wav");
	hit = Mix_LoadWAV("padhit.wav");
	lose = Mix_LoadWAV("goal.wav");
	if (!sidehit || !hit || !lose)
	{
		SDL_Quit();
		exit(1);
	}
	Mix_VolumeChunk(sidehit, VOLUME);
	Mix_VolumeChunk(hit, VOLUME);
	Mix_VolumeChunk(lose, VOLUME);

	scoretitlelabel = CreateText("Score", vgacolor[VGA_LBLUE]);
	livestitlelabel = CreateText("Lives", vgacolor[VGA_LBLUE]);
	gameoverlabel = CreateText("Game Over!", vgacolor[VGA_RED]);
}



void FreeAssets (void)
{
	SDL_DestroyTexture(scoretitlelabel.texture);
	SDL_DestroyTexture(scorelabel.texture);
}


void IncreaseScoreBy(int val)
{
	char text[32];

	score += val;
	sprintf(text, "%d",score);
	scorelabel = CreateText(text, vgacolor[VGA_LBLUE]);
}


void UpdateLives (void)
{
	char text[32];
	
	sprintf(text, "%d",lives);
	liveslabel = CreateText(text, vgacolor[VGA_LBLUE]);
}



//
//  LoadLevel
///	Load up blocks array from level's data
//
void LoadLevel (int num)
{
	int bindex;
	int row,col, i;
	byte *data;
	block_t	new;
	
	bindex = 0;
	memset(blocks, 0, sizeof(block_t)*MAXBLOCKS);

	switch (num) {
		case 1: data = &level1[0][0]; break;
		case 2: data = &level2[0][0]; break;
		default:
			break;
	}
	
	for (row=0; row<ROWS; row++) {
		for (col=0; col<COLS; col++, data++)
		{
			if (!*data)
				continue;
			
			new.type = *data;
			new.frame.origin.x = col*BLOCKWIDTH+128;
			new.frame.origin.y = row*BLOCKHEIGHT+64;
			new.frame.size.width = BLOCKWIDTH;
			new.frame.size.height = BLOCKHEIGHT;
		
			blocks[bindex++] = new;
		}
	}
	numblocks = blocksleft = bindex;
	
}




#pragma mark -
//
//  ProcessEvents
//
bool ProcessEvents(SDL_Event *ev)
{
	const unsigned char *state;

	while (SDL_PollEvent(ev))
	{
		if (ev->type == SDL_QUIT)
			return false;
		if (ev->type == SDL_KEYDOWN)
		{
			if (ev->key.keysym.sym == SDLK_SPACE) {
				if (!started) {
					started = true;
					ball.dx = 2;
					ball.dy = -4;
				}
			}
			if (ev->key.keysym.sym == SDLK_ESCAPE)
				return false;
		}
	}
	
	state = SDL_GetKeyboardState(NULL);
	
	paddle.dx = 0;
	if (state[SDL_SCANCODE_LEFT])
		paddle.dx = -8;
	else if (state[SDL_SCANCODE_RIGHT])
		paddle.dx = 8;

	return true;
}




void ResetBall (void)
{
	paddle.frame.size.width = 64;
	paddle.frame.size.height = PADDLEHEIGHT; // constant
	paddle.frame.origin.x = SCREENWIDTH/2-paddle.frame.size.width/2;
	paddle.frame.origin.y = PADDLE_Y;
	paddle.dx = 0;
	
	// position on middle of paddle
	ball.frame.origin.x = paddle.frame.origin.x+paddle.frame.size.width/2;
	ball.frame.origin.y = PADDLE_Y-BALLSIZE;
	ball.frame.size = ballsize; // constant
	ball.dx = ball.dy = 0;
	
	started = false;
}

void ProcessGame(void)
{
	int 	i, pwidth;
	rect_t	oldpos;
	box_t	blockbox, ballbox;
	point_t	*bpt, *ppt;
	
	
	paddle.frame.origin.x += paddle.dx;

	if (!started) {
		ball.frame.origin.x = paddle.frame.origin.x+paddle.frame.size.width/2;
		ball.frame.origin.y = PADDLE_Y-BALLSIZE;
		return;
	}
	
	ball.frame.origin.x += ball.dx;
	ball.frame.origin.y += ball.dy;

	// limit paddle movement
	if (paddle.frame.origin.x < 128)
		paddle.frame.origin.x = 128;
	if (paddle.frame.origin.x > 512-paddle.frame.size.width)
		paddle.frame.origin.x = 512-paddle.frame.size.width;

	
	// check for collision

	for (i=0; i<numblocks; i++)
	{
		if (blocks[i].type == 0)
			continue;

		TFBoxFromRect(&ballbox, &ball.frame);
		TFBoxFromRect(&blockbox, &blocks[i].frame);
		
		// vertically aligned
		if (ballbox.right > blockbox.left && ballbox.left < blockbox.right)
		{
			// hit from below
			if (ballbox.top < blockbox.bottom && ballbox.bottom > blockbox.bottom) {
				ball.frame.origin.y = blockbox.bottom;
				ball.dy = -ball.dy;
				IncreaseScoreBy(10*blocks[i].type);
				blocks[i].type--;
				blocksleft--;
				Mix_PlayChannel(-1, hit, 0);
			}
			// hit from above
			else if (ballbox.bottom > blockbox.top && ballbox.top < blockbox.top) {
				ball.frame.origin.y = blockbox.top-BALLSIZE;
				ball.dy = -ball.dy;
				IncreaseScoreBy(10*blocks[i].type);
				blocks[i].type--;
				blocksleft--;
				Mix_PlayChannel(-1, hit, 0);
			}
		}
		
		TFBoxFromRect(&ballbox, &ball.frame); // update

		// horizontally aligned
		if (ballbox.bottom > blockbox.top && ballbox.top < blockbox.bottom)
		{
			// hit from left
			if (ballbox.right > blockbox.left && ballbox.left < blockbox.left) {
				ball.frame.origin.x = blockbox.left-BALLSIZE;
				ball.dx = -ball.dx;
				IncreaseScoreBy(10*blocks[i].type);
				blocks[i].type--;
				blocksleft--;
				Mix_PlayChannel(-1, hit, 0);
			}
			// hit from right
			else if (ballbox.left < blockbox.right && ballbox.right > blockbox.right) {
				ball.frame.origin.x = blockbox.right;
				ball.dx = -ball.dx;
				IncreaseScoreBy(10*blocks[i].type);
				blocks[i].type--;
				blocksleft--;
				Mix_PlayChannel(-1, hit, 0);
			}
		}
	}

	for (i=0; i<numblocks; i++)
	{
		if (blocks[i].type > 0)
			break;
		
		if (level != MAXLEVEL)
			level++;
		SDL_Delay(1000);
		LoadLevel(level);
		ResetBall();
	}
	
	// check for paddle collision
	bpt = &ball.frame.origin;
	ppt = &paddle.frame.origin;
	pwidth = paddle.frame.size.width;
	if (bpt->y == PADDLE_Y-BALLSIZE)
	{
		if (bpt->x >= ppt->x-BALLSIZE && bpt->x < ppt->x+pwidth/2) {
			ball.dy = -ball.dy;
			ball.dx = -2;
			Mix_PlayChannel(-1, sidehit, 0);
		}
		if (bpt->x >= ppt->x+pwidth/2 && bpt->x <= ppt->x+pwidth) {
			ball.dy = -ball.dy;
			ball.dx = 2;
			Mix_PlayChannel(-1, sidehit, 0);
		}

	}
	
	// check if hit sides
	
	if (ball.frame.origin.x < MINX || ball.frame.origin.x+BALLSIZE > MAXX)
		ball.dx = -ball.dx;
	if (ball.frame.origin.y < 0)
		ball.dy = -ball.dy;
	if (ball.frame.origin.y > SCREENHEIGHT+BALLSIZE) {
		Mix_PlayChannel(-1, lose, 0);
		lives--;
		UpdateLives();
		SDL_Delay(500);
		if (lives == 0)
			gameover = true;
		else
			ResetBall();
	}

	
	
	
	
	
}


#pragma mark -

void RenderBlock(int index)
{
	SDL_Rect outer, inner;
	int		*type;
	point_t *origin;
	
	if (blocks[index].type == 0)
		return; // empty space
	
	type = &blocks[index].type;
	origin = &blocks[index].frame.origin;
	
	outer = SDLRect(origin->x, origin->y, BLOCKWIDTH, BLOCKHEIGHT);
	inner = SDLRect(outer.x+2, outer.y+2, outer.w-4, outer.h-4);
	
	if (blocks[index].type > 0)
	{
		SetDrawColor(vgacolor[VGA_GRAY]);
		SDL_RenderFillRect(renderer, &outer);
	}
	
	switch (*type) {
		case 1:
			SetDrawColor(vgacolor[VGA_GREEN]);
			break;
		case 2:
			SetDrawColor(vgacolor[VGA_YELLOW]);
			break;
		case 3:
			SetDrawColor(vgacolor[VGA_RED]);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			SetDrawColor(vgacolor[VGA_GRAY]);
			break;
		default:
			break;
	}

	if (blocks[index].type > 0)
		SDL_RenderFillRect(renderer, &inner);
}



void Display(void)
{
	SDL_Rect dest;
	int i;
	
	// CLEAR

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	
	// SIDES

	SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
	SDL_Rect left = { 0, 0, 128, SCREENHEIGHT};
	SDL_Rect right = { SCREENWIDTH-128, 0, 128, SCREENHEIGHT};
	SDL_RenderFillRect(renderer, &left);
	SDL_RenderFillRect(renderer, &right);

	
	// SCORE
	
	dest = SDLRect(MAXX, BLOCKHEIGHT, scoretitlelabel.size.width, scoretitlelabel.size.height);
	SDL_RenderCopy(renderer, scoretitlelabel.texture, NULL, &dest);
	dest = SDLRect(SCREENWIDTH-scorelabel.size.width, BLOCKHEIGHT, scorelabel.size.width, scorelabel.size.height);
	SDL_RenderCopy(renderer, scorelabel.texture, NULL, &dest);
	
	
	// LIVES
	
	dest = SDLRect(MAXX, BLOCKHEIGHT*2, scoretitlelabel.size.width, scoretitlelabel.size.height);
	SDL_RenderCopy(renderer, scoretitlelabel.texture, NULL, &dest);
	dest = SDLRect(SCREENWIDTH-liveslabel.size.width, BLOCKHEIGHT*2, liveslabel.size.width, liveslabel.size.height);
	SDL_RenderCopy(renderer, liveslabel.texture, NULL, &dest);
	
	
	// PADDLE
	
	SetDrawColor(vgacolor[VGA_GRAY]);
	dest = (SDL_Rect){ paddle.frame.origin.x, PADDLE_Y, paddle.frame.size.width, PADDLEHEIGHT };
	SDL_RenderFillRect(renderer, &dest);
	SetDrawColor(vgacolor[VGA_WHITE]);
	dest = SDLRect(dest.x+2, dest.y+2, dest.w-4, dest.h-4);
	SDL_RenderFillRect(renderer, &dest);
	
	// BALL
	
	SetDrawColor(vgacolor[VGA_GREEN]);
	dest = SDLRectFromPoint(ball.frame.origin, ballsize);
	SDL_RenderFillRect(renderer, &dest);
	
	
	// BLOCKS
	
	for (i=0; i<numblocks; i++)
		RenderBlock(i);
	
	
	// GAME OVER
	
	if (gameover)
	{
		dest = SDLRect(SCREENWIDTH/2-gameoverlabel.size.width/2,
					   240, gameoverlabel.size.width, gameoverlabel.size.height);
		SDL_RenderCopy(renderer, gameoverlabel.texture, NULL, &dest);
	}
	
	SDL_RenderPresent(renderer);
	if (gameover)
	{
		SDL_Delay(2000);
		FreeAssets();
		StopSDL();
		exit(0);
	}
}




#pragma mark -

int main(int argc, const char * argv[])
{
	SDL_Event	ev;
	
	StartSDL();
	InitGame();
	LoadLevel(1);
	IncreaseScoreBy(0);
	UpdateLives();
	
	while (1)
	{
		ticks++;
		
		if (!ProcessEvents(&ev))
			break;
		
		ProcessGame();
		Display();
		SDL_Delay(10);
	}

	FreeAssets();
	StopSDL();
	return 0;
}
