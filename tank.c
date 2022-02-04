#include<stdio.h>
#include<windows.h>
#include<stdint.h>
#include<math.h>
#include "tank.h"
/* Basic game engine, supports: Bitmap entity creation, can load custom image files, 2d rendering, image mirroring/reflecting,
image rotation (in progress), tile mapping, image transparency, rectangle collision, point collision, and "alpha layer collision"
(Should be more efficient, as instead of checking one point against multiple bitmaps, it only checks the alpha level of the pixels in front)
*/

#define BPP 32 //Bits per pixel 8 * 4
#define RES_WIDTH 1024
#define M_PI 3.14159265358979323846
#define WALL_LAYER 254

#define sign(x) ((x>0) - (x>0))

typedef struct BUFF
{
	BITMAPINFO binfo;
	int size;
	int height, width;
	void * Memory;
}BUFF;

struct player 
{
	Pixels pixels;
	Transform transform;
};
HWND HmainWnd;
HINSTANCE hinst;
BOOL running;
BUFF scrBuff, background;
Transform transforms[ENTITY_CAP];
Pixels pixels[ENTITY_CAP];
struct player player;
Pixels bullet;
UINT_PTR mainTmr;
int frameTimer;

//each tile should be 32x32 so 256/32 = 8 and 192 / 32 = 6
uint8_t tileMap[8*6] ={1,1,1,1,1,1,1,1,
						0,0,1,1,1,1,1,1,
						1,0,0,1,1,1,1,1,
						1,1,0,0,1,0,1,1,
						1,1,1,1,1,0,1,0,
						1,1,1,1,1,1,1,1};

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  
{ 

    switch (uMsg) 
    { 
        case WM_CLOSE:
        	running = FALSE;
        	PostQuitMessage(0);
        	break;
        default: 
        	return(DefWindowProcA(hwnd, uMsg, wParam, lParam));
        	break;
    } 
    return 0;
} 

void setChannel(Pixels * pixels, int index, uint8_t val)
{
	for(int i = 0; i < pixels->size; i+= 4)
	{
		uint8_t * ptr = (uint8_t *)pixels->memory + i * 4;
		ptr[index] = val;
	}
}

void BlitBmp(Pixels * src, BUFF * dest, int xloc, int yloc)
{
	int destLoc = ((dest->width * yloc) + xloc)*4;
	for(int y = 0; y < src->height; y++)
	{
		for(int x = 0; x < src->width; x++)
		{
			int index = ((y*src->width)+x)*4;
			uint8_t * cptr = (uint8_t *)src->memory + index;
			if(cptr[3] != 0)
			{
				uint8_t * mptr = (uint8_t *)dest->Memory + destLoc + ((dest->width * y) + x)*4; //inverse y to flip vertically, x horizontally? and replace y with x for horizontal directions
				mptr[0] = cptr[0];
				mptr[1] = cptr[1];
				mptr[2] = cptr[2];
				mptr[3] = cptr[3];
			}
		}
	}
}

//Just like BlitBmp but with image direction stuff, [!] may be slightly slower
//omnly use for player rn
void BlitBmpMirror(Pixels * bmp, int xloc, int yloc)
{
	int destLoc = ((scrBuff.width * yloc) + xloc)*4;
	for(int y = 0; y < bmp->height; y++)
	{
		for(int x = 0; x < bmp->width; x++)
		{
			int index = ((y*bmp->width)+x)*4;
			uint8_t * cptr = (uint8_t *)bmp->memory + index;
			if(cptr[3] != 0)
			{
				//up   = +y +x 0
				//left = +x -y 1
				//right= +x +y 2
				//down = -y +x 3
				//int part1 = (player.y_scale * y) + (player.x_scale * x); //warp tank v cool
				//int part2 = (player.y_scale * x) + (player.x_scale * y);
				int part1, part2;
				part1 = ((player.transform.direction == 1 || player.transform.direction == 3) * x) + ((player.transform.direction == 0) * y) + ((player.transform.direction == 2) * -y);
				part2 = ((player.transform.direction == 0 || player.transform.direction == 2) * x) + ((player.transform.direction == 3) * y) + ((player.transform.direction == 1) * -y);
				uint8_t * mptr = (uint8_t *)scrBuff.Memory + destLoc + ((scrBuff.width * part1) + part2)*4;
				mptr[0] = cptr[0];
				mptr[1] = cptr[1];
				mptr[2] = cptr[2];
			}
		}
	}
}


