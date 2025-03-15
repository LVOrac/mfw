#include "ScreenbufferTest.h"

#include "InputEvent.h"
#include "KeyCode.h"
#include "Application.h"
#include "Input.h"
#include "Clock.h"
#include "Renderer.h"
#include "imgui/imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "UiLayer.h"

static f32 ract[] = {
     1.0,  1.0,   1.0, 1.0,
     1.0, -1.0,   1.0, 0.0,
    -1.0, -1.0,   0.0, 0.0,

     1.0,  1.0,   1.0, 1.0,
    -1.0,  1.0,   0.0, 1.0,
    -1.0, -1.0,   0.0, 0.0,
};

static f32 vertex[] = {
     0.01,  0.01,
     0.01, -0.01,
    -0.01, -0.01,

     0.01,  0.01,
    -0.01,  0.01,
    -0.01, -0.01,
};

using namespace mfw;

ScreenBuffer::ScreenBuffer(i32 width, i32 height): vbo(ract, sizeof(ract), GL_STATIC_DRAW)
{
    mfw::VertexBufferLayout layout;
    layout.add<f32>(2);
    layout.add<f32>(2);
    vao.applyBufferLayout(layout);

    shader.attachShader(GL_VERTEX_SHADER, "res/shaders/framebuffer.vert");
    shader.attachShader(GL_FRAGMENT_SHADER, "res/shaders/framebuffer.frag");
    shader.link();

    vao.unbind();
    vbo.unbind();
    shader.unbind();

    GLCALL(glGenFramebuffers(1, &fbo));
    GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    GLCALL(glGenTextures(1, &texture));
    GLCALL(glBindTexture(GL_TEXTURE_2D, texture));
    GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCALL(glBindTexture(GL_TEXTURE_2D, 0));

    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0));

    GLCALL(glGenRenderbuffers(1, &rbo));
    GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    GLCALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
    GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    GLCALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo));

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "error: " << status << '\n';
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ScreenBufferTest::ScreenBufferTest()
    : Layer(__func__), vbo(vertex, sizeof(vertex), GL_DYNAMIC_DRAW)
{}

ScreenBufferTest::~ScreenBufferTest() {
    if (screenBuffer) {
        delete screenBuffer;
    }
}

void updateKernalImgui(std::string name, ScreenBufferTest::kernal& kernal) {
    ImGui::SliderFloat((name + " radius").c_str(), &kernal.radius, 0, 40);
    ImGui::SliderFloat((name + " growth u").c_str(), &kernal.growth_u, 0, 1);
    ImGui::SliderFloat((name + " growth k").c_str(), &kernal.growth_k, 0, 1);
}

class ScreenBufferUi : public UiLayer {
public:
    ScreenBufferTest* test;
    ScreenBufferUi(ScreenBufferTest* test = nullptr)
        : UiLayer(__func__), test(test)
    {
        name = "Info";
    }

    virtual bool OnUiRender() override {
        if (!test)
            return false;
        ImGui::Text("frame: %gms", test->frame * 1000);
        ImGui::SliderInt("step", &test->step, 1, 40);
        ImGui::Checkbox("stop", &test->stop);

        ImGui::SliderFloat("dt", &test->dt, 0, 1);
        ImGui::SliderFloat("cursor width", &test->cursorWidth, 1, 30);
        updateKernalImgui("outer", test->outer);
        updateKernalImgui("inner", test->inner);

        if (ImGui::SliderFloat("resolution.x", &test->resolution.x, 120, 2560) ||
            ImGui::SliderFloat("resolution.y", &test->resolution.y, 68, 1440))
        {
            delete test->screenBuffer;
            test->screenBuffer = new ScreenBuffer(test->resolution.x, test->resolution.y);
        }
        return ImGui::IsWindowFocused();
    }
};

static ScreenBufferUi* ui;

