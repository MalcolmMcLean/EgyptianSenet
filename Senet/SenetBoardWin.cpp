#include <Windows.h>
#include <Windowsx.h>
#include "resource.h"

#include <math.h>

extern "C"
{
#include "senetgame.h"
}
#include "SenetBoardWin.h"

#define MOVING_TOGO 1
#define MOVING_OPPOSITE 2


#define clamp(x, low, high) ((x) < (low) ? (low) : (x) > (high) ? (high) : (x))

typedef struct
{
	HBITMAP hpieces;
	HBITMAP hboard;
	HBITMAP hshadow;
	SENETGAME *game;
	int selx;
	int sely;
	int consider_x;
	int consider_y;
	int animating;
	UINT_PTR timer_id;
	int ticks;
	int spritex;
	int spritey;
	int pawnmoving;
} SENETBOARD;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static SENETBOARD *CreateBoard(HWND hwnd, SENETGAME *game);
static void KillSenetBoard(SENETBOARD *board);
static void PaintMe(HWND hwnd, SENETBOARD *board);
static void DarkenRect(HDC hdc, RECT *rect);
static void DoMouseButtonDown(HWND hwnd, SENETBOARD *board, int x, int y);
static void DoMouseMove(HWND hwnd, SENETBOARD *board, int x, int y);
static void StartAnimation(HWND hwnd, SENETBOARD *board, int from, int to);
static void ProcessTimer(HWND hwnd, SENETBOARD *board);
static void WindowToCell(HWND hwnd, int x, int y, int *cx, int *cy);
static void CellToRect(HWND hwnd, RECT *rect, int cx, int cy);
static HBITMAP makepieces(HWND hwnd);
static int max3(int a, int b, int c);

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM RegisterSenetBoardWin(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SENET));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0; // MAKEINTRESOURCE(IDC_SENET);
	wcex.lpszClassName = "SenetBoardWin";
	wcex.hIconSm = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SENETBOARD *board;

	board = (SENETBOARD *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_CREATE:
		board = CreateBoard(hwnd, (SENETGAME *) (((CREATESTRUCT *)lParam)->lpCreateParams) );
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)board);
		break;
	case WM_COMMAND:
		break;
	case WM_LBUTTONDOWN:
		DoMouseButtonDown(hwnd, board, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSEMOVE:
		DoMouseMove(hwnd, board, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_TIMER:
		ProcessTimer(hwnd, board);
		break;
	case WM_PAINT:
		PaintMe(hwnd, board);
		break;
	case WM_STARTANIMATION:
		StartAnimation(hwnd, board, (int)wParam, (int)lParam);
		break;
	case WM_DESTROY:
		KillSenetBoard(board);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

static SENETBOARD *CreateBoard(HWND hwnd, SENETGAME *game)
{
	SENETBOARD *board;
	HINSTANCE hInstance;
	HDC hdc;
	int x, y;

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

	board = (SENETBOARD *)malloc(sizeof(SENETBOARD));

	board->game = game;
	board->hboard = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BOARD), IMAGE_BITMAP, 0, 0, 0);
	board->hpieces = makepieces(hwnd);

	BITMAPINFO bmi;
	// setup bitmap info  
	// set the bitmap width and height to 60% of the width and height of each of the three horizontal areas. Later on, the blending will occur in the center of each of the three areas. 
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 32;
	bmi.bmiHeader.biHeight = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;         // four 8-bit components 
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 32 * 32 * 4;

	VOID *pvBits;          // pointer to DIB section 
	hdc = CreateCompatibleDC(GetDC(0));
	board->hshadow = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	for (y = 0; y < 32; y++)
	for (x = 0; x < 32; x++)
	{
		int alpha;
		alpha =  150 - ((x-16)*(x-16)+(y-16)*(y-16));
		alpha = clamp(alpha, 0, 255);

		((UINT32 *)pvBits)[x + y * 32] = (alpha << 24) | (0xFF * alpha)/0xFF;
	}




	DeleteDC(hdc);
	board->selx = -1;
	board->sely = -1;
	board->consider_x = -1;
	board->consider_y = -1;
	board->animating = 0;
	return board;
}

static void KillSenetBoard(SENETBOARD *board)
{
	if (board)
	{
		DeleteObject(board->hboard);
		DeleteObject(board->hpieces);
		free(board);
	}
}

