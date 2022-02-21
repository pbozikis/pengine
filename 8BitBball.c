#include<stdio.h>
#include<windows.h>
#include<stdint.h>
#include<math.h>

/* Basic game engine, supports: Bitmap entity creation, can load custom image files, 2d rendering, image mirroring/reflecting,
image rotation (in progress), tile mapping, image transparency, rectangle collision, point collision, and "alpha layer collision"
(Should be more efficient, as instead of checking one point against multiple bitmaps, it only checks the alpha level of the pixels in front)
*/



#define BPP 32 //Bits per pixel 8 * 4
#define RES_WIDTH 1024
#define M_PI 3.14159265358979323846
#define SIGN(x) ((x > 0) - (x < 0))

//Game type static don't use ECS
UINT_PTR mainTmr;

typedef struct BUFF
{
	BITMAPINFO binfo;
	int size;
	int height, width;
	void * Memory;
}BUFF;

HWND HmainWnd;
HINSTANCE hinst;
BOOL running;
BUFF scrBuff, background;
int frameTimer, whoHasBall;

typedef struct entity
{
    void * pixels;
    int size;
    int x, y;
    int width, height;
    int dir;
}Object;

Object p1, p2, ball, hoop1;
Object numbers[10];
int p1Score, p2Score;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  
{ 

    switch (uMsg) 
    { 
        case WM_CLOSE:
        	running = FALSE;
        	PostQuitMessage(0);
        	break;
		case WM_TIMER:
			frameTimer++;
			break;
        default: 
        	return(DefWindowProcA(hwnd, uMsg, wParam, lParam));
        	break;
    } 
    return 0;
} 


void BlitBmp(Object * src, BUFF * dest, int xloc, int yloc)
{
	int destLoc = ((dest->width * yloc) + xloc)*4;
	for(int y = 0; y < src->height; y++)
	{
		for(int x = 0; x < src->width; x++)
		{
			int index = ((y*src->width)+x)*4;
			uint8_t * cptr = (uint8_t *)src->pixels + index;
			if(cptr[3] != 0)
			{
				uint8_t * mptr = (uint8_t *)dest->Memory + destLoc + ((dest->width * y) + x * src->dir)*4; //inverse y to flip vertically, x horizontally? and replace y with x for horizontal directions
				mptr[0] = cptr[0];
				mptr[1] = cptr[1];
				mptr[2] = cptr[2];
				mptr[3] = cptr[3];
			}
		}
	}
}

void setChannel(Object * obj, int index, uint8_t val)
{
	for(int i = 0; i < obj->size; i+= 4)
	{
		uint8_t * ptr = (uint8_t *)obj->pixels + i * 4;
		ptr[index] = val;
	}
}

void setPixel(int x, int y, BUFF * bf, uint8_t col)
{
	uint8_t * ptr = (uint8_t *)bf->Memory;
	ptr[(x+(y*bf->width))*4 + 3] = col;
}

void setP(int x, int y, BUFF *bf, int col)
{
	int * ptr = (int *)bf->Memory;
	ptr[(x+(y*bf->width))] = col;
}

static inline int simpleCollision(int x, int y, Object * obj)
{
	return(x >= obj->x && x < obj->x + obj->width && y > obj->y && y < obj->y + obj->height);
}

BOOL alphaCollision(int x, int y, unsigned char val)
{
	uint8_t * ptr = (uint8_t *) scrBuff.Memory + (x + y * scrBuff.width) * 4;
	return(ptr[3] == val);
}



void pInput()
{
    static float holdPress1, holdPress2;
    static int bhspeed, bvspeed;
	int8_t p1Hspeed = ((GetAsyncKeyState('D') != 0) - (GetAsyncKeyState('A')!= 0));
	int8_t p2Hspeed = ((GetAsyncKeyState(VK_RIGHT) != 0) - (GetAsyncKeyState(VK_LEFT)!= 0));
    if(p1Hspeed != 0) p1.dir = p1Hspeed;
	if(p2Hspeed != 0) p2.dir = p2Hspeed;
    static int8_t p1Vspeed, p2Vspeed;
    if(GetAsyncKeyState('W') && p1.y == 20)
    {
        p1Vspeed = 8;
    }

	if(GetAsyncKeyState(VK_UP) && p2.y == 20)
	{
		p2Vspeed = 8;
	}

	holdPress2 += (GetAsyncKeyState('J') != 0) * 0.5;
    holdPress1 += (GetAsyncKeyState('F') != 0) * 0.5;
    if(holdPress1 != 0 && !GetAsyncKeyState('F'))
    {
        bhspeed = (holdPress1 * p1.dir) * 0.6;
        bvspeed = holdPress1;
        holdPress1 = 0;
        ball.x = p1.x + 20 * (p1.dir == 1);
        ball.y = p1.y + 20;
    }

	if(holdPress2 != 0 && !GetAsyncKeyState('J'))
	{
		bhspeed = (holdPress2 * p2.dir) * 0.6;
        bvspeed = holdPress2;
        holdPress2 = 0;
        ball.x = p2.x + 20 * (p2.dir == 1);
        ball.y = p2.y + 20;
	}
    p1Vspeed = (p1Vspeed - 1) * (p1.y > 20 + -p1Vspeed);
	p2Vspeed = (p2Vspeed - 1) * (p2.y > 20 + -p2Vspeed);
    p1.x += p1Hspeed * 2;
    p1.y += p1Vspeed * 2;
	p2.x += p2Hspeed * 2;
	p2.y += p2Vspeed * 2;

	//floor collision
	if(ball.y + bvspeed <= 15)
	{
		bvspeed = (-bvspeed - 2) *(ball.y >= 15);
	} else
	{
		//gravity
		bvspeed--;
	}

	//Border Collision
	if(ball.x + bhspeed + 8 >= scrBuff.width || ball.x + bhspeed <= 0 || simpleCollision(239, 88, &ball) || simpleCollision(250, 87, &ball) || simpleCollision(250, 100, &ball) || simpleCollision(250, 93, &ball))
	{
		bhspeed = -bhspeed + SIGN(bhspeed);
	}
	
	p1Score += (2 + (p1.x < 128)) * (simpleCollision(244, 88, &ball));
	p2Score += (2 + (p2.x >= 128))* (simpleCollision(12, 88, &ball));
	ball.x += bhspeed;
	ball.y += bvspeed;
	
}

