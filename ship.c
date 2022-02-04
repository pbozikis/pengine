#include<stdio.h>
#include<windows.h>
#include<stdint.h>
#include<math.h>
#define ENTITY_CAP 10
#define M_PI 3.14159265358979323846
int EntityNum = 0;

typedef struct transform
{
	int entity_id;
	int x, y;
	int orx, ory;
	int width, height;
	int rotation;
}Transform;

typedef struct bin
{
	int entity_id;
	int size;
	int width, height;
	void * memory;
}Pixels;

#define BPP 32 //Bits per pixel 8 * 4
#define RES_WIDTH 1024
#define M_PI 3.14159265358979323846
#define Deg2Rad(d) (d * (M_PI / 180))
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
UINT_PTR mainTmr;
struct player ship;
int frameTimer;

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

//Sets a pixels of the screen buffer
//Inline means the compiler essentially copy and pastes the contents of the function where it's called
//...to avoid function overheard which is slow
static inline void setPixel(int x, int y, int col)
{
	//Parse the void pointer to an int pointer then deference it like an array
	((int *)scrBuff.Memory)[x + y * scrBuff.width] = col;
}

//Bresenham line algorithm
//Bresenham is a cool guy probs
void bLine(int x1, int y1, int x2, int y2)
{
   int m_new = 2 * (y2 - y1);
   int slope_error_new = m_new - (x2 - x1);
   for (int x = x1, y = y1; x <= x2; x++)
   {
      setPixel(x, y, 0xff000000);
      slope_error_new += m_new;
  
      if (slope_error_new >= 0)
      {
        y++;
        slope_error_new  -= 2 * (x2 - x1);
      }
   }
}


//Draws a bitmap to the destination buffer
void BlitBmp(Pixels * src, BUFF * dest, int xloc, int yloc)
{
	int destLoc = ((dest->width * yloc) + xloc)*4;
	for(int y = 0; y < src->height; y++)
	{
		for(int x = 0; x < src->width; x++)
		{
			int index = ((y*src->width)+x)*4;
			//This pointer points to the memory of the image, at the index of the current pixel
			uint8_t * cptr = (uint8_t *)src->memory + index;
			if(cptr[3] != 0) //Checks if the alpha channel is not transparent
			{
				//Could do this in one line with an int * instead ah well
				uint8_t * mptr = (uint8_t *)dest->Memory + destLoc + ((dest->width * y) + x)*4; //inverse y to flip vertically, x horizontally? and replace y with x for horizontal directions
				mptr[0] = cptr[0];
				mptr[1] = cptr[1];
				mptr[2] = cptr[2];
				mptr[3] = cptr[3];
			}
		}
	}
}

//Checks collision between a point and an object
static inline int pointCollision(int x, int y, Transform * obj)
{
	return(x >= obj->x && x <= obj->x + obj->width && y >= obj->y && y <= obj->y + obj->height);
}

//ECS function, not used here and not finished
int createEntity(uint8_t flags)
{
	if(EntityNum + 1 >= ENTITY_CAP)
	{
		return 0;
	}
	EntityNum += 1;
	transforms[EntityNum].entity_id = EntityNum * (flags & 1 != 0);
	pixels[EntityNum].entity_id = EntityNum * (flags & 2 != 0);
	return EntityNum;
}

//Draws the bitmap to the screen with rotation, it's a workaround
void rotBlit(Pixels * bmp, int xx, int yy, int degrees)
{
	float radians = Deg2Rad(degrees);
	float cosr = cos(radians);
	float sinr = sin(radians);
	int cx = bmp->width / 2;
	int cy = bmp->height / 2;
	int * ptr = (int *)scrBuff.Memory + xx + yy * scrBuff.width;
	for(int y = 0; y < bmp->height; y++)
	{
		for(int x = 0; x < bmp->width; x++)
		{
			int nx =  cosr * (x - cx) + sinr * (y - cy);
			int ny = -sinr * (x - cx) + cosr * (y - cy);
			//if(nx >= 0 && nx < bmp->width && ny >= 0 && ny < bmp->height)
			unsigned int pix = ((unsigned int *)bmp->memory)[(nx+cx)+ (ny+cy) * bmp->width];
			if((pix >> 24) & 0xff) //Check transparency
			ptr[(x) + (y) * scrBuff.width] = pix;

		}
	}
}


void pInput()
{
	static float hspeed, vspeed;
	ship.transform.rotation += 4*((GetAsyncKeyState(VK_LEFT) != 0) - (GetAsyncKeyState(VK_RIGHT) != 0));
	if(GetAsyncKeyState(VK_UP))
	{
		float theta = Deg2Rad(ship.transform.rotation);
		hspeed += cos(theta) / 2;
		vspeed += sin(theta) / 2;
	}
	ship.transform.x += hspeed;
	ship.transform.y += vspeed;
}

void renderGrphx()
{
    HDC hdc = GetDC(HmainWnd); //Device context
    memcpy(scrBuff.Memory, background.Memory, background.size);
	rotBlit(&ship.pixels, ship.transform.x, ship.transform.y, ship.transform.rotation);
	//This function below is the windows function that actually draws the screen
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
	scrBuff.width  = 512;
	scrBuff.height = 384;
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
	ship.pixels = loadBmp("imgs/shyp.bin");
	ship.transform.x = ship.transform.y = 120;
	ship.transform.height = ship.pixels.height;
	ship.transform.width = ship.pixels.width;
	ship.transform.orx = ship.transform.width / 2;
	ship.transform.ory = ship.transform.height / 2;

	//draw stars
	srand(123);
	for(int i = 0; i < 100; i++)
		((int *)background.Memory)[rand() % scrBuff.width + (rand() % scrBuff.height) * scrBuff.width] = 0xffffffff;
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
		
	//Main LOOP
	while(running) 
	{ 	
		while(PeekMessageA(&Message, HmainWnd, 0,0, PM_REMOVE)) {	
			DispatchMessageA(&Message);
		}
		frameTimer = 0;
		//process input
		pInput();
		//Render
		renderGrphx();

		//Make sure it stays at 60 fps
		if(frameTimer < 17)	Sleep(17 - frameTimer);
	}
}