static void PaintMe(HWND hwnd, SENETBOARD *board)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HINSTANCE hInstance;
	HDC hdcbmp;
	BITMAP bm;
	RECT rect;
	int cx, cy;

	GetClientRect(hwnd, &rect);

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = BeginPaint(hwnd, &ps);
	GetObject(board->hboard, sizeof(BITMAP), &bm);
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, board->hboard);
	StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcbmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	GetObject(board->hpieces, sizeof(BITMAP), &bm);
	SelectObject(hdcbmp, board->hpieces);

	for (cy = 0; cy < 3; cy++)
	{
		for (cx = 0; cx < 10; cx++)
		{
			RECT crect;
			BLENDFUNCTION bf;

			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = 255;
			bf.AlphaFormat = AC_SRC_ALPHA;

			CellToRect(hwnd, &crect, cx, cy);
			if (cx == board->selx && cy == board->sely)
			{
				SelectObject(hdcbmp, board->hshadow);
				AlphaBlend(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
					hdcbmp, 0, 0, 32, 32, bf);
				SelectObject(hdcbmp, board->hpieces);
			}
			if (cx == board->consider_x && cy == board->consider_y)
			{
				SelectObject(hdcbmp, board->hshadow);
				AlphaBlend(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
					hdcbmp, 0, 0, 30, 30, bf);
				SelectObject(hdcbmp, board->hpieces);
			}
			
			if (board->animating && cx == board->selx && cy == board->sely)
				continue;
			if (board->animating && cx == board->consider_x && cy == board->consider_y)
				continue;

			if (board->game->board[cy][cx] == 1)
				TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 130, 0, bm.bmWidth -130, bm.bmHeight, RGB(255, 128, 128));
			else if (board->game->board[cy][cx] == 2)
				TransparentBlt(hdc, crect.left -20, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));

			//SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
			//SelectObject(hdc, GetStockObject(WHITE_PEN));
			//Rectangle(hdc, crect.left, crect.top, crect.right, crect.bottom);
		}
	}
	if (board->animating)
	{
		RECT crect;
		if (board->pawnmoving == MOVING_TOGO)
		{
			CellToRect(hwnd, &crect, board->consider_x, board->consider_y);
			if (board->game->board[board->consider_y][board->consider_x] == 1)
				TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 130, 0, bm.bmWidth - 130, bm.bmHeight, RGB(255, 128, 128));
			else if (board->game->board[board->consider_y][board->consider_x] == 2)
				TransparentBlt(hdc, crect.left - 20, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));
			crect.right = board->spritex + (crect.right - crect.left);
			crect.bottom = board->spritey + (crect.bottom - crect.top);
			crect.left = board->spritex;
			crect.top = board->spritey;
			if (board->game->board[board->sely][board->selx] == 1)
				TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 130, 0, bm.bmWidth - 130, bm.bmHeight, RGB(255, 128, 128));
			else if (board->game->board[board->sely][board->selx] == 2)
				TransparentBlt(hdc, crect.left - 20, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));

		}
		else if (board->pawnmoving == MOVING_OPPOSITE)
		{
			CellToRect(hwnd, &crect, board->consider_x, board->consider_y);
			if (board->game->board[board->sely][board->selx] == 1)
				TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 130, 0, bm.bmWidth - 130, bm.bmHeight, RGB(255, 128, 128));
			else if (board->game->board[board->sely][board->selx] == 2)
				TransparentBlt(hdc, crect.left - 20, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));
			crect.right = board->spritex + (crect.right - crect.left);
			crect.bottom = board->spritey + (crect.bottom - crect.top);
			crect.left = board->spritex;
			crect.top = board->spritey;
			if (board->game->board[board->consider_y][board->consider_x] == 1)
				TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 130, 0, bm.bmWidth - 130, bm.bmHeight, RGB(255, 128, 128));
			else if (board->game->board[board->consider_y][board->consider_x] == 2)
				TransparentBlt(hdc, crect.left - 20, crect.top, crect.right - crect.left, crect.bottom - crect.top,
				hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));
		}
	}


	DeleteDC(hdcbmp);
	EndPaint(hwnd, &ps);
}

