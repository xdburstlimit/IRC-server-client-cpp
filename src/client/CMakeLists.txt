add_executable(chat_client ${CLIENT_SRC_FILES})

target_link_libraries(chat_client
    sfml-graphics
    sfml-window
    sfml-system
    GL
)
