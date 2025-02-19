#define IMGUI_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>  // OpenGL 사용을 위해 추가
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <fftw3.h>
//#include <cufft.h>                       <<이론상 개빨라야 하는데 해당 라이브러리의 함수가 존재함 으로만 해서도 속도가 나락감  << 내 문제가 거의 확실한데 실력이 없으니 별 수 없음;;
//#include<cuda_runtime.h>              

#include <iostream>
#include <vector>
#include <cmath>
#include<Windows.h>
#include<string>
#include<thread>
#include<atomic>
#include <complex>
#include<stdio.h>


/*
* to do list
* fft 함수추가: fft계산한 녀석을 띄울 수 있도록! << 이거 커맨드로 fft 치면 나오게!<< 병렬수행으로 할 수 있을득 뭔말알? fft는 라이브러리 안쓰고 해볼수 있으려나
* 윈도우 헤더로 스케일 전달할 수 있도록! << 처음에는 무조건 스케일을 입력해야 굴러갈 수 있도록!! 그 이후에는 지금 구현된 유저커맨드 함수로 수행하면 될 듯함
* 메가샘플링은.. 뭐 일단 데이터 송수신이 되어야 될 이야기 이므로 나중에
* 지금 스파게티 그 잡채인데 정리는 언젠가 ㅋㅋ
* 일단 볼티지 함수 대충 만들어 놨고 통신을 완벽하게 수행가능하게 할 것!
*
*
*/


#define TIME_INTERVAL_1 0.000000025   //시간표현을 위한 하드코딩. 샘플링속도마다 추가 예정
#define FREQ_FFT_TEST 100000

const double PI = 2 * acos(0);
constexpr float ADC_MAX = 65535.0f;
const float REF_VOLTAGE = 3.3f;

constexpr int fftsize = 1024;                  //fft 수행하는 사이즈 변경용 변수
//std::vector<float> yData(fftsize, 0.0f);  // 입력 신호
//std::vector<std::complex<float>> fftData(fftsize / 2 + 1);  // FFT 결과
//cufftComplex* deviceData;           // GPU에서 FFT 연산을 수행할 메모

std::vector<double> frequency(fftsize / 2, 0.0);
std::vector<double> magnitude(fftsize / 2, 0.0);

std::string error_message = "";           // 에러 팝업에 들어갈 내용

std::atomic<bool> running(true);        // 런 앱 함수 시작, 종료를 위한 함수
std::atomic<bool> fftop(false);         // fft 연산시 데이터를 넘겨주는 매개변수 
std::atomic<int>scale(1);               // 스케일 변경
std::vector<double> xData, yData, fft_xData, fft_yData;
std::vector<double> xData2, yData2;
std::vector<fftw_complex> out(fftsize / 2 + 1); // 복소수 출력 (N/2 + 1 크기!)


SDL_Window* mainWindow = nullptr;
SDL_GLContext mainGLContext;  // fft 연산 시 기존 SDL3 창을 가져오기 위해 전역 변수 선언

HANDLE ocill;

//uint16_t adc_value;
float adc2voltage(uint16_t*);
void ErrorPopup();
bool connenctoscilloscope(const char* portname);
void MAGoperation(std::vector<fftw_complex>&);

void UserInput();





