#include "Application.h"

#include "Input.h"
#include "Clock.h"
#include "OpenglContext.h"
#include "Renderer.h"
#include "stb_image/stb_image.h"
#include "InputEvent.h"
#include "WindowEvent.h"

// TODO: BE CROSS PLATFORM
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"

namespace mfw {
    Application* Application::Instance = nullptr;

    Application::Application(): Application("app", 960, 640)
    {}

    Application::Application(const std::string& title, i32 width, i32 height)
    {
        ASSERT(Instance == nullptr);
        Instance = this;
        {
            START_CLOCK_TIMER("INIT WINDOW");
            m_window = std::unique_ptr<Window>(Window::Create({title, 100, 100, width, height}));
        }
        {
            START_CLOCK_TIMER("INIT OPENGL");
            OpenglContext::CreateMorden(m_window.get(), 3, 3);
            GLCALL(glEnable(GL_BLEND));
            GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        }

        stbi_set_flip_vertically_on_load(true);

        m_window->setVSync(true);

        m_window->setEventCallBack([this](const Event& event) {
                    Eventhandle(event);
                });

        eventListener.addEventFunc<KeyEvent>([this](const Event& event) {
                    return InputHandle(static_cast<const KeyEvent&>(event));
                });
        eventListener.addEventFunc<MouseButtonEvent>([this](const Event& event) {
                    return MouseButtonHandle(static_cast<const MouseButtonEvent&>(event));
                });
        eventListener.addEventFunc<MouseScrollEvent>([this](const Event& event) {
                    return MouseScrollHandle(static_cast<const MouseScrollEvent&>(event));
                });
        eventListener.addEventFunc<WindowResizeEvent>([this](const Event& event) {
                    return WindowResize(static_cast<const WindowResizeEvent&>(event));
                });
        eventListener.addEventFunc<WindowCloseEvent>([this](const Event& event) {
                    return WindowClose(static_cast<const WindowCloseEvent&>(event));
                });
        eventListener.addEventFunc<CursorMoveEvent>([this](const Event& event) {
                    return CursorMove(static_cast<const CursorMoveEvent&>(event));
                });
        eventListener.addEventFunc<WindowFocusEvent>([this](const Event& event) {
                    return WindowFocus(static_cast<const WindowFocusEvent&>(event));
                });
        eventListener.addEventFunc<WindowNotFocusEvent>([this](const Event& event) {
                    return WindowNotFocus(static_cast<const WindowNotFocusEvent&>(event));
                });
    }

    Application::~Application() {
        OpenglContext::Release();
        delete Input::Instance;
        delete Time::Instance;
        delete OpenglContext::Instance;
    }

    void Application::run() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.ConfigFlags |= ImGuiBackendFlags_HasSetMousePos;

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_InitForOpenGL(Application::Get().GetWindow().getHandle());
        ImGui_ImplOpenGL3_Init("#version 410");

        Start();
        while (m_window->isRunning()) {
            m_window->update();
            Update();

            for (auto& layer : layerSystem.getLayers()) {
                layer->OnUpdate();
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            for (auto& layer : uiLayerSystem.getLayers()) {
                layer->OnUpdate();
            }
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            m_window->swapBuffers();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void Application::Eventhandle(const Event& event) {
        auto& uiLayers = uiLayerSystem.getLayers();
        for (auto iter = uiLayers.rbegin(); iter != uiLayers.rend(); ++iter) {
            if ((*iter)->handleEvent(event)) {
                return;
            }
        }

        auto& layers = layerSystem.getLayers();
        for (auto iter = layers.rbegin(); iter != layers.rend(); ++iter) {
            if ((*iter)->handleEvent(event)) {
                return;
            }
        }

        eventListener.listen<KeyEvent>(event);
        eventListener.listen<WindowResizeEvent>(event);
        eventListener.listen<WindowCloseEvent>(event);
        eventListener.listen<MouseButtonEvent>(event);
        eventListener.listen<MouseScrollEvent>(event);
        eventListener.listen<CursorMoveEvent>(event);
        eventListener.listen<WindowFocusEvent>(event);
        eventListener.listen<WindowNotFocusEvent>(event);
    }


    void Application::Terminate() {
        m_window->close();
    }

    void Application::addLayer(Layer* layer) {
        layerSystem.addLayer(layer);
    }

    void Application::addUiLayer(UiLayer* uiLayer) {
        uiLayerSystem.addLayer(uiLayer);
    }

}

