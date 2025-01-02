cmake_minimum_required(VERSION 3.10)
project(SnakeGame)

# Add the source files for the server
set(SERVER_SOURCE_FILES
    main.c               # Server's main function
    server_snake.c       # Server logic
    snake.c              # Shared game logic
    data_pipeline.c      # Data pipeline functions
)

# Add the source files for the client
set(CLIENT_SOURCE_FILES
    client_snake.c       # Client's main function
    snake.c              # Shared game logic
    data_pipeline.c      # Data pipeline functions
)

# Create the executable for the server
add_executable(server_snake ${SERVER_SOURCE_FILES})

# Create the executable for the client
add_executable(client_snake ${CLIENT_SOURCE_FILES})
