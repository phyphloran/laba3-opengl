// +++ main.cpp +++

//////////////////////////////////////////
//                                      //
// Учебные приложения по графике OpenGL //
// Труба с диалогом свойств             //
//                                      //
//        (с) РУТ(МИИТ), каф.САП        //
//                                      //
//////////////////////////////////////////


#define STRICT
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <tchar.h>

#include <gl\\gl.h>
#include <gl\\glu.h>


#pragma comment (linker, "/defaultlib:opengl32.lib")
#pragma comment (linker, "/defaultlib:glu32.lib")

#include "resource.h"

#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof(a[0]))
#endif

HINSTANCE g_hApp = nullptr;
LPCTSTR g_szAppName = _T("Восьмиугольная труба с диалогом свойств");

HWND g_hWindow = nullptr;
HDC g_hDC = nullptr;
HGLRC g_hGLRC = nullptr;
LPCTSTR g_szWndClass = _T("WcOglBoxpipe"),
g_szTitle = g_szAppName;

int g_wndWidth = -1, g_wndHeight = -1;

double g_angle = 0;
const double g_angIncr = 1;

GLUquadricObj* g_pGluQuadObj = nullptr;

#define M_PI 3.1415926


//
// свойства
//

struct Color
{
	LPCTSTR szName;
	COLORREF value;

	Color() : szName(_T("")), value(0) {}
	Color(LPCTSTR szName_, COLORREF value_) : szName(szName_), value(value_) {}
};

Color g_colors[] =
{
	Color(_T("- Красный"), RGB(255, 0,   0)),
	Color(_T("- Зелёный"), RGB(0,   255, 0)),
	Color(_T("- Жёлтый"),  RGB(255, 255, 0)),
};

struct BoxpipeProps
{
	double W, H, T, L, C;
	int iColor;

	BoxpipeProps() : W(15.), H(10.), T(1.), L(30.), C(3.), iColor(0) {}

	void CorrectValues()
	{
		if (W < 1e-5) W = 1e-5;
		if (H < 1e-5) H = 1e-5;
		if (L < 1e-5) L = 1e-5;
		if (T < 1e-5) T = 1e-5;
		if (T * 2 > W) T = W / 2;
		if (T * 2 > H) T = H / 2;

		double maxC = min(W / 2 - T, H / 2 - T);
		if (C < 1e-5) C = 1e-5;
		if (C > maxC)  C = maxC;

		if (iColor < 0 || iColor >= _countof(g_colors))
			iColor = 0;
	}
};

BoxpipeProps g_boxpipeProps;


// размер сцены
double g_sceneWidth = 50.;


LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);
BOOL MainOnCreate(HWND, LPCREATESTRUCT);
BOOL MainOnCommand(int, HWND, UINT);
BOOL MainOnSize(int width, int height);
BOOL MainOnPaint();
BOOL MainOnSize(UINT, int, int);
BOOL MainOnDestroy();

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL InitApp(void);
void UninitApp(void);

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int);


// установка формата пикселей
BOOL SetPixelFormat(HDC dc)
{
	PIXELFORMATDESCRIPTOR pfd;

	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 24;
	pfd.cStencilBits = 8;
	pfd.cDepthBits = 24;
	pfd.cAccumBits = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pf = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pf, &pfd);

	return !(pfd.dwFlags & PFD_NEED_PALETTE);
}  //SetPixelFormat

