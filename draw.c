#include<stdio.h> //for input output/ file tings
#define BPP 32 //Bits per pixel 8 * 4
#include<windows.h> //for the GUI
#include<stdint.h> //for the uint8_t stuff
#include<string.h> //I forgot
#include<CommCtrl.h> //For the trackbars

/*This is the drawing tool, it can create, save and edit custom bin file images which can be exported into the game engine and used there
cannot resize the canvas yet, u have to recompile it*/

//Type the below command in command prompt to compile it
//gcc -o draw.exe draw.c -l gdi32 -l comdlg32 


#define ID_TRACK 1002


//Draw states
#define NOTDRAWING 0
#define DRAWING 1
#define GETTING 2
#define FLOODFILL 3

static inline void setPixelPrimitive(int x, int y, int col);
int getPixelPrimitive(int x, int y);
void loadBmp(char * str);

//Setting default file, may throw error
char szfile[100] = "imgs/img.bin";
OPENFILENAME ofc = {0};

//Data structure for Bitmap
typedef struct bmp
{
    BITMAPINFO binfo; //necessary for windows function: StretchDIBits()
    int origin_x, origin_y;
	int size;
    int height_actual, width_actual; //Destination size on screen (stretched)
    int height_pixels, width_pixels; //Size it actually is stored in (compressed)
    void *Memory; //pointer to the first pixel byte
}Bmp;



HWND HmainWnd, trkRed, trkGreen, trkBlue, trkAlpha;
//Hwnd is a window handler, everything in windows is apparently a window
HINSTANCE hinst; //Idk what the use of this is
BOOL running;
Bmp mainBmp, pallette;
int drawMode, cIndex;
unsigned int currentColour = 0xff000000; //colour: black


