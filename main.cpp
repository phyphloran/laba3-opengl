#define STRICT
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>

#include <math.h>
#include <vector>
#include <tchar.h>
#include <gl\gl.h>
#include <gl\glu.h>

#pragma comment(linker, "/defaultlib:opengl32.lib")
#pragma comment(linker, "/defaultlib:glu32.lib")
#pragma execution_character_set("utf-8")

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3
{
    double x, y, z;
};

static Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vec3 operator*(const Vec3& a, double s) { return { a.x * s, a.y * s, a.z * s }; }

static double Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

static double Length(const Vec3& v) { return sqrt(Dot(v, v)); }

static Vec3 Normalize(const Vec3& v)
{
    const double len = Length(v);
    if (len < 1e-9) return { 0.0, 0.0, 0.0 };
    return v * (1.0 / len);
}

HINSTANCE g_hApp = nullptr;
HWND g_hWindow = nullptr;
HDC g_hDC = nullptr;
HGLRC g_hGLRC = nullptr;

const wchar_t* g_szWndClass = L"WcVariant2";
const wchar_t* g_szTitle = L"Var 2: Pyramid and cube";

int g_wndWidth = 800;
int g_wndHeight = 800;

std::vector<Vec3> g_base;
Vec3 g_apex = { 0.0, 0.0, 7.0 };
std::vector<std::pair<Vec3, Vec3>> g_edges;
int g_baseVertexCount = 5;

int g_edgeIndex = 0;
double g_spinDeg = 0.0;
double g_slide = 0.2;   // положение куба вдоль ребра (0..1)
double g_cubeSize = 1.8;

double g_cameraYaw = -35.0;
double g_cameraPitch = 20.0;
bool g_isDraggingCamera = false;
POINT g_lastMousePos = { 0, 0 };

constexpr double kFovYDeg = 45.0;
constexpr double kLookAtZ = 2.5;


void BuildPyramid()
{
    g_base.clear();
    g_edges.clear();

    const int n = g_baseVertexCount;
    const double r = 6.0;
    for (int i = 0; i < n; ++i)
    {
        const double a = 2.0 * M_PI * i / n + M_PI * 0.12;
        g_base.push_back({ r * cos(a), r * sin(a), 0.0 });
    }

    // 5 рёбер основания
    for (int i = 0; i < n; ++i)
    {
        g_edges.push_back({ g_base[i], g_base[(i + 1) % n] });
    }

    // 5 боковых рёбер
    for (int i = 0; i < n; ++i)
    {
        g_edges.push_back({ g_base[i], g_apex });
    }

    if (g_edgeIndex >= (int)g_edges.size())
    {
        g_edgeIndex = 0;
    }
}

double ComputeSceneRadius()
{
    double maxRadius = 0.0;

    auto includePoint = [&](const Vec3& p)
    {
        const Vec3 shifted = { p.x, p.y, p.z - kLookAtZ };
        const double r = Length(shifted);
        if (r > maxRadius) maxRadius = r;
    };

    includePoint(g_apex);
    for (const Vec3& p : g_base)
        includePoint(p);

    // Учитываем диагональ куба, чтобы он не обрезался на крайних положениях.
    maxRadius += g_cubeSize * sqrt(3.0);

    return maxRadius;
}

double ComputeAutoCameraDistance(double aspect)
{
    if (aspect <= 0.0) aspect = 1.0;

    const double radius = ComputeSceneRadius();
    const double fovYRad = kFovYDeg * M_PI / 180.0;
    const double fovXRad = 2.0 * atan(tan(fovYRad * 0.5) * aspect);
    const double limitingFov = (aspect < 1.0) ? fovXRad : fovYRad;

    // Небольшой запас, чтобы модель помещалась при вращении.
    const double fitRadius = radius * 1.08;
    return fitRadius / tan(limitingFov * 0.5);
}

BOOL SetPixelFormatForGL(HDC dc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pf = ChoosePixelFormat(dc, &pfd);
    if (pf == 0) return FALSE;
    if (!SetPixelFormat(dc, pf, &pfd)) return FALSE;
    return TRUE;
}

