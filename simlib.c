#include "simlib.h"


FFMPEG *StartFFMPEGProcess(const size_t width, const size_t height, const size_t FPS, const char *output_dir, const char *log_level)
{
    mkdir(output_dir, S_IRWXU | S_IRWXG | S_IRWXO);
    
    char formated_time[32];
    time_t current_time = time(NULL);
    struct tm* tm_info = localtime(&current_time);
    strftime(formated_time, 32, "%Y-%m-%d %H:%M:%S", tm_info);

    int pipe_fd[2];
    if (pipe(pipe_fd) < 0)
    {
        fprintf(stderr, "ERROR: Could not create a pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int pid = fork();  
    if (pid == 0)
    {
        close(pipe_fd[WRITE_END]); 
        if (dup2(pipe_fd[READ_END], STDIN_FILENO) < 0)
        {
            fprintf(stderr, "ERROR: Could not set STDIN file descriptor as the pipe's read end file descriptor: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        char str_resolution[RESOLUTION_CAP];
        char str_fps[FPS_CAP];
        char str_output_name[OUTPUT_NAME_CAP];

        snprintf(str_resolution, RESOLUTION_CAP, "%zux%zu", width, height);
        snprintf(str_fps, FPS_CAP, "%zu", FPS);
        snprintf(str_output_name, OUTPUT_NAME_CAP, "%s/%s.mp4", output_dir, formated_time);

        int ret = execlp("ffmpeg", 
            "ffmpeg",
            "-loglevel", log_level,
            "-y",
            
            "-f", "rawvideo",
            "-pix_fmt", "rgba",
            "-s", str_resolution,
            "-r", str_fps,
            "-an", 
            "-i", "-",
            
            "-c:v", "libx264",
            str_output_name,
            NULL);

        if (ret < 0)
        {
            fprintf(stderr, "ERROR: Could not create ffmpeg process: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    close(pipe_fd[READ_END]);

    FFMPEG *ffmpeg = (FFMPEG*)malloc(sizeof(FFMPEG));
    ffmpeg->width  = width;
    ffmpeg->height = height;
    ffmpeg->pipe   = pipe_fd[WRITE_END];
    ffmpeg->pid    = pid;
    return ffmpeg;
}   

void FeedFFMPEG(FFMPEG *ffmpeg, void *data)
{
    write(ffmpeg->pipe, data, sizeof(uint32_t) * ffmpeg->width * ffmpeg->height);
}

void FeedFFMPEGInverted(FFMPEG *ffmpeg, void *data)
{
    for (int y=ffmpeg->height-1; y >= 0; --y)
        write(ffmpeg->pipe, (uint32_t*)data + y * ffmpeg->width, sizeof(uint32_t) * ffmpeg->width);
}

void CloseFFMPEG(FFMPEG *ffmpeg)
{
    close(ffmpeg->pipe);
    waitpid(ffmpeg->pid, NULL, 0);
    free(ffmpeg);
}


void CreateSimulationState(SimulationState *sim_state, enum Mode mode, int target_resolution_width, int target_resolution_height, int fps, int duration)
{
    sim_state->mode = mode;
    sim_state->target_resolution_width  = target_resolution_width;
    sim_state->target_resolution_height = target_resolution_height; 
    sim_state->counter = 0.0f;  
    sim_state->fps = fps;
    sim_state->dt = 1.0f / (float)fps;
    sim_state->duration = duration;
    sim_state->loading_bar_size = (Vector2){ 300.0f, 50.0f };
    sim_state->loading_bar_offset = 10.0f;
    sim_state->percentage_font_size = 50.0f;
    sim_state->math_standard = false;
}

void ParseSimulationState(SimulationState *sim_state, int argc, char **argv)
{
    if (argc < 5 || argc > 6)
    {
        fprintf(stderr, "ERROR: Please provide the simulation mode, resolution and FPS and duration if 'render' mode is set. (mode, width, height, FPS, duration [render])\n");
        printf("NOTE: with the target resolution does not match the monitor resolution it will be scaled.\n");
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "run") == 0)
        sim_state->mode = RUN;
    else if (strcmp(argv[1], "render") == 0)
        sim_state->mode = RENDER;
    else if (strcmp(argv[1], "both") == 0)
        sim_state->mode = BOTH;
    else
    {
        fprintf(stderr, "ERROR: '%s' is not a valid mode.\n", argv[1]);
        exit(EXIT_FAILURE);
    } 
    
    if (sscanf(argv[2], "%d", &sim_state->target_resolution_width) != 1)
    {
        fprintf(stderr, "ERROR: '%s' is not a valid width.\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    if (sscanf(argv[3], "%d", &sim_state->target_resolution_height) != 1)
    {
        fprintf(stderr, "ERROR: '%s' is not a valid height.\n", argv[3]);
        exit(EXIT_FAILURE);
    }
    if (sscanf(argv[4], "%d", &sim_state->fps) != 1)
    {
        fprintf(stderr, "ERROR: '%s' is not a valid FPS.\n", argv[4]);
        exit(EXIT_FAILURE);
    }
    
    if (sim_state->mode == RENDER && argc == 6)
    {
        if (sscanf(argv[5], "%f", &sim_state->duration) != 1)
        {
            fprintf(stderr, "ERROR: '%s' is not a valid duration.\n", argv[4]);
            exit(EXIT_FAILURE);
        }
    } else if (sim_state->mode != RENDER && argc == 6)
    {
        fprintf(stderr, "ERROR: Too many arguments provide. Expected the following arguments: width, height, FPS.\n");
        exit(EXIT_FAILURE);
    } else if (sim_state->mode == RENDER && argc != 6)
    {
        fprintf(stderr, "ERROR: Please provide simulation duration.\n");
        exit(EXIT_FAILURE);
    }
    
    sim_state->counter = 0.0f;  
    sim_state->dt = 1.0f / (float)sim_state->fps;
    sim_state->loading_bar_size = (Vector2){ 300.0f, 50.0f };
    sim_state->loading_bar_offset = 10.0f;
    sim_state->percentage_font_size = 50.0f;
    sim_state->math_standard = false;
}

void InitSimulation(SimulationState *sim_state, Vector2 start_view, const char *title)
{
    InitWindow(0, 0, title);
    sim_state->monitor_width  = GetScreenWidth();
    sim_state->monitor_height = GetScreenHeight();
    
    if (sim_state->mode != RENDER)
        SetTargetFPS(sim_state->fps);
    
    ToggleFullscreen();
    
    sim_state->target      = LoadRenderTexture(sim_state->target_resolution_width, sim_state->target_resolution_height);
    sim_state->origin      = (Vector2){ 0.0f, 0.0f };
    sim_state->destination = (Rectangle){ 0.0f, 0.0f, sim_state->monitor_width, sim_state->monitor_height }; 
    if (sim_state->math_standard)
        sim_state->source = (Rectangle){ 0.0f, 0.0f, sim_state->target_resolution_width, sim_state->target_resolution_height };
    else
        sim_state->source = (Rectangle){ 0.0f, 0.0f, sim_state->target_resolution_width, -sim_state->target_resolution_height };
        
    sim_state->ffmpeg = NULL;
    if (sim_state->mode != RUN)
    {
        sim_state->ffmpeg = StartFFMPEGProcess(sim_state->target_resolution_width, sim_state->target_resolution_height, sim_state->fps, "videos", "quiet");
        SetTraceLogLevel(LOG_NONE);
    } 
    
    sim_state->gui_camera.offset   = (Vector2){ sim_state->monitor_width / 2.0f, sim_state->monitor_height / 2.0f };  
    sim_state->gui_camera.target   = (Vector2){ 0.0f, 0.0f };
    sim_state->gui_camera.rotation = 0.0f;
    sim_state->gui_camera.zoom     = 1.0f;

    sim_state->camera.offset   = (Vector2){ sim_state->target_resolution_width / 2.0f, sim_state->target_resolution_height / 2.0f };
    sim_state->camera.target   = (Vector2){ 0.0f, 0.0f };
    sim_state->camera.rotation = 0.0f;
    if (start_view.x > start_view.y)
        sim_state->camera.zoom = (float)sim_state->target_resolution_width / start_view.x;
    else
        sim_state->camera.zoom = (float)sim_state->target_resolution_height / start_view.y;
}

void BeginSimulationMode(SimulationState *sim_state, Color clear_color)
{
    BeginDrawing();
    BeginTextureMode(sim_state->target);
    ClearBackground(clear_color);
    BeginMode2D(sim_state->camera);
}

int EndSimulationMode(SimulationState *sim_state)
{
    EndMode2D();
    EndTextureMode();
    
    if (sim_state->mode != RENDER)
        DrawTexturePro(sim_state->target.texture, sim_state->source, sim_state->destination, sim_state->origin, 0.0f, WHITE);
    else 
    {
        char percentage[6];
        sprintf(percentage, "%.1f%%", 100.0f * sim_state->counter / sim_state->duration);

        ClearBackground(BLACK);
        BeginMode2D(sim_state->gui_camera);
        DrawText(percentage, -(MeasureText(percentage, sim_state->percentage_font_size) / 2.0f), -100.0f, sim_state->percentage_font_size, WHITE);

        DrawRectangle(
                -sim_state->loading_bar_size.x / 2.0f, 
                -sim_state->loading_bar_size.y / 2.0f, 
                sim_state->loading_bar_size.x * (sim_state->counter / sim_state->duration), 
                sim_state->loading_bar_size.y, 
                GREEN); 
        
        DrawRectangleLines(
                -(sim_state->loading_bar_size.x + sim_state->loading_bar_offset) / 2.0f, 
                -(sim_state->loading_bar_size.y + sim_state->loading_bar_offset) / 2.0f, 
                sim_state->loading_bar_size.x + sim_state->loading_bar_offset, 
                sim_state->loading_bar_size.y + sim_state->loading_bar_offset, 
                WHITE);
        EndMode2D(); 
    }

    if (sim_state->mode != RUN)
    {
        sim_state->image = LoadImageFromTexture(sim_state->target.texture);
        FeedFFMPEGInverted(sim_state->ffmpeg, sim_state->image.data);
        UnloadImage(sim_state->image);        
    }
    
    EndDrawing();

    if (sim_state->mode == RENDER)
        if ((sim_state->counter += sim_state->dt) > sim_state->duration)
            return 0;
    
    return 1;
}

void CloseSimulation(SimulationState *sim_state)
{
    UnloadRenderTexture(sim_state->target);
    CloseWindow();
    if (sim_state->mode == RENDER || sim_state->mode == BOTH)
        CloseFFMPEG(sim_state->ffmpeg);
}

