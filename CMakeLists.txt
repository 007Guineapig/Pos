cmake_minimum_required(VERSION 3.10)
project(SnakeGame) 

set(CMAKE_C_STANDARD 11)  # Change from C99 to C11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Add the source files for the server
set(SERVER_SOURCE_FILES
    main.c              
    server_snake.c      
    snake.c             
)

# Add the source files for the client
set(CLIENT_SOURCE_FILES
    client_snake.c      # Client logic
    snake.c             # Shared game logic
)

add_executable(server_snake ${SERVER_SOURCE_FILES})

add_executable(client_snake ${CLIENT_SOURCE_FILES})

target_link_libraries(server_snake pthread)
target_link_libraries(client_snake pthread)