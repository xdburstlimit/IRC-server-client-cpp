add_executable(chat_server ${SERVER_SRC_FILES})

target_link_libraries(chat_server
    sfml-graphics
    sfml-window
    sfml-system
    GL
)
