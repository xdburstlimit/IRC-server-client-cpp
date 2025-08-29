Built an IRC-server-client with the use of the polling example from Beej's networking guide(would highly recommend reading through the the textbook). Used SFML,imgui,socket and pthreads.__

Setup(Linux only):__
*SFML is a dependency so install it with(Ubuntu):__
  sudo apt-get install libsfml-dev__
*Other platforms I am uncertain of__
*Afterwards you can build with CMake and the executables will be in the build\src\server and build\src\client folders respectively.__

How to run:__
*Run the server__
*And afterwards you can connect clients to the server.__

Note:__
As of now the clients just connect to the localhost and will add IP input later.__

TODO:__
*Add some type of message encryption(RSA,etc).__
*For the client add another box to connect to a specific IP and port(its only connecting to the localhost).__
*Store pollfd and usernames in a std::vector instead of pollfd* and char**.__
*Make it possible to send structs over a socket, with username and message(currently just sending char arrays)__
*Use libevent instead of poll().__

<img width="1496" height="755" alt="Screenshot from 2025-08-21 14-10-45" src="https://github.com/user-attachments/assets/bb58e39f-9b5e-4e2e-977f-94a32552e50b" />