//creates a new bmp structure
Bmp newBmp(int orx, int ory, int actualH, int actualW, int pixelW, int pixelH)
{
	Bmp nbmp;
  	nbmp.origin_x = orx;
	nbmp.origin_y = ory;
	nbmp.width_actual = actualW;
	nbmp.height_actual = actualH;
	nbmp.width_pixels = pixelW;
	nbmp.height_pixels = pixelH;
    //size of bitmap in bytes (each byte is a colour channel RGBA)
	nbmp.size = nbmp.width_pixels * nbmp.height_pixels * (BPP/8);
	nbmp.binfo.bmiHeader.biSize = sizeof(nbmp.binfo.bmiHeader);
	nbmp.binfo.bmiHeader.biHeight = nbmp.height_pixels;
	nbmp.binfo.bmiHeader.biWidth = nbmp.width_pixels;
	nbmp.binfo.bmiHeader.biBitCount = BPP;
	nbmp.binfo.bmiHeader.biCompression = BI_RGB;
	nbmp.binfo.bmiHeader.biPlanes = 1;
    //Allocate memory
	nbmp.Memory = VirtualAlloc(NULL, nbmp.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	if(nbmp.Memory == NULL)
		MessageBox(NULL, "Failed to allocate enough memory for Bmp", "Error :(", MB_ICONEXCLAMATION |MB_OK);
	return(nbmp);
}

static inline void setPixelPrimitive(int x, int y, int col)
{
    //turn x and y coords into a linear index
	int index = (x+(y*mainBmp.width_pixels));
    if(index < mainBmp.size - 1 && index >= 0)
    {
        //I should not be using memcpy for this btw
       // memcpy((char*)mainBmp.Memory + index, &col, sizeof(int));
        //much better, cast the memory as an int pointer and set it to the colour
        ((unsigned int *)mainBmp.Memory)[index] = col;
    }   
}

//Recursive function used to flood fill, not very efficient but it works
void floodFill(int x, int y, int targetColor)
{
    if(getPixelPrimitive(x,y) == targetColor)
    {
        setPixelPrimitive(x,y, currentColour);
        floodFill(x+1, y, targetColor);
        floodFill(x-1, y, targetColor);
        floodFill(x, y+1, targetColor);
        floodFill(x, y-1, targetColor);
    }
}

void saveFile()
{
    FILE *f;
    f = fopen(szfile, "wb+"); //szfile will change to the filename of whatever the user chooses
    fwrite(&mainBmp.width_pixels, sizeof(int), 1, f); //Write the width into the custom file header
    fwrite(&mainBmp.height_pixels, sizeof(int), 1, f); //and height
    fwrite(mainBmp.Memory, sizeof(char), mainBmp.size, f);//and actual image data
    fclose(f); //close file
}
int getPixelPrimitive(int x, int y)
{
   // int index = (x*(BPP/8)+(y*(BPP/8)*mainBmp.width_pixels)); //this is so unnecessary what was i thinking
    int index = (x + y * mainBmp.width_pixels) * 4; //better
    if(index < mainBmp.size && index >= 0) {
	unsigned char* cptr = (char*)mainBmp.Memory+index;
	unsigned int final = 0;
	final |= (cptr[3]<<24); //alpha
	final |= (cptr[2] << 16); //red
	final |= (cptr[1]<< 8); //green
	final |= (cptr[0]);//blue
	return(final);
    }
    return(0x0);
}

void draw(unsigned int mouse_x, unsigned int mouse_y)
{
    mouse_x /= (mainBmp.width_actual / mainBmp.width_pixels); //gets size of pixel in pixels and calculates x
    mouse_y = mainBmp.height_pixels - 1 - mouse_y / (mainBmp.width_actual / mainBmp.width_pixels); //same for y
    setPixelPrimitive(mouse_x, mouse_y, currentColour);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  
{ 
  LRESULT result = 0;
    unsigned int mouse_x = lParam & 0xffff; //Ah bitwise operations, you'll do this a lot in cab202
    unsigned int mouse_y = (lParam >> 16) & 0xffff;
    switch (uMsg) 
    { 
        case WM_CLOSE:
        	running = FALSE;
        	PostQuitMessage(0);
        	break;
        case WM_COMMAND: //buttons
            {
                switch(LOWORD(wParam))
                {
                    case 1: //User clicked save
                        saveFile();  
                        break;
                    case 2: //User clicked flood fill
                        drawMode = FLOODFILL;
                        break;
                    case 3: //User clicked get pixel
                        drawMode = GETTING;
                        break;
                    case 4: //User clicked open file
                        if(GetOpenFileName(&ofc))
                            loadBmp(ofc.lpstrFile);
                        break;
                    case 5: //User clicked Save as
                        if(GetSaveFileName(&ofc))
                            saveFile();
                        break;
                }
                break;
            }
        case WM_LBUTTONDOWN: //User clicked screen
        {
            //switch click type
            if(mouse_x >= mainBmp.origin_x && mouse_x <= mainBmp.width_actual &&
            mouse_y >= mainBmp.origin_y && mouse_y <= mainBmp.height_actual)
            {
                mouse_x /= (mainBmp.width_actual / mainBmp.width_pixels); //fix pixel sizing so it doesn't take just width <- thats not the problem
                mouse_y = mainBmp.height_pixels - 1 - mouse_y / (mainBmp.width_actual / mainBmp.width_pixels);
                switch(drawMode)
                {
                    case NOTDRAWING:
                        drawMode = DRAWING;
                        setPixelPrimitive(mouse_x, mouse_y, currentColour);
                        break;
                    case DRAWING:
                        drawMode = NOTDRAWING;
                        break;
                    case GETTING:
                        //currentColour = getPixelPrimitive(mouse_x / (mainBmp.width_actual / mainBmp.width_pixels), mouse_y / (mainBmp.height_actual / mainBmp.height_pixels));
                        
                        currentColour = getPixelPrimitive(mouse_x, mouse_y);
                        unsigned char * cptr = (unsigned char*)pallette.Memory + cIndex*4;
                        cptr[0] = currentColour & 0xff;
                        cptr[1] = (currentColour>>8) & 0xff;
                        cptr[2] = (currentColour >> 16) & 0xff;
                        cptr[3] = (currentColour >> 24) & 0xff;
                        //error
                        //sendmessages
                        break;
                    case FLOODFILL:
                        floodFill(mouse_x, mouse_y, getPixelPrimitive(mouse_x, mouse_y));
                        break;
                }
            } else 
            {
                cIndex = (mouse_x - pallette.origin_x) / (pallette.width_actual / pallette.width_pixels);
                unsigned char* ptr = (unsigned char*)pallette.Memory + cIndex * (BPP/8);
                currentColour = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
                SendMessageA(trkBlue, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ptr[0]);
                SendMessageA(trkGreen, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ptr[1]);
                SendMessageA(trkRed, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ptr[2]);
                SendMessageA(trkAlpha, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)ptr[3]);
            }
            break;
        }
        case WM_HSCROLL:
    	{
            //Getting the position of each trackbar and setting the colour
    		unsigned char redindex = SendMessageA(trkRed, TBM_GETPOS,0, 0);
    		unsigned char greenindex = SendMessageA(trkGreen, TBM_GETPOS, 0,0);
    		unsigned char blueindex = SendMessageA(trkBlue, TBM_GETPOS, 0,0);
            unsigned char alphaindex = SendMessageA(trkAlpha, TBM_GETPOS, 0,0);
    		unsigned char* ptr = (unsigned char*)pallette.Memory+(cIndex*4);
            ptr[3] = alphaindex;
    		ptr[2] = redindex;
    		ptr[1] = greenindex;
    		ptr[0] = blueindex;
    		currentColour = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
    		break;
    	}
        case WM_LBUTTONUP:
        {
            drawMode = NOTDRAWING;
            break;
        }
        case WM_MOUSEMOVE:
        {
            if(drawMode == DRAWING) draw(mouse_x,mouse_y);
            break;
        }
        default: 
        	return(DefWindowProcA(hwnd, uMsg, wParam, lParam));
        	break;
    } 
    return 0;
} 
HWND WINAPI CreateTrackbar(HWND parent, UINT iMin, UINT iMax, UINT iSelMin, UINT iSelMax, int x, int y)
{
    HWND trackbar = CreateWindowEx(0, TRACKBAR_CLASS, "Trackbar", WS_CHILD | WS_VISIBLE, x, y, 200, 30, parent, (HMENU)ID_TRACK, hinst, NULL);
    SendMessage(trackbar, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(iMin, iMax));
    SendMessage(trackbar, TBM_SETPAGESIZE, 0, (LPARAM) 4);                  
    SendMessage(trackbar, TBM_SETSEL, (WPARAM) FALSE,(LPARAM) MAKELONG(iSelMin, iSelMax)); 
    SendMessage(trackbar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) iSelMin); 
    SetFocus(trackbar);
    return trackbar;
}