int simpleCollisionM(int x, int y, int EngtityType)
{
	//change to colliders eventually
	for(int i = 0; i <= EntityNum; i++)
	{
		if(transforms[i].entity_id == EngtityType)
		{
			if(x >= transforms[i].x && x < transforms[i].x + transforms[i].width && y >= transforms[i].y && y < transforms[i].y + transforms[i].height)
			{
				return i;
			}
		}
	}
	return 0;
}

static inline int pointCollision(int x, int y, Transform * obj)
{
	return(x >= obj->x && x <= obj->x + obj->width && y >= obj->y && y <= obj->y + obj->height);
}
int createEntity(uint8_t flags)
{
	if(EntityNum + 1 >= ENTITY_CAP)
	{
		printf("Too many entities in existence");
		return 0;
	}
	EntityNum += 1;
	transforms[EntityNum].entity_id = EntityNum * (flags & 1 != 0);
	pixels[EntityNum].entity_id = EntityNum * (flags & 2 != 0);
	return EntityNum;
}

static inline int tileCollision(int x, int y)
{
	return(tileMap[(y/32) * 8 + (x/32)]);
}



void pInput()
{
	BOOL rPressed = (GetAsyncKeyState(VK_RIGHT) != 0);
	BOOL lPressed = (GetAsyncKeyState(VK_LEFT) != 0);
	int8_t hspeed =  (rPressed - lPressed);
	BOOL uPressed = (GetAsyncKeyState(VK_UP) != 0);
	BOOL dPressed = (GetAsyncKeyState(VK_DOWN) != 0);
	int8_t vspeed = (uPressed - dPressed);
	if(vspeed || hspeed) player.transform.direction = (rPressed * 3) | (lPressed) | (dPressed * 2);
	if(GetAsyncKeyState(VK_SPACE))
	{
		int current = createEntity(3);
		pixels[current].memory = malloc(16);
		memset(pixels[current].memory, 255, pixels[current].size);
		pixels[current].width = pixels[current].height = 4;
		transforms[current].x = player.transform.x + player.transform.orx;
		transforms[current].y = player.transform.y + player.transform.ory;
		transforms[current].direction = player.transform.direction;
	}
	for(int i = 0; i < EntityNum; i++)
	{
		i += (transforms[i].entity_id == -1);
		transforms[i].y += 2 * ((transforms[i].direction == 0) - (transforms[i].direction == 2));
		transforms[i].x += 2 * ((transforms[i].direction == 3) - (transforms[i].direction == 1));
		if(transforms[i].y >= scrBuff.height || transforms[i].y <= 0) 
		{
			transforms[i].entity_id = -1;
			pixels[i].entity_id = -1;
		}
	}
	if(tileCollision(player.transform.x + player.transform.orx + 10 * hspeed, player.transform.y + player.transform.ory + 10 * vspeed) == 0)
	{
		hspeed = vspeed = 0;
	}

	player.transform.x += hspeed;
	player.transform.y += vspeed;
}

