#include "win/WindowsOpenglContext.h"

#include "mfwlog.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"

#define GLAD_GL_IMPLEMENTATION
#define GLAD_WGL_IMPLEMENTATION
#include "glad/wgl.h"

namespace mfw {
    OpenglContext* OpenglContext::Instance = new WindowsOpenglContext();

    const wchar_t* OpenglContextClassName = L"__OpenglDummyWindow";

    void WindowsOpenglContext::CreateMordenImpl(Window* window, i32 major, i32 minor) {
        HINSTANCE instance = GetModuleHandle(NULL);
        WNDCLASSEX wc{};
        if (GetClassInfoEx(instance, OpenglContextClassName, &wc))
            return;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = &DefWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = OpenglContextClassName;
        wc.style = CS_OWNDC;

        RegisterClassEx(&wc);
        HWND hwnd = CreateWindow(
                OpenglContextClassName,
                L"",
                0,
                0, 0, 0, 0,
                NULL,
                NULL,
                GetModuleHandle(NULL),
                NULL
                );

        HDC hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32,
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        int suggestPfdId = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, suggestPfdId, &pfd);

        m_hglrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, m_hglrc);

        gladLoaderLoadWGL(hdc);

        //if (glewInit() != GLEW_OK) {
        //    LOG_INFO("GLEW INIT FAIL\n");
        //}

        // ReleaseImpl();
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);

        WindowsWindow* w = reinterpret_cast<WindowsWindow*>(window->getNativeWindow());

        const int attribList[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            0,
        };

        int pixelFormat;
        unsigned int numFormats;
        wglChoosePixelFormatARB(w->m_hdc, attribList, nullptr, 1, &pixelFormat, &numFormats);

        SetPixelFormat(w->m_hdc, pixelFormat, &pfd);

        const int contextAttribList[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, major,
            WGL_CONTEXT_MINOR_VERSION_ARB, minor,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0
        };

        HGLRC shareHglrc = nullptr;
        w->m_hglrc = wglCreateContextAttribsARB(w->m_hdc, shareHglrc, contextAttribList);
        wglMakeContextCurrentARB(w->m_hdc, w->m_hdc, w->m_hglrc);

        if (!gladLoaderLoadGL()) {
            LOG_INFO("glad load gl fail\n");
            ASSERT(true);
        }

        LOG_INFO("OPENGL VERSION: {}\n", glGetString(GL_VERSION));
        //wglSwapIntervalEXT(1);
    }

    void WindowsOpenglContext::ReleaseImpl() {
        wglDeleteContext(m_hglrc);
    }
}
