#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -1.3f
#define PLATFORM_COUNT 12

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include <algorithm>

/**
* Author: JADA FORRESTER
* Assignment: Lunar Lander
* Date due: 2023-11-08, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "assets/PC Computer - Omori - Omori.png",
            GOOD_PLATFORM_FILEPATH[] = "assets/black tile.png",
            BAD_PLATFORM_FILEPATH[]     = "assets/scissors.png",
            LIGHTBULB_FILEPATH[] = "assets/Whitespace_Lightbulb.png",
            DOWN_ARROW_FILEPATH[] = "assets/down_arrow.png",
            GAME_OVER_FILEPATH[] = "assets/over.jpg",
            WIN_FILEPATH[] = "assets/win.jpeg";

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;  // this value MUST be zero

// ————— VARIABLES ————— //
GameState g_game_state;
GameState g_good_platforms;
GameState g_lightbulb;
GameState g_arrow;

std::vector<glm::vec3> stored_good_positions;

std::vector<GLuint> bad_texture_ids;
std::vector<GLuint> good_texture_ids;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;

}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
  
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    

    
    // ————— PLAYER ————— //
    // Existing
    g_game_state.player = new Entity();
    //top left corner
    g_game_state.player->set_position(glm::vec3(-4.3f, 3.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    // Walking
    g_game_state.player->m_walking[g_game_state.player->LEFT]   = new int[4] { 1, 5, 9,  13 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT]  = new int[4] { 3, 7, 11, 15 };
    g_game_state.player->m_walking[g_game_state.player->UP]     = new int[4] { 2, 6, 10, 14 };
    g_game_state.player->m_walking[g_game_state.player->DOWN]   = new int[4] { 0, 4, 8,  12 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    g_game_state.player->m_animation_frames  = 4;
    g_game_state.player->m_animation_index   = 0;
    g_game_state.player->m_animation_time    = 0.0f;
    g_game_state.player->m_animation_cols    = 4;
    g_game_state.player->m_animation_rows    = 4;
    g_game_state.player->set_height(0.9f);
    g_game_state.player->set_width(0.9f);

    // ————— PLATFORM ————— //
    g_game_state.platforms = new Entity[PLATFORM_COUNT];
    //g_good_platforms.platforms = new Entity[PLATFORM_COUNT]
    //light bulb

    g_lightbulb.platforms = new Entity[1];
   // g_lightbulb.platforms->m_texture_id = load_texture(LIGHTBULB_FILEPATH);
    
    //arrow
    g_arrow.platforms = new Entity[1];
    //g_arrow.platforms->m_texture_id = load_texture(DOWN_ARROW_FILEPATH);
    
   
    for (int i = 0; i < PLATFORM_COUNT; i++){
        
        //every 4 platform is good or bad
        if(i % 4 == 0){
            //bad platform
            g_game_state.platforms[i].m_texture_id = load_texture(BAD_PLATFORM_FILEPATH);
            g_game_state.platforms[i].set_position(glm::vec3(i -4.5f,-1.0f,1.0f));
            g_game_state.platforms[i].update(0.0f, NULL, 0);
            GLuint bad_texture_id = g_game_state.platforms[i].get_texture_id();
            bad_texture_ids.push_back(bad_texture_id);
            //std::cout<< texture_id << std::endl;
            
        }
        else{
            g_game_state.platforms[i].m_texture_id = load_texture(GOOD_PLATFORM_FILEPATH);
            
            // good platform
            g_game_state.platforms[i].set_position(glm::vec3(i- 4.5f, -3.0f, 1.0f));
            g_game_state.platforms[i].update(0.0f, NULL, 0);
            GLuint good_texture_id = g_game_state.platforms[i].get_texture_id();
            good_texture_ids.push_back(good_texture_id);
            //std::cout<< good_texture_id << std::endl;
            glm::vec3 good_position =  g_game_state.platforms[i].get_position();
            stored_good_positions.push_back(good_position);
            //std::cout << "Position: x = " << good_position.x << ", y = " << good_position.y << ", z = " << good_position.z << std::endl;
            
            
            //decorations
            g_arrow.platforms[0].m_texture_id = load_texture(DOWN_ARROW_FILEPATH);
            //arrow to indicate good platform
            g_arrow.platforms[0].set_position(glm::vec3(2.7f, -2.0f, 1.0f));
            g_arrow.platforms[0].update(0.0f, NULL, 0);
           
        }
    }
   
    //lightbulb at the top right
    g_lightbulb.platforms[0].m_texture_id = load_texture(LIGHTBULB_FILEPATH);
    g_lightbulb.platforms[0].set_position(glm::vec3(3.0f, 3.0f, 0.0f));
    g_lightbulb.platforms[0].update(0.0f, NULL, 0);
    
    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}


void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP)
    {
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;
 
    if(g_game_state.player->check_collision(g_game_state.platforms)){
            auto iter = std::find(bad_texture_ids.begin(), bad_texture_ids.end(), g_game_state.platforms->get_texture_id());
            
            if(iter != bad_texture_ids.end()){
                //g_game_state.platforms->activate();
                g_game_state.platforms->m_texture_id = load_texture(GAME_OVER_FILEPATH);
                g_game_state.player->set_acceleration(glm::vec3(0.0f));
                //std::cout << "bad platform" << std::endl;
                g_game_is_running  = false;
            }
            else{
                g_game_state.platforms->m_texture_id  = load_texture(WIN_FILEPATH);
                g_game_state.player->set_acceleration(glm::vec3(0.0f));
                g_game_is_running  = false;
            }
            
           
        }
    
}



void render()
{
    // ————— GENERAL ————— //
    //light gray
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // ————— PLAYER ————— //
    g_game_state.player->render(&g_shader_program);

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++){
        g_game_state.platforms[i].render(&g_shader_program);
    }
    
    g_lightbulb.platforms[0].render(&g_shader_program);
    g_arrow.platforms[0].render(&g_shader_program);
    
    
    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{

    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