static f32 color[3] = {COLOR(0x977F89)};
void ScreenBufferTest::OnStart() {
    ui = new ScreenBufferUi(this);
    Application::Get().addUiLayer(ui);

    outer.radius = 10;
    outer.growth_u = 0.257;
    outer.growth_k = 0.336;
    inner.radius = 3;
    inner.growth_u = 0.365;
    inner.growth_k = 0.549;
    dt = 0.3;

    cursorWidth = 10;

    VertexBufferLayout layout;
    layout.add<f32>(2);
    vao.applyBufferLayout(layout);
    shader.attachShader(GL_VERTEX_SHADER, "res/shaders/default.vert");
    shader.attachShader(GL_FRAGMENT_SHADER, "res/shaders/default.frag");
    shader.link();
    vao.unbind();
    vbo.unbind();
    shader.unbind();

    resolution = glm::vec2(1280, 720);
    screenBuffer = new ScreenBuffer(resolution.x, resolution.y);

    glViewport(0, 0, resolution.x, resolution.y);
    glClearColor(0, 0, 0, 1);
}

void ScreenBufferTest::OnUpdate() {
    Timer timer;
    auto window = &Application::Get().GetWindow();

    glBindFramebuffer(GL_FRAMEBUFFER, screenBuffer->fbo);
    if (Input::KeyPress(MF_KEY_R)) {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    stop = Input::KeyPress(MF_KEY_LEFT_CONTROL);

    glViewport(0, 0, resolution.x, resolution.y);

    auto[x, y] = Input::GetMouse();
    const glm::vec2 mouse = glm::vec2(x / (f32)window->width(), 1 - y / (f32)window->height()) * 2.0f - 1.0f;
    vertex[0] = mouse.x;
    vertex[1] = mouse.y;

    if (Input::MouseButtonDown(MF_MOUSE_BUTTON_LEFT) && !ui->focus) {
        vao.bind();
        shader.bind();
        color[0] = 1;
        color[1] = 1;
        color[2] = 1;
        shader.set3f("color", color);
        glm::vec2 pp = {1.0 / resolution.x, 1.0 / resolution.y};
        for (f32 i = 0; i < pp.x * cursorWidth; i += pp.x) {
            for (f32 j = 0; j < pp.y * cursorWidth; j += pp.y) {
                vertex[0] = mouse.x + i;
                vertex[1] = mouse.y + j;
                vbo.setBuffer(vertex, 2 * sizeof(f32));
                glDrawArrays(GL_POINTS, 0, 1);
            }
        }
    }

    if (Input::MouseButtonDown(MF_MOUSE_BUTTON_RIGHT) && !ui->focus) {
        vao.bind();
        shader.bind();
        color[0] = 0;
        color[1] = 0;
        color[2] = 0;
        shader.set3f("color", color);
        glm::vec2 pp = {1.0 / resolution.x, 1.0 / resolution.y};
        for (f32 i = 0; i < pp.x * cursorWidth; i += pp.x) {
            for (f32 j = 0; j < pp.y * cursorWidth; j += pp.y) {
                vertex[0] = mouse.x + i;
                vertex[1] = mouse.y + j;
                vbo.setBuffer(vertex, 2 * sizeof(f32));
                glDrawArrays(GL_POINTS, 0, 1);
            }
        }
    }

    static int c = 0;
    c++;
    if (c % step == 0 && !stop) {
        u32 newTexture;
        GLCALL(glGenTextures(1, &newTexture));
        GLCALL(glBindTexture(GL_TEXTURE_2D, newTexture));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
        GLCALL(glReadBuffer(GL_COLOR_ATTACHMENT0));
        GLCALL(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, resolution.x, resolution.y));

        screenBuffer->vao.bind();
        ShaderProgram shader;
        shader.attachShader(GL_VERTEX_SHADER, "res/shaders/GameOfLife.vert");
        shader.attachShader(GL_FRAGMENT_SHADER, "res/shaders/GameOfLife.frag");
        shader.link();
        shader.bind();
        shader.set1f("outer.radius", outer.radius);
        shader.set1f("outer.growth_u", outer.growth_u);
        shader.set1f("outer.growth_k", outer.growth_k);
        shader.set1f("inner.radius", inner.radius);
        shader.set1f("inner.growth_u", inner.growth_u);
        shader.set1f("inner.growth_k", inner.growth_k);
        shader.set1f("dt", dt);
        glBindTexture(GL_TEXTURE_2D, newTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteTextures(1, &newTexture);
        c = 0;
    }

    screenBuffer->shader.bind();
    screenBuffer->vao.bind();
    glBindTexture(GL_TEXTURE_2D, screenBuffer->texture);
    glViewport(0, 0, window->width(), window->height());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    frame = timer.getDuration();
}

