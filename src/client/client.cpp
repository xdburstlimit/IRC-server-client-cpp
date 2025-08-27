#include "./client.h"


addrinfo* get_address(const char* ip){
    int status;
    addrinfo hints;
    addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//ipv4
    hints.ai_socktype = SOCK_STREAM;//tcp stream sockets

    if((status = getaddrinfo(ip, PORT, &hints, &res))!=0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    return res;

}

int get_server_socket(){
    addrinfo *server_info, *p;
    int yes=1;
    int socket_fd;
    const char* ip = "127.0.0.1";
    server_info = get_address(ip);
    char s[INET_ADDRSTRLEN];

    for(p = server_info; p != nullptr; p = p->ai_next){
        //make socket
        if ((socket_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        //bind socket to port
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_fd);
            perror("client: connect");
            continue;
        }
        break;
    }
    freeaddrinfo(server_info);

    //some check for pointer == nullptr
    
    if(p == nullptr){
        fprintf(stderr,"client: failed to connect\n");
        exit(1);
    }
    
    return socket_fd;
}


void send_data(char msg[],int socket_fd){
    int len{};
    len = strlen(msg);
    if(len <= 0){
        perror("send");
    }else{
        if (send(socket_fd, msg, len, 0) == -1) {
            perror("send");
        }
    }
    return;
}

void* recv_data(void *args){
    recv_args* my_args = (recv_args*) args;
    while(1){
        int poll_count = poll(my_args->pfds, my_args->fd_count,-1);
        if(client_running == 0) break;
        if(server_running == 0) break;
        for(int i{}; i < my_args->fd_count; ++i){
            if(my_args->pfds[i].revents & POLLIN){
                char buf[buf_size];
                memset(buf, 0, sizeof(buf));
                int byte_count = recv(my_args->socket_fd, buf, sizeof(buf), 0);
                if(byte_count<=0){
                    if(byte_count == 0){
                        std::string shut_down = "WARNING: Server closed, you can safely close the application now\n";
                        display_messages += shut_down;
                        server_running = 0;
                        break;
                    }
                }else{
                    if(buf[0] == '/'){
                        users_connected = "";
                        users_connected = buf+1;
                        memset(buf, 0, sizeof(buf));
                    }else{
                        std::string buf_str = buf;
                        display_messages += buf_str + '\n';
                    }
                }
            }
        }
    }
    return nullptr;
}

void start(){
    pthread_t* thread_handles;
    thread_handles = (pthread_t*)  malloc(sizeof(pthread_t));
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
    char username_i[username_length] = "";
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
        
        pthread_create(thread_handles, nullptr,
                recv_data, (void*) &args_recv_thread);
        char msg[message_length] = "";
        while(w_imgui.isOpen()){
            sf::Event event;
            while(w_imgui.pollEvent(event)){
                ImGui::SFML::ProcessEvent(event);
                if(event.type == sf::Event::Closed){
                    client_running = 0;
                    u_int64_t val = 1;//eventfd can accept <=8bytes
                    if(write(exit_event, &val, sizeof(val))==-1) perror("write");
                    w_imgui.close();
                    break;
                }
                if(server_running == 0) break;  
            }

            //ImGui windows start here
            ImGui::SFML::Update(w_imgui, deltaClock1.restart());
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0xee, 0xe8, 0xd6, 0xff));
            
            chat_window();
        
            user_window();   

            ImGui::SetNextWindowPos(ImVec2(0,675));
            ImGui::SetNextWindowSize(ImVec2(880,150));
            ImGui::Begin(" ",NULL,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::SetWindowFocus(" ");
            ImGui::InputText("", msg, IM_ARRAYSIZE(msg));
            ImGui::SameLine();
            if((ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false) &&ImGui::IsWindowFocused() || 
                ImGui::Button("Send", ImVec2(150,0))) && strlen(msg) > 0 && strlen(msg) < message_length && server_running != 0){
                send_data(msg,server_socket);
                char combined[total_length];
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
}

void chat_window(){
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::Begin("chat lobby",NULL,ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::InputTextMultiline(" ",(char*) display_messages.c_str(),sizeof(display_messages),ImVec2(575,675),ImGuiInputTextFlags_ReadOnly);
    ImGui::PopFont();
    ImGui::End();

}

void user_window(){
    ImGui::SetNextWindowPos(ImVec2(580,0));
    ImGui::Begin("users", NULL ,ImGuiWindowFlags_NoTitleBar);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::InputTextMultiline(" ",(char*) users_connected.c_str(),sizeof(users_connected),ImVec2(150,675),ImGuiInputTextFlags_ReadOnly);
    ImGui::PopFont();
    ImGui::End();

}