//
// рисование трубы (восьмиугольное сечение)
//
void DrawBoxpipe(bool fFill = true)
{
	float r = GetRValue(g_colors[g_boxpipeProps.iColor].value) / 255.f / 2,
		g = GetGValue(g_colors[g_boxpipeProps.iColor].value) / 255.f / 2,
		b = GetBValue(g_colors[g_boxpipeProps.iColor].value) / 255.f / 2;

	double w = g_boxpipeProps.W / 2,
		h = g_boxpipeProps.H / 2,
		l = g_boxpipeProps.L / 2,
		t = g_boxpipeProps.T,
		c = g_boxpipeProps.C;

	double w1 = w - t,
		h1 = h - t,
		c1 = c;

	const int N = 8;

	// Внешний контур
	double ox[N] = {w - c,  w,      w,      w, -w + c, -w,     -w,     -w};
	double oy[N] = {h,      h,      -h + c, -h, -h,     -h,      h - c,  h};

	// Внутренний восьмиугольник
	double ix[N] = { w1 - c1,  w1,       w1,       w1 - c1, -w1 + c1, -w1,      -w1,      -w1 + c1 };
	double iy[N] = { h1,       h1 - c1, -h1 + c1, -h1,      -h1,      -h1 + c1,  h1 - c1,  h1 };

	glEnable(GL_DEPTH_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, fFill ? GL_FILL : GL_LINE);

	// Внешние боковые грани
	glBegin(GL_QUADS);
	for (int i = 0; i < N; i++)
	{
		int j = (i + 1) % N;
		float br = (i % 2 == 0) ? 1.5f : 1.0f;
		glColor3f(r * br, g * br, b * br);
		glVertex3d(ox[i], oy[i], l);
		glVertex3d(ox[j], oy[j], l);
		glVertex3d(ox[j], oy[j], -l);
		glVertex3d(ox[i], oy[i], -l);
	}
	glEnd();

	// Внутренние боковые грани
	glBegin(GL_QUADS);
	for (int i = 0; i < N; i++)
	{
		int j = (i + 1) % N;
		float br = (i % 2 == 0) ? 0.5f : 1.0f;
		glColor3f(r * br, g * br, b * br);
		glVertex3d(ix[i], iy[i], l);
		glVertex3d(ix[i], iy[i], -l);
		glVertex3d(ix[j], iy[j], -l);
		glVertex3d(ix[j], iy[j], l);
	}
	glEnd();

	// Передний торец
	glColor3f(0.5f, 0.5f, 0.5f);
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= N; i++)
	{
		int k = i % N;
		glVertex3d(ox[k], oy[k], l);
		glVertex3d(ix[k], iy[k], l);
	}
	glEnd();

	// Задний торец
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= N; i++)
	{
		int k = i % N;
		glVertex3d(ix[k], iy[k], -l);
		glVertex3d(ox[k], oy[k], -l);
	}
	glEnd();
}


//
// штриховка торцевого сечения
// zPos — позиция торца по оси Z (передний: +l, задний: -l, проекция: 0)
//
void DrawHatching(double zPos = 0.0)
{
	double w = g_boxpipeProps.W / 2,
		h = g_boxpipeProps.H / 2,
		t = g_boxpipeProps.T,
		c = g_boxpipeProps.C;

	double w1 = w - t,
		h1 = h - t,
		c1 = c;

	const int N = 8;

	// Внешний контур
	double ox[N] = { w - c,  w,      w,      w, -w + c, -w,     -w,     -w };
	double oy[N] = {h,      h,      -h + c, -h, -h,     -h,      h - c,  h};

	// Внутренний восьмиугольник
	double ix[N] = { w1 - c1,  w1,       w1,       w1 - c1, -w1 + c1, -w1,      -w1,      -w1 + c1 };
	double iy[N] = { h1,       h1 - c1, -h1 + c1, -h1,      -h1,      -h1 + c1,  h1 - c1,  h1 };

	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0, 0, zPos);
	for (int i = 0; i <= N; i++)
		glVertex3d(ox[i % N], oy[i % N], zPos);
	glEnd();

	glStencilFunc(GL_ALWAYS, 0, 0xFF);

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0, 0, zPos);
	for (int i = 0; i <= N; i++)
		glVertex3d(ix[i % N], iy[i % N], zPos);
	glEnd();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glColor3f(0.15f, 0.15f, 0.15f);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	double step = 1.2;
	double ext = w + h;

	glBegin(GL_LINES);
	for (double d = -ext; d <= ext; d += step)
	{
		glVertex3d(-ext, d - ext, zPos);
		glVertex3d(ext, d + ext, zPos);
	}
	glEnd();

	glStencilFunc(GL_ALWAYS, 0, 0xFF);
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(1.5f);

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < N; i++)
		glVertex3d(ox[i], oy[i], zPos);
	glEnd();

	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < N; i++)
		glVertex3d(ix[i], iy[i], zPos);
	glEnd();

	glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(1.0f);
}


//
// рисование осей
//
void DrawAxes(double dAxisSize)
{
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_LINE);
	gluQuadricDrawStyle(g_pGluQuadObj, GLU_FILL);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_LINES);
	glColor3d(1, 0, 0);
	glVertex3d(-dAxisSize / 2, 0, 0);
	glVertex3d(dAxisSize, 0, 0);
	glColor3d(0, 1, 0);
	glVertex3d(0, -dAxisSize / 2, 0);
	glVertex3d(0, dAxisSize, 0);
	glColor3d(0, 0, 1);
	glVertex3d(0, 0, -dAxisSize / 2);
	glVertex3d(0, 0, dAxisSize);
	glEnd();

	glPushMatrix();
	glColor3d(1, 0, 0);
	glTranslated(dAxisSize, 0, 0);
	glRotated(90, 0, 1, 0);
	gluCylinder(g_pGluQuadObj, dAxisSize / 10, 0, dAxisSize / 5, 32, 1);
	glPopMatrix();

	glPushMatrix();
	glColor3d(0, 1, 0);
	glTranslated(0, dAxisSize, 0);
	glRotated(-90, 1, 0, 0);
	gluCylinder(g_pGluQuadObj, dAxisSize / 10, 0, dAxisSize / 5, 32, 1);
	glPopMatrix();

	glPushMatrix();
	glColor3d(0, 0, 1);
	glTranslated(0, 0, dAxisSize);
	gluCylinder(g_pGluQuadObj, dAxisSize / 10, 0, dAxisSize / 5, 32, 1);
	glPopMatrix();

}  //DrawAxes

