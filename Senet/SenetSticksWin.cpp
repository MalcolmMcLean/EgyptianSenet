#include <Windows.h>
#include "Resource.h"

extern "C" {
#include "senetgame.h"
};
#include "SenetSticksWin.h"

typedef struct
{
	int side[4];
	int stick_x[4];
	int stick_y[4];
	HBITMAP hfront;
	HBITMAP hback;
	HBITMAP htile;
	SENETGAME *game;
} SENETSTICKS;


static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static SENETSTICKS *CreateSenetSticks(HWND hwnd);
static void KillSenetSticks(SENETSTICKS *sticks);
static int SetSticks(HWND hwnd, SENETSTICKS *sticks, int mask);
static void PaintMe(HWND hwnd, SENETSTICKS *sticks);
static HBITMAP makefrontstick(HWND hwnd);
static HBITMAP makebackstick(HWND hwnd);
static int max2(int a, int b);
static int max3(int a, int b, int c);


ATOM RegisterSenetSticksWin(HINSTANCE hInstance)
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
	wcex.lpszClassName = "SenetSticksWin";
	wcex.hIconSm = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SENETSTICKS *sticks;

	sticks = (SENETSTICKS *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_CREATE:
		sticks = CreateSenetSticks(hwnd);
		sticks->game = (SENETGAME *)(((CREATESTRUCT *)lParam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sticks);
		break;
	case WM_COMMAND:
		break;
	case WM_PAINT:
		PaintMe(hwnd, sticks);
		break;
	case WM_LBUTTONDOWN:
		if (sticks->game->state == SENET_WAITING_ROLL1)
			SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), 0);
		break;
	case WM_SETSTICKS:
		SetSticks(hwnd, sticks, wParam);
		break;
	case WM_DESTROY:
		KillSenetSticks(sticks);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

static SENETSTICKS *CreateSenetSticks(HWND hwnd)
{
	SENETSTICKS *answer;
	RECT rect;
	int i;
	HINSTANCE hInstance;

	hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	GetClientRect(hwnd, &rect);
	answer = (SENETSTICKS *) malloc(sizeof(SENETSTICKS));
	answer->htile = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_TILE), IMAGE_BITMAP, rect.bottom, rect.right, 0);
	answer->hfront = makefrontstick(hwnd);
	answer->hback = makebackstick(hwnd);
	for (i = 0; i < 4; i++)
	{
		answer->stick_x[i] = rand() % 50;
		answer->stick_y[i] = i * 30;
		answer->side[i] = i % 2;
	}

	return answer;
}

static void KillSenetSticks(SENETSTICKS *sticks)
{
	if (sticks)
	{
		DeleteObject(sticks->htile);
		DeleteObject(sticks->hfront);
		DeleteObject(sticks->hback);
		free(sticks);
	}

}

static int SetSticks(HWND hwnd, SENETSTICKS *sticks, int mask)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		sticks->side[i] = (mask & (1 << i)) ? 1 : 0;
		sticks->stick_x[i] = rand() % 50;
		sticks->stick_y[i] = i * 30;
	}
	InvalidateRect(hwnd, 0, TRUE);
	UpdateWindow(hwnd);

	return 0;
}

static void PaintMe(HWND hwnd, SENETSTICKS *sticks)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HINSTANCE hInstance;
	HBITMAP hbitmap;
	HDC hdcbmp;
	BITMAP bm;
	RECT rect;
	int i;

	GetClientRect(hwnd, &rect);

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = BeginPaint(hwnd, &ps);
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, sticks->htile);
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcbmp, 0, 0, SRCCOPY);

	for (i = 0; i < 4; i++)
	{
		hbitmap = sticks->side[i] ? sticks->hfront : sticks->hback;
		GetObject(hbitmap, sizeof(BITMAP), &bm);
		SelectObject(hdcbmp, hbitmap);
		TransparentBlt(hdc, 
			sticks->stick_x[i], sticks->stick_y[i], 
			200, (200 * bm.bmHeight)/bm.bmWidth, hdcbmp, 
			0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 128, 128));
	}

	DeleteDC(hdcbmp);
	EndPaint(hwnd, &ps);
}

static HBITMAP makefrontstick(HWND hwnd)
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

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = GetDC(hwnd);
	hbitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_STICK_FRONT), IMAGE_BITMAP, 0, 0, 0);
	GetObject(hbitmap, sizeof(BITMAP), &bm);
	width = bm.bmWidth;
	height = bm.bmHeight;
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, hbitmap);
	hdcdest = CreateCompatibleDC(hdc);
	hanswer = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdcdest, hanswer);
	for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	{
		COLORREF col = GetPixel(hdcbmp, x, y);
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		if ( (red > blue && red > green && red - max2(blue, green) > 100)  || x > width - 20)
			col = RGB(255, 128, 128);
		SetPixel(hdcdest, x, y, col);
	}
	DeleteObject(hbitmap);
	DeleteDC(hdcbmp);
	DeleteDC(hdcdest);
	DeleteDC(hdc);

	return hanswer;
}

static HBITMAP makebackstick(HWND hwnd)
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

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = GetDC(hwnd);
	hbitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_STICK_BACK), IMAGE_BITMAP, 0, 0, 0);
	GetObject(hbitmap, sizeof(BITMAP), &bm);
	width = bm.bmWidth;
	height = bm.bmHeight;
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, hbitmap);
	hdcdest = CreateCompatibleDC(hdc);
	hanswer = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdcdest, hanswer);
	for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	{
		COLORREF col = GetPixel(hdcbmp, x, y);
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		if ((red > blue && red > green && red - max2(blue, green) > 100) || x > width - 20)
			col = RGB(255, 128, 128);
		SetPixel(hdcdest, x, y, col);
	}
	DeleteObject(hbitmap);
	DeleteDC(hdcbmp);
	DeleteDC(hdcdest);
	DeleteDC(hdc);

	return hanswer;
}


static int max2(int a, int b)
{
	return a > b ? a : b;
}

static int max3(int a, int b, int c)
{
	return max2(a, max2(b, c));
}