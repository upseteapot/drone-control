#include <raylib.h>
#include <math.h>
#include <simlib.h>
#define THRUSTERS 2
#define RAD_DEG(x) (180.0f / PI) * (x) 
#define VEC_MAG(vec) sqrt(pow(vec.x, 2) + pow(vec.y, 2))
#define VEC_ROT(vec, a) (Vector2){ cos(a) * vec.x - sin(a) * vec.y, sin(a) * vec.x + cos(a) * vec.y }


typedef struct
{
    Rectangle shape;
    Vector2 offset;
    Vector2 origin;
    float activation;
    float angle;
} Thruster;

typedef struct
{
    Rectangle shape;
    Rectangle collision;
    Vector2 origin;
    Vector2 vel;
    float angle;
    float angle_vel;
    float mass;
    float moment_of_inertia;
    Thruster thrusters[THRUSTERS];
    float thruster_magnitude;
} Drone;


void create_drone(Drone *drone, float mass, float x, float y, float width, float height)
{
    drone->shape = (Rectangle){ x, y, width, height };
    drone->origin = (Vector2){ width / 2.0f, height / 2.0f };
    drone->vel = (Vector2){ 0.0f, 0.0f };
    drone->angle = 0.0f;
    drone->angle_vel = 0.0f; 
    drone->mass = mass;
    drone->moment_of_inertia = mass * (pow(width, 2) + pow(height, 2)) / 12.0f;
    drone->thruster_magnitude = 300.0f; 

    float angle = PI / 2.5f;
    float thruster_width = 28.0f;
    float thruster_height = 12.0f;

    drone->thrusters[0].shape = (Rectangle){ 0.0f, 0.0f, thruster_width, thruster_height };
    drone->thrusters[0].offset = (Vector2){ -width / 2.0f, height / 2.0f };
    drone->thrusters[0].origin = (Vector2){ thruster_width / 3.0f, thruster_height / 2.0f };
    drone->thrusters[0].activation = 0.0f;
    drone->thrusters[0].angle = PI - angle; 
    
    drone->thrusters[1].shape = (Rectangle){ 0.0f, 0.0f, thruster_width, thruster_height };
    drone->thrusters[1].offset = (Vector2){ width / 2.0f, height / 2.0f };
    drone->thrusters[1].origin = (Vector2){ thruster_width / 3.0f, thruster_height / 2.0f };
    drone->thrusters[1].activation = 0.0f;
    drone->thrusters[1].angle = angle; 
}

void check_player_input(Drone *drone)
{
    if (IsKeyDown(KEY_A))
        drone->thrusters[0].activation = 1.0f;
    else    
        drone->thrusters[0].activation = 0.0f;
    
    if (IsKeyDown(KEY_D)) 
        drone->thrusters[1].activation = 1.0f;    
    else    
        drone->thrusters[1].activation = 0.0f;
}

void update_drone(Drone *drone, Vector2 gravity, float dt)
{
    Vector2 force = (Vector2){ 0.0f, 0.0f };
    Vector2 thruster_force = (Vector2){ 0.0f, 0.0f };
    float torque = 0.0f;
    Vector2 rotated_offset;

    for (int i = 0; i < THRUSTERS; i++)
    {
        rotated_offset = VEC_ROT(drone->thrusters[i].offset, drone->angle);
        thruster_force.x = cos(drone->thrusters[i].angle + drone->angle + PI) * drone->thrusters[i].activation * drone->thruster_magnitude;
        thruster_force.y = sin(drone->thrusters[i].angle + drone->angle + PI) * drone->thrusters[i].activation * drone->thruster_magnitude;
        
        torque += thruster_force.y * rotated_offset.x - thruster_force.x * rotated_offset.y;

        force.x += thruster_force.x;
        force.y += thruster_force.y;
    }
    
    force.x += drone->mass * gravity.x;
    force.y += drone->mass * gravity.y;

    drone->vel.x += force.x * dt / drone->mass;
    drone->vel.y += force.y * dt / drone->mass;
    drone->shape.x += drone->vel.x * dt;
    drone->shape.y += drone->vel.y * dt;

    drone->collision.x = drone->shape.x - drone->shape.width / 2.0f;
    drone->collision.y = drone->shape.y - drone->shape.height / 2.0f;
    drone->collision.width = drone->shape.width; 
    drone->collision.height = drone->shape.height;
    
    for (int i = 0; i < THRUSTERS; i++)
    {
        rotated_offset = VEC_ROT(drone->thrusters[i].offset, drone->angle);
        drone->thrusters[i].shape.x = drone->shape.x + rotated_offset.x; 
        drone->thrusters[i].shape.y = drone->shape.y + rotated_offset.y; 
    }
    
    drone->angle_vel += torque * dt / drone->moment_of_inertia; 
    drone->angle += drone->angle_vel * dt;
}

void check_bounderies(Drone *drone, Rectangle *bounderies, int bounderies_count, float dt)
{
    for (int i = 0; i < bounderies_count; i++)
        if (CheckCollisionRecs(drone->collision, bounderies[i]))
        {
            drone->shape.x -= drone->vel.x * dt; 
            drone->shape.y -= drone->vel.y * dt;
            drone->vel.x = 0.0f;
            drone->vel.y = 0.0f;
        }
}

void draw_drone(Drone *drone)
{
    DrawRectanglePro(drone->shape, drone->origin, RAD_DEG(drone->angle), WHITE);
    
    Color color;
    for (int i = 0; i < THRUSTERS; i++)
    {
        if (drone->thrusters[i].activation > 0.0f)
            color = RED;
        else
            color = GRAY;

        DrawRectanglePro(drone->thrusters[i].shape, drone->thrusters[i].origin, RAD_DEG(drone->angle + drone->thrusters[i].angle), color);
    }
}


int main(int argc, char **argv)
{
    int world_width  = 1280;
    int world_height = 720;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SimulationState sim_state;
    ParseSimulationState(&sim_state, argc, argv);
    InitSimulation(&sim_state, (Vector2){world_width, world_height}, "Simlib");
    
    const Vector2 gravity = (Vector2){ 0.0f, 400.0f }; 
    
    Rectangle bounderies[4];
    float bounderies_size = 20.0f;
    Color bounderies_color = (Color){ 60, 60, 70, 255 };
    bounderies[0] = (Rectangle){ -world_width / 2.0f, -world_height / 2.0f, world_width, bounderies_size };     
    bounderies[1] = (Rectangle){ -world_width / 2.0f, -world_height / 2.0f, bounderies_size, world_height };
    bounderies[2] = (Rectangle){ -world_width / 2.0f,  world_height / 2.0f - bounderies_size, world_width, bounderies_size };
    bounderies[3] = (Rectangle){  world_width / 2.0f - bounderies_size, -world_height / 2.0f, bounderies_size, world_height };

    Drone drone;
    create_drone(&drone, 1.0f, 0.0f, 0.0f, 95, 35);

    while (!WindowShouldClose())
    {
        BeginSimulationMode(&sim_state, BLACK);
        
        for (int i = 0; i < 4; i++)
            DrawRectangleRec(bounderies[i], bounderies_color);
       
        check_bounderies(&drone, bounderies, 4, sim_state.dt);
        check_player_input(&drone); 
        update_drone(&drone, gravity, sim_state.dt);
        draw_drone(&drone); 

        if (!EndSimulationMode(&sim_state))
            break;
    }

    CloseSimulation(&sim_state);
    
    return 0;
}

