#include "Windows.h"
#include "resource.h"
#include "SenetHomeWin.h"

#define ID_CONTINUE_BUT 1000

/* bitmap font structure */
struct bitmap_font {
	unsigned char width;         /* max. character width */
	unsigned char height;        /* character height */
	int ascent;                  /* font ascent */
	int descent;                 /* font descent */
	unsigned short Nchars;       /* number of characters in font */
	unsigned char *widths;       /* width of each character */
	unsigned short *index;       /* encoding to character index */
	unsigned char *bitmap;       /* bitmap of all characters */
};

extern "C" { extern struct bitmap_font hieroglify_font; };

typedef struct
{
	HBITMAP hbackground;
	HBITMAP hpieces;
	int Nwhite;
	int Nblack;
	HWND hbutton;
} SENETHOME;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static SENETHOME *CreateSenetHome(HWND hwnd);
static void KillSenetHome(SENETHOME *home);
static void PaintMe(HWND hwnd, SENETHOME *home);
static void SetPawns(HWND hwnd, SENETHOME *home, int Nwhite, int Nblack);
static void GetPawnRect(HWND hwnd, RECT *rect, int colour, int index);
static HBITMAP makepieces(HWND hwnd);
static int max2(int a, int b);
static int max3(int a, int b, int c);

ATOM RegisterSenetHomeWin(HINSTANCE hInstance)
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
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wcex.lpszMenuName = 0; // MAKEINTRESOURCE(IDC_SENET);
	wcex.lpszClassName = "SenetHomeWin";
	wcex.hIconSm = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SENETHOME *home;
	int adj;

	home = (SENETHOME *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_CREATE:
		home = CreateSenetHome(hwnd);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)home);
		break;
	case WM_COMMAND:
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), 0);
		break;
	case WM_PAINT:
		PaintMe(hwnd, home);
		break;
	case WM_SETPAWNS:
		SetPawns(hwnd, home, wParam, lParam);
		break;
	case WM_SHOWBUTTON:
		if (home->Nblack == 7)
			adj = 50;
		else if (home->Nwhite == 7)
			adj = -50;
		else
			adj = 0;
		home->hbutton = CreateWindow("HieroglyphicsOKBut",
			"",
			WS_CHILD | WS_VISIBLE,
			721 / 2 - 30 + adj, 20, 60, 30,
			hwnd,
			(HMENU)ID_CONTINUE_BUT,
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
			NULL);
		ShowWindow(home->hbutton, TRUE);
		break;
	case WM_HIDEBUTTON:
		DestroyWindow(home->hbutton);
		home->hbutton = 0;
		InvalidateRect(hwnd, 0, FALSE);
		UpdateWindow(hwnd);
		break;
	case WM_DESTROY:
		KillSenetHome(home);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

static SENETHOME *CreateSenetHome(HWND hwnd)
{
	SENETHOME *answer;
	RECT rect;
	int i;
	HINSTANCE hInstance;

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	GetClientRect(hwnd, &rect);
	answer = (SENETHOME *)malloc(sizeof(SENETHOME));
	answer->hbackground = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_HIEROGLYPHS_BG), IMAGE_BITMAP, rect.right, 0, 0);
	answer->hpieces = makepieces(hwnd);	
	answer->Nblack = 0;
	answer->Nwhite = 0;

	answer->hbutton = CreateWindow("HieroglyphicsOKBut",
		"",
		WS_CHILD | WS_VISIBLE,
		721 / 2 - 30, 20, 60, 30,
		hwnd,
		(HMENU)ID_CONTINUE_BUT,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		NULL);

	ShowWindow(answer->hbutton, TRUE);

	return answer;
}

static void KillSenetHome(SENETHOME *home)
{
	if (home)
	{
		DeleteObject(home->hbackground);
		DeleteObject(home->hpieces);
		free(home);
	}
}

static void PaintMe(HWND hwnd, SENETHOME *home)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HINSTANCE hInstance;
	HBITMAP hbitmap;
	HDC hdcbmp;
	BITMAP bm;
	RECT rect;
	RECT crect;
	int i;

	GetClientRect(hwnd, &rect);

	

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = BeginPaint(hwnd, &ps);
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, home->hbackground);
	GetObject(home->hbackground, sizeof(BITMAP), &bm);
	StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcbmp, 0, 0, bm.bmWidth, (rect.bottom *rect.right)/bm.bmWidth, SRCCOPY);

	SelectObject(hdcbmp, home->hpieces);
	GetObject(home->hpieces, sizeof(BITMAP), &bm);

	/*
	for (int y = 0; y < hieroglify_font.height; y++)
		for (int x = 0; x < hieroglify_font.width; x++)
		if (hieroglify_font.bitmap[(hieroglify_font.width*hieroglify_font.height * 14) + y*hieroglify_font.width + x])
			SetPixel(hdc, x, y, RGB(0, 0, 0));
			*/

	for (i = 0; i < home->Nwhite; i++)
	{
		GetPawnRect(hwnd, &crect, 1, i);
			TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
			hdcbmp, 130, 0, bm.bmWidth - 130, bm.bmHeight, RGB(255, 128, 128));
			
	}

	for (i = 0; i < home->Nblack; i++)
	{
		GetPawnRect(hwnd, &crect, 0, i);
		TransparentBlt(hdc, crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top,
			hdcbmp, 0, 0, 130, bm.bmHeight, RGB(255, 128, 128));
	}
	DeleteDC(hdcbmp);
	EndPaint(hwnd, &ps);
}

static void SetPawns(HWND hwnd, SENETHOME *home, int Nwhite, int Nblack)
{
	if (home->Nwhite == Nwhite && home->Nblack == Nblack)
		return;
	home->Nwhite = Nwhite;
	home->Nblack = Nblack;
	InvalidateRect(hwnd, 0, FALSE);
	UpdateWindow(hwnd);
}

static void GetPawnRect(HWND hwnd, RECT *rect, int colour, int index)
{
	RECT winrect;
	int cwidth;

	GetClientRect(hwnd, &winrect);
	cwidth = (winrect.right - winrect.left) / 14;

	if (colour == 0)
	{
		rect->left = winrect.left + index * cwidth;
		rect->right = rect->left + cwidth;
		rect->top = winrect.top;
		rect->bottom = winrect.bottom;
	}
	else
	{
		rect->left = winrect.left + (7-index+6)* cwidth;
		rect->right = rect->left + cwidth;
		rect->top = winrect.top;
		rect->bottom = winrect.bottom;
	}
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

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
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
	for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	{
		COLORREF col = GetPixel(hdcbmp, x, y);
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		if (blue > red && blue > green && max3(red, green, blue) > 70)
			col = RGB(255, 128, 128);
		else if (x < width / 2 && red < 50)
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