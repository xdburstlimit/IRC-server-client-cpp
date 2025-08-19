#include "./client.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML/Graphics.hpp"

int server_running = 1;
int client_running = 1;
int msg_i = 0;
std::string display_messages = "Welcome! Be respectful.\n";
std::string users_connected = "";

int main(){
    pthread_t* thread_handles;
    thread_handles = (pthread_t*)  malloc(sizeof(pthread_t));
    send_args args_send_thread;
    recv_args args_recv_thread;

    //getting username from client
    sf::RenderWindow username_window(sf::VideoMode(300, 200), " ",sf::Style::Titlebar|sf::Style::Close);
	ImGui::SFML::Init(username_window);
    sf::Clock deltaClock;
    //init fonts
    float size_pixels = 18;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFont* font = io.Fonts->AddFontFromFileTTF("../../../res/client/font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());
    io.FontDefault = font;
    ImGui::SFML::UpdateFontTexture();

    std::vector <std::string> chat_history;

    int len;
    char username_i[50] = "";
    while(username_window.isOpen()){
        sf::Event event;
        while(username_window.pollEvent(event)){
            ImGui::SFML::ProcessEvent(event);
            if(event.type == sf::Event::Closed){
                username_window.close();
                break;
            }
        }
        ImGui::SFML::Update(username_window, deltaClock.restart());
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0xee, 0xe8, 0xd6, 0xff));

        ImGui::SetNextWindowPos(ImVec2(60,60));
        ImGui::SetNextWindowSize(ImVec2(200,0));
		ImGui::Begin(" ",NULL,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetWindowFocus(" ");
        ImGui::InputText("", username_i, IM_ARRAYSIZE(username_i));

        ImGui::SameLine();
        if((ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false) &&ImGui::IsWindowFocused() || 
            ImGui::Button("Enter")) && (username_i[0] != '/'&&strlen(username_i) > 0)){
            username_window.close();
            break;
        }
        ImGui::PopFont();
		ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    
        username_window.clear(sf::Color(238, 232, 214, 1));
        ImGui::SFML::Render(username_window);
        username_window.display();
        
    }
    ImGui::SFML::Shutdown();
    std::string username(username_i);
    
    if(!username.empty()){
        sf::RenderWindow w_imgui(sf::VideoMode(745, 715), "Client",sf::Style::Titlebar|sf::Style::Close);
        ImGui::SFML::Init(w_imgui);
        sf::Clock deltaClock1;

        //font
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        ImFont* font = io.Fonts->AddFontFromFileTTF("../../../res/client/font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());
        io.FontDefault = font;
        ImGui::SFML::UpdateFontTexture();

        //sending username to server
        int server_socket = get_server_socket();
        len = strlen(username_i);
        if(len <= 0){
            perror("send");
        }else{
            if (send(server_socket, username_i, len, 0) == -1) {
                perror("username");
            }
        }

        //Filling in args_recv_thread
        args_recv_thread.socket_fd = server_socket;
        args_recv_thread.pfds[0].fd = server_socket;
        args_recv_thread.pfds[0].events = POLLIN;

        args_recv_thread.fd_count = 1;

        int exit_event = eventfd(0,0);

        args_recv_thread.pfds[1].fd = exit_event;
        args_recv_thread.pfds[1].events = POLLIN;

        args_recv_thread.fd_count = 2;

        //Filling in args_send_thread
        args_send_thread.socket_fd = server_socket;
        args_send_thread.exit_fd = exit_event;
        
        pthread_create(thread_handles, nullptr,
                recv_data, (void*) &args_recv_thread);
        char msg[256] = "";
        while(w_imgui.isOpen()){
            sf::Event event;
            while(w_imgui.pollEvent(event)){
                ImGui::SFML::ProcessEvent(event);
                if(event.type == sf::Event::Closed){
                    client_running = 0;
                    server_running = 0;
                    u_int64_t val = 1;//eventfd can accept <=8bytes
                    if(write(exit_event, &val, sizeof(val))==-1) perror("write");
                    w_imgui.close();
                    break;
                }
                if(server_running == 0) break;  
            }
            ImGui::SFML::Update(w_imgui, deltaClock1.restart());
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0xee, 0xe8, 0xd6, 0xff));
            
            ImGui::SetNextWindowPos(ImVec2(0,0));
            ImGui::Begin("chat lobby",NULL,ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::InputTextMultiline(" ",(char*) display_messages.c_str(),sizeof(display_messages),ImVec2(575,675),ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();
            ImGui::End();
        
            ImGui::SetNextWindowPos(ImVec2(580,0));
            ImGui::Begin("users", NULL ,ImGuiWindowFlags_NoTitleBar);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::InputTextMultiline(" ",(char*) users_connected.c_str(),sizeof(users_connected),ImVec2(150,675),ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(0,675));
            ImGui::SetNextWindowSize(ImVec2(880,150));
            ImGui::Begin(" ",NULL,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::SetWindowFocus(" ");
            ImGui::InputText("", msg, IM_ARRAYSIZE(msg));
            ImGui::SameLine();
            if((ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false) &&ImGui::IsWindowFocused() || 
                ImGui::Button("Send", ImVec2(150,0))) && strlen(msg) > 0){
                send_data(msg,server_socket);
                char combined[256];
                memset(combined,0,IM_ARRAYSIZE(combined));
                strcat(combined, username_i);
                strcat(combined, ": ");
                strcat(combined, msg);
                std::string msg_str = combined;
                display_messages += (msg_str + '\n');
                memset(msg,0 ,IM_ARRAYSIZE(msg));
            }
            ImGui::PopFont();
            ImGui::End();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            w_imgui.clear();
            ImGui::SFML::Render(w_imgui);
            w_imgui.display();
        }


            
            
        pthread_join(*thread_handles, nullptr);
        
    }
    std::cout << "shalom\n";

    
    return 0;
}