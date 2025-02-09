#define IMGUI_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>  // OpenGL 사용을 위해 추가
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <vector>
#include <cmath>

void RunApp() {
    // SDL3 초기화
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Real-time Plot",
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    // OpenGL 컨텍스트 활성화
    SDL_GL_MakeCurrent(window, gl_context);

    // OpenGL 관련 기본 설정
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // ImGui & ImPlot 초기화
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool running = true;
    SDL_Event event;

    std::vector<float> xData, yData;
    float time = 0.0f;

    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        // 새로운 데이터 추가
        time += 0.001f;
        xData.push_back(time);
        yData.push_back(3*sin(time));  // 예제: 사인 함수

        // 오래된 데이터 삭제 
        if (xData.size() > 100000) {
            xData.erase(xData.begin());
            yData.erase(yData.begin());
        }

        // 렌더링 시작
        glClear(GL_COLOR_BUFFER_BIT);  // GLEW 없이 OpenGL 사용 가능
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // ImPlot으로 그래프 출력
        ImGui::Begin("Real-time Data");
        if (ImPlot::BeginPlot("Real-time Plot")) {
            ImPlot::PlotLine("Data", xData.data(), yData.data(), xData.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        // ImGui 렌더링
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // 정리
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    SDL_GL_DestroyContext(gl_context);  // SDL_GL_DeleteContext 문제 해결
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    RunApp();
    return 0;
}
