#define IMGUI_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>  // OpenGL ����� ���� �߰�
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <fftw3.h>
//#include <cufft.h>                       <<�̷л� ������� �ϴµ� �ش� ���̺귯���� �Լ��� ������ ���θ� �ؼ��� �ӵ��� ������  << �� ������ ���� Ȯ���ѵ� �Ƿ��� ������ �� �� ����;;
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
* fft �Լ��߰�: fft����� �༮�� ��� �� �ֵ���! << �̰� Ŀ�ǵ�� fft ġ�� ������!<< ���ļ������� �� �� ������ ������? fft�� ���̺귯�� �Ⱦ��� �غ��� ��������
* ������ ����� ������ ������ �� �ֵ���! << ó������ ������ �������� �Է��ؾ� ������ �� �ֵ���!! �� ���Ŀ��� ���� ������ ����Ŀ�ǵ� �Լ��� �����ϸ� �� ����
* �ް����ø���.. �� �ϴ� ������ �ۼ����� �Ǿ�� �� �̾߱� �̹Ƿ� ���߿�
* ���� ���İ�Ƽ �� ��ä�ε� ������ ������ ����
* �ϴ� ��Ƽ�� �Լ� ���� ����� ���� ����� �Ϻ��ϰ� ���డ���ϰ� �� ��!
*
*
*/


#define TIME_INTERVAL_1 0.000000025   //�ð�ǥ���� ���� �ϵ��ڵ�. ���ø��ӵ����� �߰� ����
#define FREQ_FFT_TEST 100000

const double PI = 2 * acos(0);
constexpr float ADC_MAX = 65535.0f;
const float REF_VOLTAGE = 3.3f;

constexpr int fftsize = 1024;                  //fft �����ϴ� ������ ����� ����
//std::vector<float> yData(fftsize, 0.0f);  // �Է� ��ȣ
//std::vector<std::complex<float>> fftData(fftsize / 2 + 1);  // FFT ���
//cufftComplex* deviceData;           // GPU���� FFT ������ ������ �޸�

std::vector<double> frequency(fftsize / 2, 0.0);
std::vector<double> magnitude(fftsize / 2, 0.0);

std::string error_message = "";           // ���� �˾��� �� ����

std::atomic<bool> running(true);        // �� �� �Լ� ����, ���Ḧ ���� �Լ�
std::atomic<bool> fftop(false);         // fft ����� �����͸� �Ѱ��ִ� �Ű����� 
std::atomic<int>scale(1);               // ������ ����
std::vector<double> xData, yData, fft_xData, fft_yData;
std::vector<double> xData2, yData2;
std::vector<fftw_complex> out(fftsize / 2 + 1); // ���Ҽ� ��� (N/2 + 1 ũ��!)


SDL_Window* mainWindow = nullptr;
SDL_GLContext mainGLContext;  // fft ���� �� ���� SDL3 â�� �������� ���� ���� ���� ����

HANDLE ocill;

//uint16_t adc_value;
float adc2voltage(uint16_t*);
void ErrorPopup();
bool connenctoscilloscope(const char* portname);
void MAGoperation(std::vector<fftw_complex>&);

void UserInput();





