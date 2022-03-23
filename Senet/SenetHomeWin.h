#ifndef senethomewin_h
#define senethomewin_h

ATOM RegisterSenetHomeWin(HINSTANCE hInstance);

/* wParam - Nwhite, lParam, Nblack */
#define WM_SETPAWNS (WM_USER + 100)
#define WM_SHOWBUTTON (WM_USER + 101)
#define WM_HIDEBUTTON (WM_USER + 102)

#endif
