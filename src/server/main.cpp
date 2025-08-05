#include "./server.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML/Graphics.hpp"

int thread_count = 2;
int server_running = 1;

int main(){
    start();
    return 0;
}