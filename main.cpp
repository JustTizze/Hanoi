#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "resource.h"


LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;
LPD3DXFONT m_pFont;
LPD3DXSPRITE sprite;

LPD3DXMESH mPole;
LPD3DXBUFFER mPoleMaterials;
DWORD mPoleMaterialNum;
D3DXMATRIX matView;
D3DXMATRIX matProjection;
D3DXMATRIX matSprite;

float alpha = 0.0f;
#define SHADOW_MAP_SIZE	512
#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 720


#define VERTEXFORMAT (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)
struct CUSTOMVERTEX {FLOAT X, Y, Z, nX, nY, nZ, tU, tV;};

CUSTOMVERTEX * vertices;
short indices [48*8*3];

LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;
LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;

LPDIRECT3DTEXTURE9 Textures[4];

LPD3DXBUFFER materialBuffer;
D3DXVECTOR3			g_vEyePos			= D3DXVECTOR3 (0.0f, 1.0f, -1.75f);
D3DXVECTOR3			g_vEyeAim			= D3DXVECTOR3 (0.0f, 0.25f, 0.0f);
D3DXVECTOR3			g_vUp				= D3DXVECTOR3(   0.0f,  1.0f,   0.0f );

float frame=0.0f;

RECT screen;
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define PoleA 0
#define PoleB 1
#define PoleC 2

#define tau 6.283185307179586

LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
LARGE_INTEGER Frequency;

char discsinthispole [3];
bool quit = false;
char totaldiscs;
wchar_t string[3];
char activedisc=0;
int stage = 0;
char * poles[3];
struct disc
{
	char X;
	char Y;
	float rnd;
};

disc * discs;

void initD3D(HWND hWnd);
void render_frame(void);
void cleanD3D(void);
void init_graphics(void); 
void init_light(void);
void render_stage1(void);
void cre8discs(void);
void renderdisc (char);

void hanoi(int, char, char, char);
void anim8(char, char, char, char, char);
void movedisc(char, char);
void upd8coords(char, char, char);
void init(void);
void clear(void);
void peek(void);
void renderroom(void);
void init_shad(void);
void render_shad(void);

D3DXMATRIX * Translate;
D3DXMATRIX TranslateD;
D3DXMATRIX Rotate;
LRESULT CALLBACK WindowProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam);

// entry point

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nShowCmd)
{
	HWND hWnd;
	WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass1";

    // register the window class
    RegisterClassEx(&wc);

	hWnd = CreateWindowEx(0,
                          L"WindowClass1",    // name of the window class
                          L"Hanoi",   // title of the window
                          (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),    // window style
                          CW_USEDEFAULT,    // x-position of the window
                          0,    // y-position of the window
                          SCREEN_WIDTH,    // width of the window
                          SCREEN_HEIGHT,    // height of the window
                          NULL,    // we ain't got no parent windows, NULL
                          NULL,    // we ain't using menus, NULL
                          hInstance,    // application handle
                          NULL);    // used with multiple windows, NULL

    // display the window on the screen
    ShowWindow(hWnd, SW_SHOW);
	initD3D(hWnd);
	init_graphics();
    // enter the main loop:

    // this struct holds Windows event messages
    MSG msg;


    // wait for the next message in the queue, store the result in 'msg'
    while(TRUE)
	{
		// Check to see if any messages are waiting in the queue
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If the message is WM_QUIT, exit the while loop
		if(msg.message == WM_QUIT)
			break;

		if(quit == true)
			break;

		if(stage==0)
			render_stage1();
		else if(stage==1)
		{
			init();
			cre8discs();
			render_frame();
			stage++;
		}
		else if(stage==2)
		{
			hanoi(totaldiscs, PoleA, PoleC, PoleB);			
			stage++;
		}

		else if(stage==3)
		{
			clear();
			string[0] = 0;
			totaldiscs = 0;
			stage=0;
		}

	}
    cleanD3D();
	
	clear();
    return msg.wParam;
	  
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch(message)
    {
        // this message is read when the window is closed
        case WM_DESTROY:
            {
                // close the application entirely
				quit = true;
                PostQuitMessage(0);
                return 0;
            } break;

		case WM_KEYDOWN:
            {
				if (stage==0)
				{
					if (wParam==VK_RETURN)
					{
						if (string[1]==0)
							totaldiscs = string[0]-0x30;
						else if (string[2]==0)
							totaldiscs = 10*(string[0]-0x30)+string[1]-0x30;
						if (totaldiscs > 0)
							stage=1;
					}
					else if (wParam==VK_BACK)
					{
						if (string[0]!=0)
						{
							char i=0;
							while(string[i]!=0)
								i++;
							string[i-1]=0;
						}
					}
					else if (wParam>=0x30 && wParam<=0x39)
					{
						char i=0;
						while(string[i]!=0)
							i++;
						if (i<2)
							string[i]=wParam;
					}
					else if (wParam>=0x60 && wParam<=0x69)
					{
						char i=0;
						while(string[i]!=0)
							i++;
						if (i<2)
							string[i]=wParam-0x30;
					}
				}
				/*if (wParam== 'A')
				{
					alpha += 0.25;
					D3DXMatrixLookAtLH(&matView,
							   &D3DXVECTOR3 (1.75f*sin(alpha), 1.0f, -1.75f*cos(alpha)),
							   &D3DXVECTOR3 (0.0f, 0.25f, 0.0f),
							   &D3DXVECTOR3 (0.0f, 1.0f, 0.0f));

					d3ddev->SetTransform(D3DTS_VIEW, &matView);
				}*/
               
            } break;

    }

    // Handle any messages the switch statement didn't
    return DefWindowProc (hWnd, message, wParam, lParam);
}