//
// рисование
//
void Draw()
{
	double maxWH = g_boxpipeProps.W;
	if (maxWH < g_boxpipeProps.H)
		maxWH = g_boxpipeProps.H;

	double minWHL = g_boxpipeProps.W;
	if (minWHL > g_boxpipeProps.H)
		minWHL = g_boxpipeProps.H;
	if (minWHL > g_boxpipeProps.L)
		minWHL = g_boxpipeProps.L;

	double maxWHL = g_boxpipeProps.L;
	if (maxWHL < g_boxpipeProps.W)
		maxWHL = g_boxpipeProps.W;
	if (maxWHL < g_boxpipeProps.H)
		maxWHL = g_boxpipeProps.H;

	double l = g_boxpipeProps.L / 2;

	GLsizei viewportSize = g_wndHeight;

	glViewport(0, 0, viewportSize, viewportSize);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-g_sceneWidth / 2, g_sceneWidth / 2,
		-g_sceneWidth / 2, g_sceneWidth / 2,
		-g_sceneWidth / 2, g_sceneWidth / 2);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// ── Главный 3D вид с штриховкой на обоих торцах ───────────────────
	glPushMatrix();

	glRotated(30., 1., 0., 0.);
	glRotated(g_angle, 0., 1., 0.);

	DrawBoxpipe();

	// штриховка на переднем торце (+l)
	DrawHatching(l);
	// штриховка на заднем торце (-l)
	DrawHatching(-l);

	DrawAxes(minWHL / 2);

	glPopMatrix();

	glViewport(viewportSize, 0, viewportSize, viewportSize);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double gap = maxWH / 10,
		size = maxWH + maxWHL + gap;

	double scw2 = size / 2 * 1.2;
	glOrtho(-scw2, scw2, -scw2, scw2, -scw2, scw2);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	double _05size = size / 2,
		_05maxWH = maxWH / 2,
		_05L = maxWHL / 2;

	// ── Проекция 1: вид спереди (торец) — штриховка в z=0 ────────────
	glPushMatrix();
	glTranslated(-_05size + _05maxWH, _05size - _05maxWH, 0.);
	DrawBoxpipe();
	DrawHatching(0.0);
	DrawAxes(minWHL / 2);
	glPopMatrix();

	// ── Проекция 2: вид сбоку ─────────────────────────────────────────
	glPushMatrix();
	glTranslated(-_05size + _05maxWH, _05size - maxWH - _05L - gap, 0.);
	glRotated(90., 1., 0., 0.);
	DrawBoxpipe();
	DrawAxes(minWHL / 2);
	glPopMatrix();

	// ── Проекция 3: вид сверху ────────────────────────────────────────
	glPushMatrix();
	glTranslated(-_05size + maxWH + _05L + gap, _05size - _05maxWH, 0.);
	glRotated(90., 0., 1., 0.);
	DrawBoxpipe();
	DrawAxes(minWHL / 2);
	glPopMatrix();

	// ── Проекция 4: аксонометрия (wireframe) ──────────────────────────
	glPushMatrix();
	glTranslated(-_05size + maxWH + _05L + gap, _05size - maxWH - _05L - gap, 0.);
	glRotated(30., 1., 0., 0.);
	glRotated(int(g_angle / 22.5) * 22.5, 0, -1., 0.);
	glScaled(0.7, 0.7, 0.7);
	DrawBoxpipe(false);
	DrawAxes(minWHL * 0.7 / 2);
	glPopMatrix();

	glFinish();
	SwapBuffers(g_hDC);
}  //Draw


LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		return MainOnCreate(hwnd, (LPCREATESTRUCT)lParam);

	case WM_SIZE:
		return MainOnSize(LOWORD(lParam), HIWORD(lParam));

	case WM_PAINT:
		return MainOnPaint();

	case WM_DESTROY:
		return MainOnDestroy();

	case WM_TIMER:
		if ((g_angle = g_angle + g_angIncr) >= 360)
			g_angle -= 360;
		InvalidateRect(g_hWindow, nullptr, FALSE);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0L;
}  //MainWindowProc

//
// WM_CREATE
//
BOOL MainOnCreate(HWND hwnd, LPCREATESTRUCT p_cs)
{
	g_hDC = GetDC(hwnd);

	SetPixelFormat(g_hDC);

	g_hGLRC = wglCreateContext(g_hDC);
	wglMakeCurrent(g_hDC, g_hGLRC);

	SetTimer(hwnd, 0, 10, 0);

	g_pGluQuadObj = gluNewQuadric();
	assert(g_pGluQuadObj);

	CreateDialog(g_hApp, MAKEINTRESOURCE(IDD_PROPS), hwnd, DlgProc);

	return TRUE;
}  //MainOnCreate

