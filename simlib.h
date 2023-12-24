#ifndef SIMLIB_H
#define SIMLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <raylib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define READ_END         0
#define WRITE_END        1
#define RESOLUTION_CAP  32
#define FPS_CAP         16
#define OUTPUT_NAME_CAP 514
#define TITLE_CAP       64

typedef struct
{
    size_t width;
    size_t height;
    int pipe;
    pid_t pid;
} FFMPEG;

enum Mode
{
    RUN,
    RENDER,
    BOTH
};

typedef struct
{   
    enum Mode mode;
    int target_resolution_width;
    int target_resolution_height;
    int monitor_width;
    int monitor_height;
    int fps;
    float duration;
    float counter;
    float dt;
    Camera2D gui_camera;
    Camera2D camera;
    RenderTexture2D target;
    Image image;
    Vector2 origin;
    Rectangle source;
    Rectangle destination;
    FFMPEG *ffmpeg;
    Vector2 loading_bar_size;
    float loading_bar_offset;
    float percentage_font_size;
    bool math_standard;
} SimulationState;


FFMPEG *StartFFMPEGProcess(const size_t width, const size_t height, const size_t FPS, const char *output, const char *log_level);
void    FeedFFMPEG(FFMPEG *ffmpeg, void *data);
void    FeedFFMPEGInverted(FFMPEG *ffmpeg, void *data);
void    CloseFFMPEG(FFMPEG *ffmpeg);

void    CreateSimulationState(SimulationState *sim_state, enum Mode mode, int target_resolution_width, int target_resolution_height, int fps, int duration);
void    ParseSimulationState(SimulationState *sim_state, int argc, char **argv);
void    InitSimulation(SimulationState *sim_state, Vector2 start_view, const char *title);
void    BeginSimulationMode(SimulationState *sim_state, Color clear_color);
int     EndSimulationMode(SimulationState *sim_state);
void    CloseSimulation(SimulationState *sim_state);

#endif

