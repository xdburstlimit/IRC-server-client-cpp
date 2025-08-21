#include "./server.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML/Graphics.hpp"

int server_running = 1;
int msg_i{};
int fd_start{};
std::string display_messages = "Welcome! Be respectful.\n";
std::string users_connected = "";


int main(){
    start();
    std::cout << "Server successfully closed.\n";
    return 0;
}