VOID WINAPI TBNotifications(WPARAM wParam, HWND track, UINT iSelMin, UINT iSelMax)
{
    DWORD dpos;
    switch(LOWORD(wParam))
    {
        case TB_ENDTRACK:
            dpos = SendMessage(track, TBM_GETPOS, 0, 0);
            if(dpos > iSelMax) SendMessage(track, TBM_SETPOS,(WPARAM)TRUE, (LPARAM)iSelMax);
            else if(dpos < iSelMin) SendMessage(track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)iSelMin);
            break;
        default:
            break;
            
    }
}

void createWindows()
{
    CreateWindow("BUTTON", "Save", WS_VISIBLE | WS_CHILD | WS_BORDER, 1050, 50, 100, 30, HmainWnd, (HMENU)1, NULL, NULL);
    trkRed = CreateTrackbar(HmainWnd, 0, 255, 0, 255, 1050, 110);
    trkGreen = CreateTrackbar(HmainWnd, 0, 255, 0, 255, 1050, 150);
    trkBlue = CreateTrackbar(HmainWnd, 0, 255, 0, 255, 1050, 190);
    trkAlpha = CreateTrackbar(HmainWnd, 0, 255, 0, 255, 1050, 230);
    SendMessageA(trkAlpha, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)255);
    CreateWindow("BUTTON", "Flood Fill", WS_VISIBLE | WS_CHILD | WS_BORDER, 1050, 320, 200, 30, HmainWnd, (HMENU)2, NULL, NULL);
    CreateWindow("BUTTON", "Get Colour", WS_BORDER | WS_VISIBLE | WS_CHILD, 1050, 360, 200, 30, HmainWnd, (HMENU)3,NULL, NULL);
    CreateWindow("BUTTON", "Open File", WS_VISIBLE | WS_CHILD | WS_BORDER, 1050, 500, 200, 30, HmainWnd, (HMENU)4, NULL, NULL);
    CreateWindow("BUTTON", "Save As", WS_VISIBLE | WS_CHILD | WS_BORDER, 1160, 50, 100, 30, HmainWnd, (HMENU)5, NULL, NULL);
}

