#include "./client.h"

int server_running = 1;
int client_running = 1;

std::string display_messages = "Welcome! Be respectful.\n";
std::string users_connected = "";

int main(){
    start();
    std::cout << "Client successfully closed.\n";
    return 0;
}