static void DarkenRect(HDC hdc, RECT *rect)
{
	int x, y;
	COLORREF col;
	int red, green, blue;

	for (y = rect->top; y < rect->bottom; y++)
	{
		for (x = rect->left; x < rect->right; x++)
		{
			col = GetPixel(hdc, x, y);
			red = GetRValue(col);
			green = GetGValue(col);
			blue = GetBValue(col);
			SetPixel(hdc, x, y, RGB(red / 2, green / 2, blue / 2));
		}
	}
}

static void DoMouseButtonDown(HWND hwnd, SENETBOARD *board, int x, int y)
{
	int cx, cy;
	if (board->animating)
		return;
	if (board->game->state != SENET_WAITING_P1)
		return;
	if (senet_stuck(board->game))
		return;
	WindowToCell(hwnd, x, y, &cx, &cy);
	if (board->game->board[cy][cx] == board->game->togo)
	{
		board->selx = cx;
		board->sely = cy;
		InvalidateRect(hwnd, 0, TRUE);
		UpdateWindow(hwnd);
	}
	else if (board->selx != -1)
	{
		static MOVE move;
		move.from = senet_xytoindex(board->selx, board->sely);
		move.to = senet_xytoindex(cx, cy);
		if (senet_moveallowed(board->game, &move))
		{
			board->consider_x = cx;
			board->consider_y = cy;
			StartAnimation(hwnd, board, move.from, move.to);
			/*
			senet_applymove(board->game, &move);
			board->selx = -1;
			board->sely = -1;
			board->consider_x = -1;
			board->consider_y = -1;
			InvalidateRect(hwnd, 0, TRUE);
			UpdateWindow(hwnd);
			SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM) &move);
			*/
		}
	}
}

static void DoMouseMove(HWND hwnd, SENETBOARD *board, int x, int y)
{
	int cx, cy;
	if (board->animating)
		return;
	if (board->game->state != SENET_WAITING_P1)
		return;
	WindowToCell(hwnd, x, y, &cx, &cy);
	if (board->consider_x == cx && board->consider_y == cy)
		return;

	if (board->selx != -1)
	{
		MOVE move;
		move.from = senet_xytoindex(board->selx, board->sely);
		move.to = senet_xytoindex(cx, cy);
		if (senet_moveallowed(board->game, &move))
		{
			board->consider_x = cx;
			board->consider_y = cy;
			InvalidateRect(hwnd, 0, FALSE);
			UpdateWindow(hwnd);
		}
		else if (board->consider_x != -1)
		{
			board->consider_x = -1;
			board->consider_y = -1;
			InvalidateRect(hwnd, 0, FALSE);
			UpdateWindow(hwnd);

		}

	}	

}

static void StartAnimation(HWND hwnd, SENETBOARD *board, int from, int to)
{
	RECT fromrect;
	board->animating = 1;
	board->pawnmoving = MOVING_TOGO;
	senet_indextoxy(from, &board->selx, &board->sely);
	senet_indextoxy(to, &board->consider_x, &board->consider_y);
	CellToRect(hwnd, &fromrect, board->selx, board->sely);
	board->spritex = fromrect.left;
	board->spritey = fromrect.top;
	board->ticks = 0;
	board->timer_id = SetTimer(hwnd, 0, 100, 0);

}

static void ProcessTimer(HWND hwnd, SENETBOARD *board)
{
	double pathlen;
	RECT from;
	RECT to;
	double vx, vy;
	double pos;

	if (!board->animating)
	{
		return;
	}
	CellToRect(hwnd, &from, board->selx, board->sely);
	CellToRect(hwnd, &to, board->consider_x, board->consider_y);
	pathlen = sqrt((double) (from.top - to.top)*(from.top - to.top) + (from.left - to.left)*(from.left - to.left));
	vx = to.left - from.left;
	vy = to.top - from.top;
	vx /= pathlen;
	vy /= pathlen;
	pos = board->ticks * 6.0;
	if (board->pawnmoving == MOVING_TOGO && pos < pathlen)
	{
		board->spritex = (int) (from.left + vx * pos);
		board->spritey = (int) (from.top + vy * pos);
		InvalidateRect(hwnd, 0, FALSE);
		UpdateWindow(hwnd);
	}
	else if (board->pawnmoving == MOVING_TOGO && board->game->board[board->consider_y][board->consider_x])
	{
		board->pawnmoving = MOVING_OPPOSITE;
		board->ticks = 0;
		board->spritex = to.left;
		board->spritey = to.top;
		InvalidateRect(hwnd, 0, FALSE);
		UpdateWindow(hwnd);
	}
	else if (board->pawnmoving == MOVING_OPPOSITE && pos < pathlen)
	{
		board->spritex = (int)(to.left - vx * pos);
		board->spritey = (int)(to.top - vy * pos);
		InvalidateRect(hwnd, 0, FALSE);
		UpdateWindow(hwnd);

	}
	else
	{
		board->animating = 0;
		KillTimer(hwnd, board->timer_id);
		static MOVE move;
		move.from = senet_xytoindex(board->selx, board->sely);
		move.to = senet_xytoindex(board->consider_x, board->consider_y);
		senet_applymove(board->game, &move);
		board->selx = -1;
		board->sely = -1;
		board->consider_x = -1;
		board->consider_y = -1;
		InvalidateRect(hwnd, 0, TRUE);
		UpdateWindow(hwnd);
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM) &move);
	}
	board->ticks++;
}