void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	d3dpp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // create a device class using this information and information from the d3dpp stuct
     d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);

}
void renderroom(void)
{
	D3DXMATERIAL* pMaterials = (D3DXMATERIAL*) mPoleMaterials->GetBufferPointer();
	//D3DXMatrixScaling(&Scale,1.0f, 1.0f, 1.0f);
	D3DXMatrixTranslation(&TranslateD,0.0f, 0.0f,0.0f);
	d3ddev->SetTransform(D3DTS_WORLD, &(TranslateD));
	pMaterials[3].MatD3D.Specular = D3DXCOLOR(0.2f,0.2f,0.2f,0.2f);
	pMaterials[1].MatD3D.Specular = D3DXCOLOR(1.0f,1.0f,1.0f,1.0f);

	for ( DWORD i = 0; i < mPoleMaterialNum; i++ )
	{	
		//if (pMaterials[i].pTextureFilename != NULL)
		d3ddev->SetTexture(0,Textures[i]);
		pMaterials[i].MatD3D.Ambient = D3DXCOLOR(1.0f,1.0f,1.0f,1.0f); //shitty exporter makes me do this
		d3ddev->SetMaterial(&(pMaterials[i].MatD3D));
		mPole->DrawSubset(i);
	}
}
void render_frame(void)
{
	
	for (char hh = 1; hh < totaldiscs+1; hh++)
	{

		if (totaldiscs > 6)
			D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f*6.0f/totaldiscs,0.0f);
		else
			D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f,0.0f);
	}

    render_shad();
}
void render_stage1(void)
{
    // clear the window to a deep blue
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    d3ddev->BeginScene();    // begins the 3D scene

	screen.top = 60;
	
	m_pFont->DrawText(NULL,
		L"Number of discs:",-1,&screen,DT_CENTER, D3DCOLOR_XRGB(200, 200, 100));
	screen.top = 220;
	m_pFont->DrawText(NULL,
		string,-1,&screen,DT_CENTER, D3DCOLOR_XRGB(200, 200, 100));

	d3ddev->EndScene();    // ends the 3D scene

    d3ddev->Present(NULL, NULL, NULL, NULL);    // displays the created frame
}

