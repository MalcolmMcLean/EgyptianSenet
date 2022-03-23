#include "Windows.h"
#include "resource.h"
#include "HieroglyphicsWin.h"

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
	wchar_t *str;
	HBITMAP htext;
	int tx;
	int ty;
	int twidth;
	int theight;
} SENETHIEROGLYPHICS;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static SENETHIEROGLYPHICS *CreateSenetHieroglyphics(HWND hwnd);
static void KillSenetHieroglyphics(SENETHIEROGLYPHICS *hy);
static void PaintMe(HWND hwnd, SENETHIEROGLYPHICS *hy);
static void SetString(HWND hwnd, SENETHIEROGLYPHICS *hy, wchar_t *str);
static HBITMAP MakeTextBitmap(struct bitmap_font *font, wchar_t *str, int *width, int *height);
static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height);

ATOM RegisterHieroglyphicsOKBut(HINSTANCE hInstance)
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
	wcex.lpszClassName = "HieroglyphicsOKBut";
	wcex.hIconSm = 0; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SENETHIEROGLYPHICS *hy;

	hy = (SENETHIEROGLYPHICS *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_CREATE:
		hy = CreateSenetHieroglyphics(hwnd);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) hy);
		break;
	case WM_COMMAND:
		break;
	case WM_PAINT:
		PaintMe(hwnd, hy);
		break;
	case WM_LBUTTONDOWN:
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), 0);
		break;
	case WM_SETTEXT:
		SetString(hwnd, hy, (wchar_t *)lParam);
		break;
	case WM_DESTROY:
		KillSenetHieroglyphics(hy);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

static SENETHIEROGLYPHICS *CreateSenetHieroglyphics(HWND hwnd)
{
	SENETHIEROGLYPHICS *answer;
	RECT rect;
	HINSTANCE hInstance;

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	GetClientRect(hwnd, &rect);
	answer = (SENETHIEROGLYPHICS *)malloc(sizeof(SENETHIEROGLYPHICS));
	answer->hbackground = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_HIEROBUTTON), IMAGE_BITMAP, rect.right, rect.bottom, 0);
	answer->htext = 0;
	answer->str = 0;
	answer->tx = 0;
	answer->ty = 0;
	answer->twidth = 0;
	answer->theight = 0;
	return answer;
}

static void KillSenetHieroglyphics(SENETHIEROGLYPHICS *hy)
{
	if (hy)
	{
		DeleteObject(hy->hbackground);
		DeleteObject(hy->htext);
		free(hy->str);
		free(hy);
	}
}

static void PaintMe(HWND hwnd, SENETHIEROGLYPHICS *hy)
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
	BLENDFUNCTION bf;

	GetClientRect(hwnd, &rect);

	hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	hdc = BeginPaint(hwnd, &ps);
	Rectangle(hdc, 0, 0, 20, 20);
	hdcbmp = CreateCompatibleDC(hdc);
	SelectObject(hdcbmp, hy->hbackground);
	GetObject(hy->hbackground, sizeof(BITMAP), &bm);
	StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcbmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	/*




	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;


	if (hy->htext)
	{
		SelectObject(hdcbmp, hy->htext);
		AlphaBlend(hdc, hy->tx, hy->ty, hy->twidth, hy->theight, hdcbmp, 0, 0, hy->twidth, hy->theight, bf);
	}
	*/

	DeleteDC(hdcbmp);
	EndPaint(hwnd, &ps);
}

static void SetString(HWND hwnd, SENETHIEROGLYPHICS *hy, wchar_t *str)
{
	int width, height;
	RECT rect;

	GetClientRect(hwnd, &rect);
	DeleteObject(hy->htext);
	hy->htext = MakeTextBitmap(&hieroglify_font, str, &width, &height);
	hy->twidth = width;
	hy->theight = height;
	hy->tx = (rect.right - rect.left - width) / 2;
	hy->ty = 8;
	InvalidateRect(hwnd, 0, FALSE);
	UpdateWindow(hwnd);
}


static HBITMAP MakeTextBitmap(struct bitmap_font *font, wchar_t *str, int *width, int *height)
{
	int w, h;
	unsigned char *rgba;
	int index;
	HBITMAP answer;
	int i, ii;
	int ix, iy;
	int x;

	if (str == 0 || str[0] == 0)
	{
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		return 0;
	}
	h = font->height;
	w = 0;
	for (i = 0; str[i]; i++)
	{
		for (ii = 0; ii < font->Nchars; ii++)
		if (font->index[ii] == str[i])
			break;
		if (ii == font->Nchars)
			ii = 0;
		w += font->widths[ii];
	}
	rgba = (unsigned char *)malloc(w * h * 4);
	if (!rgba)
	{
		if (*width)
			*width = 0;
		if (*height)
			*height = 0;
		return 0;
	}
	memset(rgba, 0, w*h * 4);

	x = 0;
	for (i = 0; str[i]; i++)
	{
		for (ii = 0; ii < font->Nchars; ii++)
		if (font->index[ii] == str[i])
			break;
		if (ii == font->Nchars)
			ii = 0;

		for (iy = 0; iy < font->height; iy++)
		for (ix = 0; ix < font->widths[ii]; ix++)
			rgba[(iy*w + x + ix) * 4 + 3] = font->bitmap[ii*font->width*font->height + iy *font->width + ix];

		x += font->widths[ii];
	}
	answer = MakeBitmap(rgba, w, h);
	free(rgba);
	if (width)
		*width = w;
	if (height)
		*height = h;

	return answer;
}

static HBITMAP MakeBitmap(unsigned char *rgba, int width, int height)
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
			((UINT32 *)pvBits)[(height - y - 1) * width + x] = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}
	}

	return answer;
}