static void WindowToCell(HWND hwnd, int x, int y, int *cx, int *cy)
{
	RECT big;
	int cwidth, cheight;

	GetClientRect(hwnd, &big);
	cwidth = big.right / 10;
	cheight = big.bottom / 3;
	*cx = clamp(x / cwidth, 0, 9);
	*cy = clamp(y / cheight, 0, 2);
	
}

static void CellToRect(HWND hwnd, RECT *rect, int cx, int cy)
{
	RECT big;
	int cwidth, cheight;

	GetClientRect(hwnd, &big);
	cwidth = big.right / 10;
	cheight = big.bottom / 3;
	rect->left = cwidth * cx;
	rect->right = rect->left + cwidth;
	rect->top = cheight * cy;
	rect->bottom = rect->top + cheight;
	if (cy == 0)
	{
		rect->top += 10;
		rect->bottom += 10;
	}
	if (cy == 2)
	{
		rect->top -= 20;
		rect->bottom -= 20;
	}
	rect->left += 2 * (10 - cx);
   rect->right += 2 * (10 - cx);
}


static HBITMAP makepieces(HWND hwnd)
{
	HDC hdc;
	HDC hdcbmp;
	HDC hdcdest;
	HBITMAP hanswer;
	HBITMAP hbitmap;
	HINSTANCE hInstance;
	BITMAP bm;
	int width, height;
	int red, green, blue;
	int x, y;

	hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = GetDC(hwnd);
	hbitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_PIECES), IMAGE_BITMAP, 0, 0, 0);
	GetObject(hbitmap, sizeof(BITMAP), &bm);
	width = bm.bmWidth;
	height = bm.bmHeight;
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, hbitmap);
	hdcdest = CreateCompatibleDC(hdc);
	hanswer = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdcdest, hanswer);
	for (y = 0; y < height;y++)
	for (x = 0; x < width; x++)
	{
		COLORREF col = GetPixel(hdcbmp, x, y);
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		if (blue > red && blue > green && max3(red, green, blue) > 70)
			col = RGB(255, 128, 128);
		SetPixel(hdcdest, x, y, col);
	}
	DeleteObject(hbitmap);
	DeleteDC(hdcbmp);
	DeleteDC(hdcdest);
	DeleteDC(hdc);

	return hanswer;
}

HBITMAP MakeBitmap(unsigned char *rgba, int width, int height)
{
	VOID *pvBits;          // pointer to DIB section 
	HBITMAP answer;
	BITMAPINFO bmi;
	HDC hdc;
	int x, y;
	int red, green, blue, alpha;

	// setup bitmap info   
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;         // four 8-bit components 
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = width * height * 4;

	hdc = CreateCompatibleDC(GetDC(0));
	answer = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			red = rgba[(y*width + x) * 4];
			green = rgba[(y*width + x) * 4 + 1];
			blue = rgba[(y*width + x) * 4 + 2];
			alpha = rgba[(y*width + x) * 4 + 3];
			red = (red * alpha) >> 8;
			green = (green * alpha) >> 8;
			blue = (blue * alpha) >> 8;
			((UINT32 *)pvBits)[y * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}
	}

	return answer;
}

static int max2(int a, int b)
{
	return a > b ? a : b;
}

static int max3(int a, int b, int c)
{
	return max2(a, max2(b, c));
}