void cleanD3D(void)
{
    d3ddev->Release();
    d3d->Release();
	m_pFont->Release();
	mPole->Release();
	if (totaldiscs != 0)
	{
		v_buffer->Release();
		i_buffer->Release();
	}
}
void init_graphics(void)
{
    	   
    d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE); 
	d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	
	 
	D3DXCreateFont(d3ddev,80, 30, 1000, 1, false, DEFAULT_CHARSET, 
	OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"", &m_pFont ); 

	screen.left=0; screen.right = SCREEN_WIDTH; screen.top=0; screen.bottom=SCREEN_HEIGHT;

	D3DXLoadMeshFromXResource(NULL,MAKEINTRESOURCEA(IDR_MESHES1),"MESHES",
		D3DXMESH_MANAGED, d3ddev, NULL, &mPoleMaterials, NULL, &mPoleMaterialNum, &mPole);
	D3DXCreateTextureFromResource(d3ddev,NULL,MAKEINTRESOURCE(IDR_CARPET),&Textures[1]);
	D3DXCreateTextureFromResource(d3ddev,NULL,MAKEINTRESOURCE(IDR_BRICKS),&Textures[2]);
	D3DXCreateTextureFromResource(d3ddev,NULL,MAKEINTRESOURCE(IDR_WOOD),&Textures[3]);

			D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3 (0.2f, 1.0f, -1.85f),
                       &D3DXVECTOR3 (0.0f, 0.25f, 0.0f),
                       &D3DXVECTOR3 (0.0f, 1.0f, 0.0f));

		d3ddev->SetTransform(D3DTS_VIEW, &matView);

		D3DXMatrixPerspectiveFovLH(&matProjection,
                               0.125f *tau,
                               1.5f,
                               1.0f,
                               10.0f);


		d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);
	init_light();
	init_shad();
}
void init_shad(void)
{
	;
}
void init_light(void)
{
    D3DLIGHT9 light;    // create the light struct

	d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(180, 180, 180)); 
	d3ddev->SetRenderState(D3DRS_SPECULARENABLE, TRUE);

    ZeroMemory(&light, sizeof(light)); 	
    light.Type = D3DLIGHT_POINT;
    light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 0.8f, 1.0f);
	light.Specular = D3DXCOLOR(0.4f, 0.4f, 0.35f, 1.0f);
    light.Position = D3DXVECTOR3(0.0f, 0.0f, 2.0f);
	light.Range = 20.0f;
	light.Attenuation0 = 0.00f;
    light.Attenuation1 = 0.01f;
    light.Attenuation2 = 0.01f;

    d3ddev->SetLight(0, &light);    // send the light struct properties to light #0
    d3ddev->LightEnable(0, FALSE);    // turn on light #0

	ZeroMemory(&light, sizeof(light)); 	
    light.Type = D3DLIGHT_POINT;
    light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 0.8f, 1.0f);
	light.Specular = D3DXCOLOR(0.4f, 0.4f, 0.35f, 1.0f);
    light.Position = D3DXVECTOR3(1.0f, 1.0f, -0.5f);
	light.Range = 20.0f;
	light.Attenuation0 = 0.01f;
    light.Attenuation1 = 0.02f;
    light.Attenuation2 = 0.01f;

    d3ddev->SetLight(1, &light);    // send the light struct properties to light #0
    d3ddev->LightEnable(1, TRUE);    // turn on light #0

	return;
}

//void prog()
//{
//	
//
//      hanoi(totaldiscs, PoleA, PoleC, PoleB);
//	
//	  clear();
//
//	  //scanf("%d", &totaldiscs);
//
//   return;
//
//}
void init()
{
	poles[0] = new char [totaldiscs];
	  poles[1] = new char [totaldiscs];
	  poles[2] = new char [totaldiscs];
	  discs = new disc [totaldiscs+1]; //Smallest disc is 1, so we create one more disc
	  srand(totaldiscs);
	  
	Translate = new D3DXMATRIX [totaldiscs + 1];
	  //arrange dem discs on pole A
	  for(char i = 1; i<=totaldiscs; i++)
	  {
		  poles[PoleA][i-1] = totaldiscs - i + 1;
		  discs[i].X = PoleA;
		  discs[i].Y = totaldiscs - i + 1;
		  discs[i].rnd = (float)(rand()%22)/11+(rand()%74)/37;
	  }
	  discsinthispole [PoleA] = totaldiscs;
	  discsinthispole [PoleB] = 0;
	  discsinthispole [PoleC] = 0;
	  return;
}
void clear()
{
	if (totaldiscs != 0)
	{
		delete[] poles[0];
		delete[] poles[1];
		delete[] poles[2];
		delete[] discs;
		delete[] Translate;
	}
	return;
}

void hanoi(int n, char from, char to, char via)
{
   if (n == 0)
      return;

   hanoi(n - 1, from, via, to);
   movedisc(from, to);
   hanoi(n - 1, via, to, from);
}

void movedisc(char from, char to)
{
	discsinthispole [from]--;
	poles [to][discsinthispole [to]] = poles [from] [discsinthispole [from]];
	upd8coords(poles [to][discsinthispole [to]], to, discsinthispole [to] + 1);
	discsinthispole [to]++;
	return;
}