void renderGrphx()
{
    HDC hdc = GetDC(HmainWnd);
    memcpy(scrBuff.Memory, background.Memory, background.size);
	for(int i = 0; i <= EntityNum; i ++)
	{
		BlitBmp(&pixels[i], &scrBuff, transforms[i].x, transforms[i].y);
	}
	BlitBmpMirror(&player.pixels, player.transform.x + (player.pixels.width-1) * (player.transform.direction == 1), player.transform.y + (player.pixels.height-1) * (player.transform.direction == 2));
	StretchDIBits(hdc, 0, 0, 1024, 768, 0, 0, scrBuff.width, scrBuff.height, scrBuff.Memory, &scrBuff.binfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(HmainWnd, hdc);
}



Pixels loadBmp(char * str)
{
	FILE * f = fopen(str, "rb+");
	Pixels p;
	int width, height; //two int32s for width and height of bmp
	fread(&width, sizeof(int), 1, f); //read header
	fread(&height, sizeof(int), 1, f);
	p.height = height;
	p.width = width;
	p.size = width * height * 4;
	p.memory = malloc(p.size);
	fread(p.memory, sizeof(char), p.size, f); //read data
	fclose(f);
	return(p);
}

void setup()
{
	//Main Bmp initialisation
    //screen buffer
	scrBuff.width  = 256;
	scrBuff.height = 192;
	scrBuff.size = scrBuff.width * scrBuff.height * (BPP/8);
	scrBuff.binfo.bmiHeader.biSize = sizeof(scrBuff.binfo.bmiHeader);
	scrBuff.binfo.bmiHeader.biHeight = scrBuff.height;
	scrBuff.binfo.bmiHeader.biWidth = scrBuff.width;
	scrBuff.binfo.bmiHeader.biBitCount = BPP;
	scrBuff.binfo.bmiHeader.biCompression = BI_RGB;
	scrBuff.binfo.bmiHeader.biPlanes = 1;
	scrBuff.Memory = VirtualAlloc(NULL, scrBuff.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	//Background setup
    background.width = scrBuff.width;
    background.height = scrBuff.height;
    background.size = scrBuff.size;
    background.Memory = VirtualAlloc(NULL, background.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	//Player setup
	player.pixels = loadBmp("imgs/tank.bin");
	player.transform.x = player.transform.y = 120;
	player.transform.height = player.pixels.height;
	player.transform.width = player.pixels.width;
	player.transform.orx = player.transform.width / 2;
	player.transform.ory = player.transform.height / 2;



	//Set up tilemap
	Pixels tiles[] = {loadBmp("imgs/wall.bin"), loadBmp("imgs/grass.bin")};
	for(int i = 0; i < 8*6; ++i)
	{
		//Blitted to the background instead of scrBuff because to save resources
		BlitBmp(&tiles[tileMap[i]], &background, (i % 8) * 32, (i / 8) * 32);
	}
	//free the poor enslaved memory
	for(int i = 0; i < sizeof(tiles)/sizeof(tiles[0]); i++)
	{
		free(tiles[i].memory);
	}
	Pixels px = loadBmp("imgs/tree.bin");
	BlitBmp(&px, &background, 32, 130);
	BlitBmp(&px, &background, 200, 100);
	BlitBmp(&px, &background, 50, 50);
	free(px.memory);
	
	bullet.size = 16;
	bullet.width = bullet.height = 4;
	bullet.memory = malloc(bullet.size);
	memset(&bullet.memory, 255, bullet.size);

}

int APIENTRY WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, PSTR CommandLine, INT CmdShow)
{
	//Create MAIN WINDOW
	hinst = Instance;
	WNDCLASSEXA wc = {0};
	MSG msg;
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = 0;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(0,100,50));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Game";
	wc.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

	//Register Window
	if(!RegisterClassExA(&wc)) {
		MessageBox(NULL, "Window Registration Failed", "Error :(", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return 0;
	}

	//Create WINDOWs
	HmainWnd = CreateWindowExA(WS_EX_CLIENTEDGE,wc.lpszClassName,"Canvas", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
	CW_USEDEFAULT, 1024, 768, NULL, NULL, Instance, NULL);
	if(HmainWnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "ERROR :(", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return 0;
 	}

	setup();

	//Timer set up
	mainTmr = SetTimer(HmainWnd, 1, 1, NULL);

	//MESSAGE PIPING
	MSG Message = {0};
	running = TRUE;

	while(running) 
	{ 	
		while(PeekMessageA(&Message, HmainWnd, 0,0, PM_REMOVE)) {	
			DispatchMessageA(&Message);
		}
		//Main LOOP
		frameTimer = 0;

		//process input
		pInput();

		//Render
		renderGrphx();

		//Keep framerate at 60 fps
		Sleep((17 - frameTimer) * (frameTimer < 17));
	}
}