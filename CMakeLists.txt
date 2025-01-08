cmake_minimum_required(VERSION 3.10)
project(SnakeGame)  # Add project name

# Set C standard to C11 and enable all warnings
set(CMAKE_C_STANDARD 11)  # Change from C99 to C11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Add the source files for the server
set(SERVER_SOURCE_FILES
    main.c              # Contains the main function for the server
    server_snake.c      # Server logic
    snake.c             # Shared game logic
)

# Add the source files for the client
set(CLIENT_SOURCE_FILES
    client_snake.c      # Client logic
    snake.c             # Shared game logic
)

# Create the executable for the server
add_executable(server_snake ${SERVER_SOURCE_FILES})

# Create the executable for the client
add_executable(client_snake ${CLIENT_SOURCE_FILES})

# Link pthread for both executables (required for threading)
target_link_libraries(server_snake pthread)
target_link_libraries(client_snake pthread)