void RunApp() {
    // SDL3 �ʱ�ȭ
    SDL_Init(SDL_INIT_VIDEO);
    mainWindow = SDL_CreateWindow("Real-time Plot and fft",
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    mainGLContext = SDL_GL_CreateContext(mainWindow);


    // OpenGL ���ؽ�Ʈ Ȱ��ȭ
    SDL_GL_MakeCurrent(mainWindow, mainGLContext);

    // OpenGL ���� �⺻ ����
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // ImGui & ImPlot �ʱ�ȭ
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(mainWindow, mainGLContext);
    ImGui_ImplOpenGL3_Init("#version 130");


    SDL_Event event;


    float time = 0.0f;

    int fftcal = 0;      // fft ������ �󵵸� ���߱� ���� ���� 

    for (int i = 0; i < fftsize / 2; i++) {
        frequency[i] = i * (FREQ_FFT_TEST / fftsize); // ���ļ� ��
    }


    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        // ���ο� ������ �߰�

        time += 0.001f;
        //time+=TIME_INTERVAL1;
        xData.push_back(time);
        yData.push_back(5 * (sin(time) + sin(time * 3) / 3 + sin(time * 5) / 5 + sin(time * 7) / 7 + sin(9 * time) / 9 + sin(11 * time) / 11 + sin(13 * time) / 13 + sin(time * 15) / 15 + sin(time * 17) / 17 + sin(19 * time) / 19 + sin(21 * time) / 21 + sin(time * 23) / 23));  // ����: ������
        //yData.push_back(adc2voltage(*adc_value))



        // ������ ������ ���� ���߿��� �ǽð����� �߿������Ƿ� ���� �ʿ�
        if (xData.size() > 8192) {
            xData.erase(xData.begin());
            yData.erase(yData.begin());

        }


        // ������ ����
        glClear(GL_COLOR_BUFFER_BIT);  // GLEW ���� OpenGL ��� ����
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // ImPlot���� �׷��� ���
        ImGui::Begin("Real-time Data");
        if (ImPlot::BeginPlot("Real-time Plot")) {
            ImPlot::PlotLine("Data", xData.data(), yData.data(), xData.size());
            ImPlot::EndPlot();

        }
        ImGui::End();


        if (fftcal == 1024 || fftcal == -1024) {               //���⼭ fft ���� ����
            fftw_plan plan = fftw_plan_dft_r2c_1d(fftsize, yData.data(), out.data(), FFTW_ESTIMATE);

            fftw_execute(plan);         // fft ���� 
            MAGoperation(out);

            xData2 = frequency;
            yData2 = magnitude;

            fftcal = -1;
        }
        else if ((fftcal > -1) && (fftcal < 1024)) {            // ���̵����� 
            xData2.push_back(time);
            yData2.push_back(std::cos(time * 2.0f) * 1.5f); // ���ļ�*2, ���� 1.5
            fftcal++;
        }
        else {
            fftcal--;
        }


        ImGui::Begin("fft");                         // ������ ���� ������ 
        if (ImPlot::BeginPlot("freq - amp ")) {
            if (!xData2.empty()) {
                ImPlot::PlotLine("Wave2", xData2.data(), yData2.data(), (int)xData2.size());
            }
            ImPlot::EndPlot();
        }
        ImGui::End();
        /*
        if (fftop.load()) {        // fft ���� ����� ������ ����
            fft_xData = xData;
            fft_yData = yData;

            ImGui::NewFrame();
            if (ImPlot::BeginPlot("FFT Output")) {
                ImPlot::PlotLine("Magnitude", frequency.data(), magnitude.data(), magnitude.size());
                ImPlot::EndPlot();
            }
        }
        */


        // ImGui ������
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(mainWindow);

    }

    // ����
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    SDL_GL_DestroyContext(mainGLContext);  // SDL_GL_DeleteContext ���� �ذ�
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
}





int main() {
    //  if (!connenctoscilloscope("COM3")) return -1;  // ���� ����������� �����͸� �޾ƿ� �� �ּ�ó�� ����

    std::thread inputThread(UserInput);

    RunApp();
    return 0;
}



bool show_error_popup = false;


void ErrorPopup() {             // �̰� ���߿� ��������� �˾����� ���� ���� �Լ��� �����ִ� ������ �Բ�
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", error_message.c_str());  // ���� �޽��� ���
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
    portstate.BaudRate = CBR_115200;          //���� ���巹��Ʈ ���߱� ���
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
    if (adcvalue == NULL) return -1.0f;  // null ������ ����ó��

    switch (scale) {
    case 0:
        return (float)(*adcvalue) * REF_VOLTAGE / ADC_MAX;    // ���߿��� 65536�߿� ������ 0���� �� ���̴� �������� �߰� �ٶ�, ����� ���� ���̴� ��ȯ�� �и���Ʈ�� �ƴ� ��Ʈ ������
    case 1:
        return (float)(*adcvalue) * REF_VOLTAGE / ADC_MAX;  // ���⼭ �����Ͽ� ���� ��Ƽ�� �� ����
    case 2:
        break;
    default:
        break;
    }
}


void MAGoperation(std::vector<fftw_complex>& out) {
    for (int i = 0; i < fftsize / 2; i++) {
        magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]); // ũ�� ���
    }
}