void upd8coords(char discno, char iX, char iY)
{
	anim8 (discno, discs [discno].X, discs [discno].Y, iX, iY);
	discs [discno].X = iX;
	discs [discno].Y = iY;
	return;
}
//void printstats()
//{
//	//printf("A:(%d) \t B:(%d) \t C:(%d) \r\n", discsinthispole[PoleA], discsinthispole[PoleB], discsinthispole[PoleC]);
//	for(char i = 0; i<totaldiscs; i++)
//	{
//		//printf("%d \t %d \t %d \r\n", poles [PoleA][i],poles [PoleB][i],poles [PoleC][i]);
//	}
//
//	return;
//}

void anim8(char discno, char oldX, char oldY, char newX, char newY)
{
	
	//lift the disc
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);
	QueryPerformanceCounter(&EndingTime);
	frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);

	while((frame <= 1)&&quit==false) 
	{


		for (char hh = 1; hh < totaldiscs+1; hh++)
		{
			if (hh==discno)
			{
				if (totaldiscs > 6)
					D3DXMatrixTranslation(&Translate[hh], (float)(oldX - 1)*0.5f, (float)((oldY - 1)*0.1f*6.0f/totaldiscs)*(1-frame) + 0.8f * frame,0.0f);
				else
					D3DXMatrixTranslation(&Translate[hh], (float)(oldX - 1)*0.5f, (float)((oldY - 1)*0.1f)*(1-frame) + 0.8f * frame,0.0f);
			}
			else
			{
				if (totaldiscs > 6)
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f*6.0f/totaldiscs,0.0f);
				else
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f,0.0f);
			}
		}

		render_shad();

		QueryPerformanceCounter(&EndingTime);
		frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);
	}

	//shift the disc
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);
	QueryPerformanceCounter(&EndingTime);
	frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);

	while((frame <= 1)&&quit==false) 
	{
		peek();
		for (char hh = 1; hh < totaldiscs+1; hh++)
		{
			if (hh==discno)
			{
				D3DXMatrixTranslation(&Translate[hh], (float)((oldX - 1)*(1-frame)+(newX - 1)*(frame))*0.5f, 0.8f,0.0f);

			}
			else
			{
				if (totaldiscs > 6)
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f*6.0f/totaldiscs,0.0f);
				else
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f,0.0f);
			}
		}

		render_shad();

		QueryPerformanceCounter(&EndingTime);
		frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);
	}

	//drop the disc
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);
	QueryPerformanceCounter(&EndingTime);
	frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);

	while((frame <= 1)&&quit==false) 
	{
		peek();
		for (char hh = 1; hh < totaldiscs+1; hh++)
		{
			if (hh==discno)
			{
				if (totaldiscs > 6)
					D3DXMatrixTranslation(&Translate[hh], (float)(newX - 1)*0.5f, (float)((newY - 1)*0.1f*6.0f/totaldiscs)*frame + 0.8 * (1-frame),0.0f);
				else
					D3DXMatrixTranslation(&Translate[hh], (float)(newX - 1)*0.5f, (float)((newY - 1)*0.1f)*frame + 0.8 * (1-frame),0.0f);
			}
			else
			{
				if (totaldiscs > 6)
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f*6.0f/totaldiscs,0.0f);
				else
					D3DXMatrixTranslation(&Translate[hh], (float)(discs[hh].X - 1)*0.5f, (float)(discs[hh].Y - 1)*0.1f,0.0f);
			}
		}
		render_shad();


		QueryPerformanceCounter(&EndingTime);
		frame = ((float)(EndingTime.QuadPart) - (float)(StartingTime.QuadPart))/((float)(Frequency.QuadPart)*1.5);
	}
}