void RunApp() {
    // SDL3 초기화
    SDL_Init(SDL_INIT_VIDEO);
    mainWindow = SDL_CreateWindow("Real-time Plot and fft",
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    mainGLContext = SDL_GL_CreateContext(mainWindow);


    // OpenGL 컨텍스트 활성화
    SDL_GL_MakeCurrent(mainWindow, mainGLContext);

    // OpenGL 관련 기본 설정
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // ImGui & ImPlot 초기화
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(mainWindow, mainGLContext);
    ImGui_ImplOpenGL3_Init("#version 130");


    SDL_Event event;


    float time = 0.0f;

    int fftcal = 0;      // fft 연산의 빈도를 낮추기 위한 변수 

    for (int i = 0; i < fftsize / 2; i++) {
        frequency[i] = i * (FREQ_FFT_TEST / fftsize); // 주파수 값
    }


    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        // 새로운 데이터 추가

        time += 0.001f;
        //time+=TIME_INTERVAL1;
        xData.push_back(time);
        yData.push_back(5 * (sin(time) + sin(time * 3) / 3 + sin(time * 5) / 5 + sin(time * 7) / 7 + sin(9 * time) / 9 + sin(11 * time) / 11 + sin(13 * time) / 13 + sin(time * 15) / 15 + sin(time * 17) / 17 + sin(19 * time) / 19 + sin(21 * time) / 21 + sin(time * 23) / 23));  // 예제: 구형파
        //yData.push_back(adc2voltage(*adc_value))



        // 오래된 데이터 삭제 나중에는 실시간성이 중요해지므로 개선 필요
        if (xData.size() > 8192) {
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


        if (fftcal == 1024 || fftcal == -1024) {               //여기서 fft 연산 수행
            fftw_plan plan = fftw_plan_dft_r2c_1d(fftsize, yData.data(), out.data(), FFTW_ESTIMATE);

            fftw_execute(plan);         // fft 연산 
            MAGoperation(out);

            xData2 = frequency;
            yData2 = magnitude;

            fftcal = -1;
        }
        else if ((fftcal > -1) && (fftcal < 1024)) {            // 더미데이터 
            xData2.push_back(time);
            yData2.push_back(std::cos(time * 2.0f) * 1.5f); // 주파수*2, 진폭 1.5
            fftcal++;
        }
        else {
            fftcal--;
        }


        ImGui::Begin("fft");                         // 지금은 더미 데이터 
        if (ImPlot::BeginPlot("freq - amp ")) {
            if (!xData2.empty()) {
                ImPlot::PlotLine("Wave2", xData2.data(), yData2.data(), (int)xData2.size());
            }
            ImPlot::EndPlot();
        }
        ImGui::End();
        /*
        if (fftop.load()) {        // fft 연산 수행시 데이터 전달
            fft_xData = xData;
            fft_yData = yData;

            ImGui::NewFrame();
            if (ImPlot::BeginPlot("FFT Output")) {
                ImPlot::PlotLine("Magnitude", frequency.data(), magnitude.data(), magnitude.size());
                ImPlot::EndPlot();
            }
        }
        */


        // ImGui 렌더링
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(mainWindow);

    }

    // 정리
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    SDL_GL_DestroyContext(mainGLContext);  // SDL_GL_DeleteContext 문제 해결
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
}





int main() {
    //  if (!connenctoscilloscope("COM3")) return -1;  // 이후 유선통신으로 데이터를 받아올 때 주석처리 해제

    std::thread inputThread(UserInput);

    RunApp();
    return 0;
}



bool show_error_popup = false;


void ErrorPopup() {             // 이거 나중에 오류출력을 팝업으로 띄우기 위한 함수임 위에있는 변수와 함께
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", error_message.c_str());  // 오류 메시지 출력
        if (ImGui::Button("OK")) {

            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}


bool connenctoscilloscope(const char* portname) {
    ocill = CreateFileA(portname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ocill == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening port\n";
        return false;
    }
    DCB portstate = { 0 };
    portstate.DCBlength = sizeof(portstate);
    if (!GetCommState(ocill, &portstate)) {
        std::cerr << "Error getting serial state\n";
        return false;
    }
    portstate.BaudRate = CBR_115200;          //이후 보드레이트 맞추길 요망
    portstate.ByteSize = 8;
    portstate.Parity = NOPARITY;
    portstate.StopBits = ONESTOPBIT;
    if (!SetCommState(ocill, &portstate)) {
        std::cerr << "Error setting serial parameters\n";
        CloseHandle(ocill);
        return false;
    }

    return true;
}


void UserInput() {
    std::string command;
    std::cout << "oscilloscope CLI\ncommand hint: exit, scale_N\n>>";
    while (running) {
        std::getline(std::cin, command);
        if (!command.empty()) {
            if (command == "exit") {
                std::cout << "Shutting down\n";
                fftop.store(false);
                running.store(false);
            }
            else if (command == "scale_1") {
                scale = 1;
                std::cout << "scale set:" << scale << std::endl;
            }
            else if (command == "scale_2") {
                scale = 2;
                std::cout << "scale set:" << scale << std::endl;
            }

            else {
                std::cout << "unkown command: " << command << std::endl;
            }
        }
    }
}


float adc2voltage(uint16_t* adcvalue) {
    if (adcvalue == NULL) return -1.0f;  // null 포인터 예외처리

    switch (scale) {
    case 0:
        return (float)(*adcvalue) * REF_VOLTAGE / ADC_MAX;    // 나중에는 65536중에 절반을 0으로 볼 것이니 덧셈연산 추가 바람, 참고로 지금 보이는 변환은 밀리볼트가 아닌 볼트 단위임
    case 1:
        return (float)(*adcvalue) * REF_VOLTAGE / ADC_MAX;  // 여기서 스케일에 따른 볼티지 값 조정
    case 2:
        break;
    default:
        break;
    }
}


void MAGoperation(std::vector<fftw_complex>& out) {
    for (int i = 0; i < fftsize / 2; i++) {
        magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]); // 크기 계산
    }
}