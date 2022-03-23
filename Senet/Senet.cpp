// Senet.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <time.h>
#include "Senet.h"
extern "C"
{
#include "senetgame.h"
}

#include "SenetBoardWin.h"
#include "SenetSticksWin.h"
#include "SenetHomeWin.h"
#include "HieroglyphicsOKBut.h"
#include "HieroglyphicsWin.h"

#define ID_BOARD_WGT 1000
#define ID_STICKS_WGT 1001
#define ID_HOME_WGT 1002
#define ID_HIEROGLYPHICS_WGT 1003
#define ID_CONTINUE_BUT 1004



#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

SENETGAME *g_game;
int g_movefirst = 1;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static void CreateControls(HWND hwnd);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	srand(time(0));

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SENET, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	RegisterSenetBoardWin(hInstance);
	RegisterSenetSticksWin(hInstance);
	RegisterSenetHomeWin(hInstance);
	RegisterHieroglyphicsOKBut(hInstance);
	RegisterHieroglyphicsWin(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SENET));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SENET));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SENET);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   RECT rect;

   hInst = hInstance; // Store instance handle in our global variable

   rect.left = 0;
   rect.top = 0;
   rect.right = 250 + 721;
   rect.bottom = 250 + 64;
   AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 1, 0);

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, rect.right-rect.left, rect.bottom - rect.top, 
	  NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int Nwhitehome, Nblackhome;
	int a, b, c, d;
	int sticks;
	int roll;
	MOVE move;
	MOVE nullmove = { 0, 0 };

	switch (message)
	{
	case WM_CREATE:
		g_game = senetgame(g_movefirst);
		CreateControls(hWnd);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_STICKS_WGT:
			if (g_game->state == SENET_WAITING_ROLL1)
			{
				MOVE *moves;
				int N;
				roll = senet_throwsticks(&a, &b, &c, &d);
				sticks = a | (b << 1) | (c << 2) | (d << 3);
				SendMessage(GetDlgItem(hWnd, ID_STICKS_WGT), WM_SETSTICKS, sticks, 0);
				senet_applyroll(g_game, roll);
				moves = senet_listmoves(g_game, &N, roll);
				if (N )
					SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "Z s");
				else
				{
					SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "Z pass");
					SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_SHOWBUTTON, 0, 0);
				}
				free(moves);
			}
			break;
		case ID_BOARD_WGT:
			senet_pawnshome(g_game, &Nwhitehome, &Nblackhome);
			SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_SETPAWNS, Nwhitehome, Nblackhome);
			if (g_game->state == SENET_GAME_OVER)
			{
				SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_SHOWBUTTON, 0, 0);
				SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "Z won");
			}
			else if (g_game->state == SENET_WAITING_ROLL2)
			{
				SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "T");
				Sleep(500 + 100 * (rand() % 10));
				roll = senet_throwsticks(&a, &b, &c, &d);
				sticks = a | (b << 1) | (c << 2) | (d << 3);
				SendMessage(GetDlgItem(hWnd, ID_STICKS_WGT), WM_SETSTICKS, sticks, 0);
				senet_applyroll(g_game, roll);
				senet_choosemove(g_game, &move, roll);
				SendMessage(GetDlgItem(hWnd, ID_BOARD_WGT), WM_STARTANIMATION, move.from, move.to);
			}
			else if (g_game->state == SENET_WAITING_ROLL1)
			{
				SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "Z");
			}
			break;
		case ID_HOME_WGT:
			if (g_game->state == SENET_WAITING_P1)
			{
				senet_applymove(g_game, &nullmove);
				SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_HIDEBUTTON, 0, 0);
				PostMessage(hWnd, WM_COMMAND, ID_BOARD_WGT, 0);
			}
			else if (g_game->state == SENET_GAME_OVER)
			{
				SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_SETPAWNS, 0, 0);
				SendMessage(GetDlgItem(hWnd, ID_HOME_WGT), WM_HIDEBUTTON, 0, 0);
				g_movefirst = (g_movefirst == 1) ? 2 : 1;
				senet_resetgame(g_game, g_movefirst);
				InvalidateRect(GetDlgItem(hWnd, ID_BOARD_WGT), 0, FALSE);
				UpdateWindow(GetDlgItem(hWnd, ID_BOARD_WGT));
				if (g_game->state == SENET_WAITING_ROLL1)
					SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "Z");
				else if (g_game->state == SENET_WAITING_ROLL2)
					SetWindowText(GetDlgItem(hWnd, ID_HIEROGLYPHICS_WGT), "T");
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static void CreateControls(HWND hwnd)
{
	HWND hctl;

	hctl = CreateWindow("SenetBoardWin", 
		"", 
		WS_CHILD | WS_VISIBLE,
		0, 0, 721, 250, 
		hwnd, 
		(HMENU) ID_BOARD_WGT, 
		(HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		g_game);

	ShowWindow(hctl, TRUE);
	UpdateWindow(hctl);

	hctl = CreateWindow("SenetSticksWin",
		"",
		WS_CHILD | WS_VISIBLE,
		721, 0, 250, 250,
		hwnd,
		(HMENU) ID_STICKS_WGT,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		g_game);

	ShowWindow(hctl, TRUE);
	UpdateWindow(hctl);

	hctl = CreateWindow("SenetHomeWin",
		"",
		WS_CHILD | WS_VISIBLE,
		0, 250, 721, 64,
		hwnd,
		(HMENU)ID_HOME_WGT,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		NULL);

	ShowWindow(hctl, TRUE);
	UpdateWindow(hctl);
	SendMessage(hctl, WM_SETPAWNS, 0, 0);
	SendMessage(hctl, WM_HIDEBUTTON, 0, 0);

	hctl = CreateWindow("HieroglyphicsWin",
		"",
		WS_CHILD | WS_VISIBLE,
		721, 250, 250, 64,
		hwnd,
		(HMENU)ID_HIEROGLYPHICS_WGT,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		NULL);

	ShowWindow(hctl, TRUE);
	UpdateWindow(hctl);
	//T = Seth
	//Z = Eye of Horus
	SetWindowText(hctl, "Z");

	/*
	hctl = CreateWindow(L"HieroglyphicsOKBut",
		L"",
		WS_CHILD | WS_VISIBLE,
		721/2 - 30, 270, 60, 30,
		hwnd,
		(HMENU)ID_CONTINUE_BUT,
		(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
		NULL);

	ShowWindow(hctl, FALSE);
	UpdateWindow(hctl);
	*/

}