void cre8discs()
{	
	//CUSTOMVERTEX vertices[48*8];
	
	float iSin;
	float iCos;
	float il;
	float height=0.1;
	float scaledsize;
	vertices = new CUSTOMVERTEX [48*8*totaldiscs];

	
	for (char i=0; i < 48; i++)
	{
		iSin = sin((double)i/48*tau);
		iCos = cos((double)i/48*tau);
		il = (float)i/48;

		for (char size=1; size <= totaldiscs; size++)
		{
			scaledsize=(float)size*0.025+0.12;
			if (totaldiscs > 6)
				{
					height=(float)0.1f*6.0f/totaldiscs;
					scaledsize=(float)size*0.025*6.0f/totaldiscs+0.12;
				}
			//inner circle bottom, normal parallel to y axis
			vertices[(size-1)*48*8+i].X = 0.08 * iCos;
			vertices[(size-1)*48*8+i].Y = 0.0;
			vertices[(size-1)*48*8+i].Z = 0.08 * iSin;
			vertices[(size-1)*48*8+i].nX = 0.0;
			vertices[(size-1)*48*8+i].nY = -1.0;
			vertices[(size-1)*48*8+i].nZ = 0.0;
			vertices[(size-1)*48*8+i].tU = il;
			vertices[(size-1)*48*8+i].tV = 1.0;

			//inner circle top, normal parallel to y axis
			vertices[(size-1)*48*8+i+48].X = 0.08 * iCos;
			vertices[(size-1)*48*8+i+48].Y = height;
			vertices[(size-1)*48*8+i+48].Z = 0.08 * iSin;
			vertices[(size-1)*48*8+i+48].nX = 0.0;
			vertices[(size-1)*48*8+i+48].nY = 1.0;
			vertices[(size-1)*48*8+i+48].nZ = 0.0;
			vertices[(size-1)*48*8+i+48].tU = il;
			vertices[(size-1)*48*8+i+48].tV = 0.0;

			//outer circle bottom, normal parallel to y axis
			vertices[(size-1)*48*8+i+48*2].X = scaledsize * iCos;
			vertices[(size-1)*48*8+i+48*2].Y = 0.0;
			vertices[(size-1)*48*8+i+48*2].Z = scaledsize * iSin;
			vertices[(size-1)*48*8+i+48*2].nX = 0.0;
			vertices[(size-1)*48*8+i+48*2].nY = -1.0;
			vertices[(size-1)*48*8+i+48*2].nZ = 0.0;
			vertices[(size-1)*48*8+i+48*2].tU = il;
			vertices[(size-1)*48*8+i+48*2].tV = 0.1f;

			//outer circle top, normal parallel to y axis
			vertices[(size-1)*48*8+i+48*3].X = scaledsize * iCos;
			vertices[(size-1)*48*8+i+48*3].Y = height;
			vertices[(size-1)*48*8+i+48*3].Z = scaledsize * iSin;
			vertices[(size-1)*48*8+i+48*3].nX = 0.0;
			vertices[(size-1)*48*8+i+48*3].nY = 1.0;
			vertices[(size-1)*48*8+i+48*3].nZ = 0.0;
			vertices[(size-1)*48*8+i+48*3].tU = il;
			vertices[(size-1)*48*8+i+48*3].tV = 0.0f;

			//inner circle bottom, normal parallel to face
			vertices[(size-1)*48*8+i+48*4].X = 0.08 * iCos;
			vertices[(size-1)*48*8+i+48*4].Y = 0.0;
			vertices[(size-1)*48*8+i+48*4].Z = 0.08 * iSin;
			vertices[(size-1)*48*8+i+48*4].nX = iSin;
			vertices[(size-1)*48*8+i+48*4].nY = 0.0;
			vertices[(size-1)*48*8+i+48*4].nZ = iCos;
			vertices[(size-1)*48*8+i+48*4].tU = 0.5 + 0.08 * iSin;
			vertices[(size-1)*48*8+i+48*4].tV = 0.5 + 0.08 * iCos;

			//inner circle top, normal parallel to face
			vertices[(size-1)*48*8+i+48*5].X = 0.08 * iCos;
			vertices[(size-1)*48*8+i+48*5].Y = height;
			vertices[(size-1)*48*8+i+48*5].Z = 0.08 * iSin;
			vertices[(size-1)*48*8+i+48*5].nX = iSin;
			vertices[(size-1)*48*8+i+48*5].nY = 0.0;
			vertices[(size-1)*48*8+i+48*5].nZ = iCos;
			vertices[(size-1)*48*8+i+48*5].tU = 0.5 + 0.08 * iSin;
			vertices[(size-1)*48*8+i+48*5].tV = 0.5 + 0.08 * iCos;

			//outer circle bottom, normal parallel to face
			vertices[(size-1)*48*8+i+48*6].X = scaledsize * iCos;
			vertices[(size-1)*48*8+i+48*6].Y = 0.0;
			vertices[(size-1)*48*8+i+48*6].Z = scaledsize * iSin;
			vertices[(size-1)*48*8+i+48*6].nX = iCos;
			vertices[(size-1)*48*8+i+48*6].nY = 0.0;
			vertices[(size-1)*48*8+i+48*6].nZ = iSin;
			vertices[(size-1)*48*8+i+48*6].tU = 0.5 + 0.25*iCos;
			vertices[(size-1)*48*8+i+48*6].tV = 0.5 + 0.25*iSin;

			//outer circle top, normal parallel to face
			vertices[(size-1)*48*8+i+48*7].X = scaledsize * iCos;
			vertices[(size-1)*48*8+i+48*7].Y = height;
			vertices[(size-1)*48*8+i+48*7].Z = scaledsize * iSin;
			vertices[(size-1)*48*8+i+48*7].nX = iCos;
			vertices[(size-1)*48*8+i+48*7].nY = 0.0;
			vertices[(size-1)*48*8+i+48*7].nZ = iSin;
			vertices[(size-1)*48*8+i+48*7].tU = 0.5 + 0.25*iCos;
			vertices[(size-1)*48*8+i+48*7].tV = 0.5 + 0.25*iSin;
		}

		#define indx(a, b, c, d) indices[a] = b; indices[a+1] = c; indices[a+2] = d;
		if (i==47)
		{
			// inner circle faces
			indx(i*3, i, 0, 48); 
			indx(48*3+i*3, 48, i+48, i);

			// outer circle faces
			indx(48*2*3+i*3, 48*2, i+48*2, i+48*3);
			indx(48*3*3+i*3, i+48*3, 48*3, 48*2);

			// base
			indx(48*4*3+i*3, 48*4, i+48*4, i+48*6);
			indx(48*5*3+i*3, i+48*6, 48*6, 48*4);

			// top
			indx(48*6*3+i*3, i+48*5, 48*5, 48*7);
			indx(48*7*3+i*3, 48*7, i+48*7, i+48*5);
		}
		else
		{
			// inner circle faces
			indx(i*3, i, i+1, i+49); 
			indx(48*3+i*3, i+49, i+48, i);

			// outer circle faces
			indx(48*2*3+i*3, i+48*2+1, i+48*2, i+48*3);
			indx(48*3*3+i*3, i+48*3, i+48*3+1, i+48*2+1);

			// base
			indx(48*4*3+i*3, i+48*4+1, i+48*4, i+48*6);
			indx(48*5*3+i*3, i+48*6, i+48*6+1, i+48*4+1);

			// top
			indx(48*6*3+i*3, i+48*5, i+48*5+1, i+48*7+1);
			indx(48*7*3+i*3, i+48*7+1, i+48*7, i+48*5);
		}
	}

	d3ddev->CreateVertexBuffer(sizeof(CUSTOMVERTEX)*48*8*totaldiscs*2,
                               0,
                               VERTEXFORMAT,
                               D3DPOOL_DEFAULT,
                               &v_buffer,
                               NULL);

    VOID* pVoid;    // a void pointer

    // lock v_buffer and load the vertices into it
    v_buffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, vertices, sizeof(CUSTOMVERTEX)*48*8*totaldiscs);

	v_buffer->Unlock();

	d3ddev->CreateIndexBuffer(sizeof(short)*48*8*3,
                              0,
                              D3DFMT_INDEX16,
                              D3DPOOL_DEFAULT,
                              &i_buffer,
                              NULL);

    // lock i_buffer and load the indices into it
    i_buffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, indices, sizeof(short)*48*8*3);
    i_buffer->Unlock();
	return;
}