void DrawPyramid()
{
    glEnable(GL_DEPTH_TEST);

    // Боковые грани
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < (int)g_base.size(); ++i)
    {
        const Vec3& a = g_base[i];
        const Vec3& b = g_base[(i + 1) % g_base.size()];
        glColor3f(0.1f, 0.6f + 0.05f * (float)(i % 2), 0.3f);
        glVertex3d(a.x, a.y, a.z);
        glVertex3d(b.x, b.y, b.z);
        glVertex3d(g_apex.x, g_apex.y, g_apex.z);
    }
    glEnd();

    // Основание
    glColor3f(0.05f, 0.4f, 0.2f);
    glBegin(GL_POLYGON);
    for (int i = (int)g_base.size() - 1; i >= 0; --i)
        glVertex3d(g_base[i].x, g_base[i].y, g_base[i].z);
    glEnd();

    // Каркас
    glLineWidth(2.0f);
    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < g_edges.size(); ++i)
    {
        glVertex3d(g_edges[i].first.x, g_edges[i].first.y, g_edges[i].first.z);
        glVertex3d(g_edges[i].second.x, g_edges[i].second.y, g_edges[i].second.z);
    }
    glEnd();
}

void DrawUnitCube()
{
    glBegin(GL_QUADS);

    glColor3f(0.75f, 0.1f, 0.2f); // +X
    glVertex3d(1, -1, -1); glVertex3d(1, 1, -1); glVertex3d(1, 1, 1); glVertex3d(1, -1, 1);

    glColor3f(0.65f, 0.08f, 0.18f); // -X
    glVertex3d(-1, -1, -1); glVertex3d(-1, -1, 1); glVertex3d(-1, 1, 1); glVertex3d(-1, 1, -1);

    glColor3f(0.85f, 0.2f, 0.3f); // +Y
    glVertex3d(-1, 1, -1); glVertex3d(-1, 1, 1); glVertex3d(1, 1, 1); glVertex3d(1, 1, -1);

    glColor3f(0.6f, 0.05f, 0.15f); // -Y
    glVertex3d(-1, -1, -1); glVertex3d(1, -1, -1); glVertex3d(1, -1, 1); glVertex3d(-1, -1, 1);

    glColor3f(0.8f, 0.15f, 0.25f); // +Z
    glVertex3d(-1, -1, 1); glVertex3d(1, -1, 1); glVertex3d(1, 1, 1); glVertex3d(-1, 1, 1);

    glColor3f(0.55f, 0.04f, 0.12f); // -Z
    glVertex3d(-1, -1, -1); glVertex3d(-1, 1, -1); glVertex3d(1, 1, -1); glVertex3d(1, -1, -1);

    glEnd();
}

void DrawCubeOnEdge()
{
    if (g_edges.empty()) return;

    const auto& edge = g_edges[g_edgeIndex % g_edges.size()];
    const Vec3 p0 = edge.first;
    const Vec3 p1 = edge.second;

    Vec3 dir = p1 - p0;
    const double edgeLen = Length(dir);
    if (edgeLen < 1e-6) return;

    Vec3 axis = Normalize(dir);

    // Локальное ребро куба от (-1,-1,-1) до (1,-1,-1), длина после масштаба = 2*g_cubeSize
    const double cubeEdgeLen = 2.0 * g_cubeSize;
    double maxStart = edgeLen - cubeEdgeLen;
    if (maxStart < 0.0) maxStart = 0.0;

    const double startOffset = g_slide * maxStart;
    const Vec3 anchorStart = p0 + axis * startOffset;
    const Vec3 anchorMid = anchorStart + axis * (cubeEdgeLen * 0.5);

    // Повернуть локальную ось X в направление axis
    const Vec3 ex = { 1.0, 0.0, 0.0 };
    const double c = Dot(ex, axis);
    Vec3 rotAxis = Cross(ex, axis);
    double rotAngle = 0.0;

    if (Length(rotAxis) < 1e-8)
    {
        if (c < 0.0)
        {
            rotAxis = { 0.0, 1.0, 0.0 };
            rotAngle = 180.0;
        }
    }
    else
    {
        rotAxis = Normalize(rotAxis);
        rotAngle = acos(c) * 180.0 / M_PI;
    }

    glPushMatrix();

    glTranslated(anchorMid.x, anchorMid.y, anchorMid.z);

    if (rotAngle != 0.0)
        glRotated(rotAngle, rotAxis.x, rotAxis.y, rotAxis.z);

    // Вращение куба относительно ребра пирамиды (ось X после выравнивания)
    glRotated(g_spinDeg, 1.0, 0.0, 0.0);

    glScaled(g_cubeSize, g_cubeSize, g_cubeSize);
    // Сместить куб так, чтобы ребро (-1,-1,-1) -> (1,-1,-1) проходило через локальный origin.
    // Тогда после выравнивания и переноса anchorMid именно это ребро ляжет на ребро пирамиды.
    glTranslated(0.0, 1.0, 1.0);
    DrawUnitCube();

    glPopMatrix();

    // Подсветить активное ребро
    glLineWidth(4.0f);
    glColor3f(1.0f, 0.8f, 0.1f);
    glBegin(GL_LINES);
    glVertex3d(p0.x, p0.y, p0.z);
    glVertex3d(p1.x, p1.y, p1.z);
    glEnd();
}

