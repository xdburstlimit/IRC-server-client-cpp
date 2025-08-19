#include "./server.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML/Graphics.hpp"

int server_running = 1;
int msg_i{};
std::string display_messages = "Welcome! Be respectful.\n";
std::string users_connected = "";

/*
    TODO:
        somehow need to send user list to users
    DONE:
        pass display messages as a parameter or see if you can bullshit with the code(since display_messages is defined in the header)
        horizontal scrolling
        make it show the the box cant be moved around
        Show users currently connected

*/

int main(){
    start();
    return 0;
}