void renderdisc (char discno)
{	
	d3ddev->SetTexture(0,Textures[3]);
	d3ddev->SetFVF(VERTEXFORMAT);
	d3ddev->SetStreamSource(0,v_buffer,0,sizeof(CUSTOMVERTEX));
	d3ddev->SetIndices(i_buffer);
	D3DXMatrixRotationY(&Rotate, discs[discno].rnd * tau);
	d3ddev->SetTransform(D3DTS_WORLD, &(Rotate * Translate[discno]));
	d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, (discno-1)*48*8, 0, 48*8*totaldiscs, 0, 48*8);
	return;
}
void render_shad()
{
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    d3ddev->BeginScene();    // begins the 3D scen
	D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3 (0.2f, 1.0f, -1.85f),
                       &D3DXVECTOR3 (0.0f, 0.25f, 0.0f),
                       &D3DXVECTOR3 (0.0f, 1.0f, 0.0f));

		d3ddev->SetTransform(D3DTS_VIEW, &matView);
		D3DXMatrixPerspectiveFovLH(&matProjection,
                               0.125f *tau,
                               1.5f,
                               1.0f,
                               10.0f);

		d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);


				renderroom();
			for (char hh=1; hh <= totaldiscs; hh++)
			{
				renderdisc(hh);
			}
			
	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);
	return;
}
void peek(void)
{
	MSG msg;
		// Check to see if any messages are waiting in the queue
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
}