void renderGrphx()
{
    HDC hdc = GetDC(HmainWnd);
    memcpy(scrBuff.Memory, background.Memory, background.size);
	BlitBmp(&p2, &scrBuff, p2.x + p2.width * (p2.dir == -1), p2.y);
    BlitBmp(&p1, &scrBuff, p1.x + p1.width * (p1.dir == -1), p1.y);
    BlitBmp(&ball, &scrBuff, ball.x, ball.y);
	BlitBmp(&numbers[p1Score % 9], &scrBuff, 50, scrBuff.height - 10);
    StretchDIBits(hdc, 0, 0, 1024, 768, 0, 0, scrBuff.width, scrBuff.height, scrBuff.Memory, &scrBuff.binfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(HmainWnd, hdc);
}




Object loadBmp(char * str)
{
	FILE * f = fopen(str, "rb+");
	Object p;
	int width, height; //two int32s for width and height of bmp
	fread(&width, sizeof(int), 1, f); //read header
	fread(&height, sizeof(int), 1, f);
	p.height = height;
	p.width = width;
	p.size = width * height * 4;
	p.pixels = malloc(p.size);
    p.dir = 1;
	fread(p.pixels, sizeof(char), p.size, f); //read data
	fclose(f);
	return(p);
}


void setup()
{
	//Main Bmp initialisation
    //screen buffer
	//This is the buffer that will be displayed
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
	int * bptr = (int *)background.Memory;
	for(int i = 0; i < background.size/ 4; ++i)
	{
		bptr[i] = 0xff00ffff;
	}

	//Object setUp
    p1 = loadBmp("imgs/kobe.bin");
    p1.x = p1.y = 20;
	p2 = loadBmp("imgs/giannis.bin");
	p2.x = p2.y = 25;
    ball = loadBmp("imgs/ball2.bin");
    ball.x = ball.y = 55;


    Object floor = loadBmp("imgs/floor.bin");
    for(int x = 0; x < 8; x++)
    {
        BlitBmp(&floor, &background, x * 32, 0);
    }
    free(floor.pixels);

	hoop1 = loadBmp("imgs/hoop.bin");
	hoop1.x = scrBuff.width - 32;
	hoop1.y = 72;
	BlitBmp(&hoop1, &background, hoop1.x, hoop1.y);
	free(hoop1.pixels);
	for(int x = 1; x < 3; x++)
	{
		for(int y = 32; y < 72; y++)
		{
			uint8_t * ptr = (uint8_t *)background.Memory + (y * background.width + background.width -x)*4;
			ptr[0] = ptr[1] = ptr[2] = 0;
		}
	}

	numbers[0] = loadBmp("imgs/0.bin");
	numbers[1] = loadBmp("imgs/1.bin");
	numbers[2] = loadBmp("imgs/2.bin");
	numbers[3] = loadBmp("imgs/3.bin");
	numbers[4] = loadBmp("imgs/4.bin");
	numbers[5] = loadBmp("imgs/5.bin");
	numbers[6] = loadBmp("imgs/6.bin");
	numbers[7] = loadBmp("imgs/7.bin");
	numbers[8] = loadBmp("imgs/8.bin");
	numbers[9] = loadBmp("imgs/9.bin");


	
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
	CW_USEDEFAULT, 1044, 810, NULL, NULL, Instance, NULL);
	if(HmainWnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "ERROR :(", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return 0;
 	}

	setup();
	//MESSAGE PIPING
	MSG Message = {0};
	running = TRUE;
	
	//Timer set up
	mainTmr = SetTimer(HmainWnd, 1, 1, NULL);

	while(running) 
	{ 	
		while(PeekMessageA(&Message, HmainWnd, 0,0, PM_REMOVE)) {	
			DispatchMessageA(&Message);
		}
		//Main LOOP
		//start finding elapsed ms
		frameTimer = 0;
		//process input
		pInput();
		//Render
		renderGrphx();
		if(frameTimer < 17)
		{
			Sleep(17 - frameTimer);
		}
	}
}