//
// WM_SIZE
//
BOOL MainOnSize(int width, int height)
{
	g_wndWidth = width;
	g_wndHeight = height;
	return TRUE;
}

//
// WM_PAINT
//
BOOL MainOnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(g_hWindow, &ps);
	Draw();
	EndPaint(g_hWindow, &ps);
	return TRUE;
}


BOOL MainOnDestroy()
{
	gluDeleteQuadric(g_pGluQuadObj);
	g_pGluQuadObj = nullptr;

	wglMakeCurrent(nullptr, nullptr);
	if (g_hGLRC)
		wglDeleteContext(g_hGLRC);

	PostQuitMessage(0);

	return TRUE;
}


void SetDlgItemReal(HWND hDlg, int id, double val)
{
	TCHAR buff[256];
	_stprintf(buff, _T("%g"), val);
	SetDlgItemText(hDlg, id, buff);
}

bool GetDlgItemReal(HWND hDlg, int id, double& val)
{
	TCHAR buff[512];
	GetDlgItemText(hDlg, id, buff, _countof(buff));

	LPTSTR err = nullptr;
	val = _tcstod(buff, &err);
	return nullptr == err;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool s_fInit;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		s_fInit = true;

		SetDlgItemReal(hDlg, IDC_ED_W, g_boxpipeProps.W);
		SetDlgItemReal(hDlg, IDC_ED_H, g_boxpipeProps.H);
		SetDlgItemReal(hDlg, IDC_ED_T, g_boxpipeProps.T);
		SetDlgItemReal(hDlg, IDC_ED_L, g_boxpipeProps.L);
		SetDlgItemReal(hDlg, IDC_ED_C, g_boxpipeProps.C);

		for (int i = 0; i < _countof(g_colors); i++)
			SendDlgItemMessage(hDlg, IDC_LB_COLORS, LB_ADDSTRING, 0, (LPARAM)g_colors[i].szName);

		SendDlgItemMessage(hDlg, IDC_LB_COLORS, LB_SETCURSEL, g_boxpipeProps.iColor, 0);

		s_fInit = false;
	}
	return (INT_PTR)TRUE;

	case WM_COMMAND:
	{
		if (s_fInit)
			return (INT_PTR)FALSE;

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		if (HIWORD(wParam) == EN_CHANGE
			|| HIWORD(wParam) == LBN_SELCHANGE)
		{
			GetDlgItemReal(hDlg, IDC_ED_W, g_boxpipeProps.W);
			GetDlgItemReal(hDlg, IDC_ED_H, g_boxpipeProps.H);
			GetDlgItemReal(hDlg, IDC_ED_T, g_boxpipeProps.T);
			GetDlgItemReal(hDlg, IDC_ED_L, g_boxpipeProps.L);
			GetDlgItemReal(hDlg, IDC_ED_C, g_boxpipeProps.C);

			g_boxpipeProps.iColor = SendDlgItemMessage(hDlg, IDC_LB_COLORS, LB_GETCURSEL, 0, 0);

			g_boxpipeProps.CorrectValues();

			InvalidateRect(g_hWindow, nullptr, FALSE);
		}
	}
	break;

	case WM_CLOSE:
		DestroyWindow(g_hWindow);
		break;
	}
	return (INT_PTR)FALSE;
}


BOOL InitApp()
{
	WNDCLASSEX wce;
	ZeroMemory(&wce, sizeof(WNDCLASSEX));
	wce.cbSize = sizeof(WNDCLASSEX);
	wce.hInstance = g_hApp;
	wce.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wce.lpfnWndProc = MainWindowProc;
	wce.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wce.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wce.lpszClassName = g_szWndClass;
	if (!RegisterClassEx(&wce))
		return FALSE;

	SetLastError(0);
	g_hWindow = CreateWindow(g_szWndClass, g_szTitle,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
		| WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		50, 50, 900, 400, nullptr, nullptr, g_hApp, nullptr);
	if (!g_hWindow)
	{
		DWORD err = GetLastError();
		return FALSE;
	}

	ShowWindow(g_hWindow, SW_SHOW);
	UpdateWindow(g_hWindow);

	return TRUE;
}


void UninitApp()
{
	UnregisterClass(g_szWndClass, g_hApp);
}


int APIENTRY WinMain(HINSTANCE hApp_, HINSTANCE, LPSTR, int)
{
	g_hApp = hApp_;

	if (InitApp())
	{
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UninitApp();

	return 0;
}

// --- main.cpp ---