void DrawScene()
{
    glViewport(0, 0, g_wndWidth, g_wndHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const double aspect = (g_wndHeight > 0) ? (double)g_wndWidth / (double)g_wndHeight : 1.0;
    const double cameraDistance = ComputeAutoCameraDistance(aspect);
    const double sceneRadius = ComputeSceneRadius();
    const double nearPlane = max(0.1, cameraDistance - sceneRadius * 1.3);
    const double farPlane = cameraDistance + sceneRadius * 1.5;

    gluPerspective(kFovYDeg, aspect, nearPlane, farPlane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const double yawRad = g_cameraYaw * M_PI / 180.0;
    const double pitchRad = g_cameraPitch * M_PI / 180.0;

    const double cp = cos(pitchRad);
    const double sp = sin(pitchRad);
    const double cy = cos(yawRad);
    const double sy = sin(yawRad);

    const Vec3 eye = {
        cameraDistance * cp * cy,
        cameraDistance * cp * sy,
        cameraDistance * sp
    };

    gluLookAt(eye.x, eye.y, eye.z,
        0.0, 0.0, kLookAtZ,
        0.0, 0.0, 1.0);

    DrawPyramid();
    DrawCubeOnEdge();

    SwapBuffers(g_hDC);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_hDC = GetDC(hwnd);
        if (!SetPixelFormatForGL(g_hDC)) return -1;

        g_hGLRC = wglCreateContext(g_hDC);
        wglMakeCurrent(g_hDC, g_hGLRC);

        BuildPyramid();
        return 0;
    }

    case WM_SIZE:
        g_wndWidth = LOWORD(lParam);
        g_wndHeight = HIWORD(lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        g_isDraggingCamera = true;
        g_lastMousePos.x = GET_X_LPARAM(lParam);
        g_lastMousePos.y = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        if (g_isDraggingCamera)
        {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            const int dx = x - g_lastMousePos.x;
            const int dy = y - g_lastMousePos.y;

            g_cameraYaw += dx * 0.35;
            g_cameraPitch -= dy * 0.35;

            if (g_cameraPitch > 89.0) g_cameraPitch = 89.0;
            if (g_cameraPitch < -89.0) g_cameraPitch = -89.0;

            g_lastMousePos.x = x;
            g_lastMousePos.y = y;

            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_isDraggingCamera)
        {
            g_isDraggingCamera = false;
            ReleaseCapture();
        }
        return 0;

    case WM_RBUTTONDOWN:
        g_spinDeg -= 10.0;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_KEYDOWN:
    {
        if (wParam == VK_UP)
        {
            g_slide += 0.05;
            if (g_slide > 1.0) g_slide = 1.0;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == VK_DOWN)
        {
            g_slide -= 0.05;
            if (g_slide < 0.0) g_slide = 0.0;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam >= '0' && wParam <= '9')
        {
            int digit = (int)(wParam - '0');
            if (digit == 0) digit = 10;
            if (digit >= 1 && digit <= (int)g_edges.size())
            {
                g_edgeIndex = digit - 1;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (wParam == 'E')
        {
            if (g_baseVertexCount < 50)
            {
                ++g_baseVertexCount;
                BuildPyramid();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        else if (wParam == 'Q')
        {
            if (g_baseVertexCount > 3)
            {
                --g_baseVertexCount;
                BuildPyramid();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        DrawScene();
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        wglMakeCurrent(nullptr, nullptr);
        if (g_hGLRC) wglDeleteContext(g_hGLRC);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BOOL InitApp()
{
    WNDCLASSEXW wce = {};
    wce.cbSize = sizeof(WNDCLASSEXW);
    wce.hInstance = g_hApp;
    wce.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wce.lpfnWndProc = MainWindowProc;
    wce.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wce.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wce.lpszClassName = g_szWndClass;

    if (!RegisterClassExW(&wce)) return FALSE;

    g_hWindow = CreateWindowW(
        g_szWndClass,
        g_szTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 900, 900,
        nullptr, nullptr, g_hApp, nullptr);

    return g_hWindow != nullptr;
}

void UninitApp()
{
    UnregisterClassW(g_szWndClass, g_hApp);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) 
{
    g_hApp = hInstance;

    if (InitApp())
    {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    UninitApp();
    return 0;
}