void renderGrphx()
{
	HDC devCtx = GetDC(HmainWnd);
	StretchDIBits(devCtx, 0, 0, mainBmp.width_actual, mainBmp.height_actual, 0,0, mainBmp.width_pixels, mainBmp.height_pixels, mainBmp.Memory, &mainBmp.binfo, DIB_RGB_COLORS, SRCCOPY);
	StretchDIBits(devCtx, pallette.origin_x, pallette.origin_y, pallette.width_actual, pallette.height_actual,
	 0,0, pallette.width_pixels, pallette.height_pixels, pallette.Memory, &pallette.binfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(HmainWnd, devCtx);
}

void shiftBits(int xoffset, int yoffset) //This is yikes at best, keeps crashing
{
    for(int y = 2; y < mainBmp.height_pixels -1; y++)
    {
        for(int x = 0; x < mainBmp.width_pixels; x++)
        {
           unsigned int * srcPixel = &((unsigned int *)mainBmp.Memory)[y * mainBmp.width_pixels + x];
          if((*srcPixel >> 24) & 0xff)
            {
              ((unsigned int *)mainBmp.Memory)[((y + yoffset) * mainBmp.width_pixels + (x + xoffset))] = *srcPixel;
              *(srcPixel - mainBmp.width_pixels*yoffset) = 0x00808080;
            }
        }
    }
}
void pInput()
{
    short upP = (GetAsyncKeyState(VK_UP)  != 0);
    short doP = (GetAsyncKeyState(VK_DOWN) != 0);
    if(GetAsyncKeyState(VK_SHIFT) && (upP || doP))
        shiftBits(0, upP - doP);
}

void loadBmp(char * str)
{
	FILE * f = fopen(str, "rb+");
	int width, height; //two int32s for width and height of bmp
	fread(&width, sizeof(int), 1, f); //read header
	fread(&height, sizeof(int), 1, f);
	mainBmp.height_pixels = height;
	mainBmp.width_pixels = width;
	mainBmp.size = width * height * 4;
	mainBmp.Memory = malloc(mainBmp.size);
	fread(mainBmp.Memory, sizeof(char), mainBmp.size, f); //read data
	fclose(f);
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
	wc.hIcon = LoadIconA(NULL, MAKEINTRESOURCE(IDI_WINLOGO));
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(0,100,50));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Sprite Maker";
	wc.hIconSm = LoadIconA(NULL, MAKEINTRESOURCE(IDI_WINLOGO));

    ofc.lStructSize = sizeof ofc;
    ofc.hwndOwner = HmainWnd;
    ofc.lpstrFile = szfile;
    ofc.lpstrFile[0] = '\0';
    ofc.nMaxFile = sizeof szfile;
    ofc.lpstrFileTitle = NULL;
    ofc.nMaxFileTitle = 0 ;
    ofc.lpstrInitialDir=NULL ;
    ofc.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
    ofc.lpstrFilter = "bin\0*.bin\0";
    ofc.nFilterIndex = 1;

	//Register Window
	if(!RegisterClassExA(&wc)) {
		MessageBox(NULL, "Window Registration Failed", "Error :(", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return 0;
	}

	//Create WINDOWs
	HmainWnd = CreateWindowExA(WS_EX_CLIENTEDGE,wc.lpszClassName,"Quickdraw", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
	CW_USEDEFAULT, 1024+250, 1024, NULL, NULL, Instance, NULL);
	if(HmainWnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "ERROR :(", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return 0;
 	}
    createWindows();
    //change the width and height, 64 and 64 to something you want, rn has to be a factor of 960(the actual size)
    mainBmp = newBmp(0,0, 960, 960, 64, 64);

    //Set bitmap to transparent grey
    for(int i = 0; i < mainBmp.size / 4; ++i)
        ((unsigned int *)mainBmp.Memory)[i] = 0x00808080;
    
    //Each colour in this program has the normal rgb channels plus a transparency one
    //Set Pallette
    pallette = newBmp(1050, 400, 50, 200, 8, 1);
	int colo[] = {0xff000000, 0x00808080, 0x80ff8080, 0xffff0000, 0xff00ff00, 0xffffff00, 0xffffDBAC, 0xffAA724B};
    memcpy(pallette.Memory, colo, pallette.size);

    //MESSAGE PIPING
	MSG Message = {0}; 
	running = TRUE;

    //Main loop
	while(running) 
	{
        //Windows function that sends the user event (click etc) to the handler MainWndProc()
		while(PeekMessageA(&Message, HmainWnd, 0,0, PM_REMOVE))
			DispatchMessageA(&Message);
		//Render
        pInput();
		renderGrphx();
		Sleep(1);
	}
}
