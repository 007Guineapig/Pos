#include "data_pipeline.h"
#include <string.h> // For memcpy
#include <stdio.h>  // For debugging


// Serialize GameState into a buffer
int serialize_game_state(GameState *state, char *buffer, size_t buffer_size) {
    size_t required_size = sizeof(GameState);
    if (buffer_size < required_size) {
        printf("Buffer too small for serialization: required %zu, got %zu\n", required_size, buffer_size);
        return -1; // Buffer too small
    }

    printf("Serializing GameState: %zu bytes\n", required_size);
    // Copy struct into buffer
    memcpy(buffer, state, required_size);
    return 0;
}
int deserialize_game_state(char *buffer, size_t buffer_size, GameState *state) {
    if (buffer_size < sizeof(GameState)) {
        printf("Buffer too small for deserialization: expected %zu, got %zu\n", sizeof(GameState), buffer_size);
        return -1; // Insufficient data
    }

    printf("Deserializing GameState: %zu bytes\n", sizeof(GameState));
    memcpy(state, buffer, sizeof(GameState)); // Copy buffer into struct
    return 0;
}
int init_pipeline(DataPipeline *pipeline) {
    pipeline->preprocess = NULL; // Add custom functions as needed
    pipeline->postprocess = NULL;
    pipeline->serialize = serialize_game_state;
    pipeline->deserialize = deserialize_game_state;
    return 0;
}



