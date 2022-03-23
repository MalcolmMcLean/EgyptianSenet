#ifndef senestickswin_h
#define senetstickswin_h

#define STICK1 1
#define STICK2 2
#define STICK3 4
#define STICK4 8

#define WM_SETSTICKS (WM_USER +100) /* Wparam = stick state)*/
ATOM RegisterSenetSticksWin(HINSTANCE hInstance);

#endif