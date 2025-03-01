/**
* Author: Hanqi Liu
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <cmath>

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;


constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0,
SPEEDFACTOR = 5.0f,
BALLSPEEDFACTOR = 5.0f,
BOUND = 3.5f;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero


constexpr char PAT1_FILEPATH[] = "assets/pat.png",
PAT2_FILEPATH[] = "assets/pat.png",
BALL_FILEPATH[] = "assets/ball.png",
RIGHTWIN_FILEPATH[] = "assets/RightWinPic.png",
LEFTWIN_FILEPATH[] = "assets/LeftWinPic.png";

constexpr glm::vec3 INIT_PAT1_SCALE = glm::vec3(1.0f, 1.5f, 0.0f),
INIT_PAT2_SCALE = glm::vec3(1.0f, 1.5f, 0.0f),
INIT_BALL_SCALE = glm::vec3(1.0f,1.0f,0.0f),
INIT_WINPIC_SCALE = glm::vec3(12.0f,8.0f,0.0f);

constexpr float GITAI_ROTATION_RADIUS = 2.0f,
GITAI_ROTATION_SPEEDRATE = 1.0f;

bool ballMovRight = false;

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_pat1_matrix,
g_pat2_matrix,
g_ball1_matrix,
g_ball2_matrix,
g_ball3_matrix,
g_projection_matrix,
rightWin_matrix,
leftWin_matrix;

float g_previous_ticks = 0.0f,
yui_x = 0.0f,
yui_y = 0.0f,
counter = 0.0f;

bool pat1Win;

glm::vec3 pat1_movement = glm::vec3(0.0f, 0.0f, 0.0f),
pat1_pos = glm::vec3(4.8f, 0.0f, 0.0f),
pat2_movement = glm::vec3(0.0f, 0.0f, 0.0f),
pat2_pos = glm::vec3(-4.8f, 0.0f, 0.0f),
ball_pos1 = glm::vec3(0.0f,0.0f,0.0f),
ball_movement1 = glm::vec3(1.0f, 0.0f, 0.0f),
ball_pos2 = glm::vec3(0.0f, 1.0f, 0.0f),
ball_movement2 = glm::vec3(1.0f, 0.0f, 0.0f),
ball_pos3 = glm::vec3(0.0f, -1.0f, 0.0f),
ball_movement3= glm::vec3(1.0f, 0.0f, 0.0f);

GLuint g_pat1_texture_id,
g_pat2_texture_id,
g_ball_texture_id,
rightWin_texture_id,
leftWin_texture_id;

enum Gamemode{TWOPLAYER, ONEPLAYER};
Gamemode gamemode = TWOPLAYER;

enum Gamestate{PLAYING, RIGHTPATWIN, LEFTPATWIN};
Gamestate gamestate= PLAYING;

int ballNum = 1;

glm::vec3* ballPosVec[3] = { &ball_pos1,&ball_pos2,&ball_pos3 };
glm::vec3* ballMovementVec[3] = { &ball_movement1,&ball_movement2,&ball_movement3 };
glm::mat4* ballMatrixVec[3] = { &g_ball1_matrix,&g_ball2_matrix, &g_ball3_matrix };


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Homework 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_pat1_matrix = glm::mat4(1.0f);
    g_pat2_matrix = glm::mat4(1.0f);
    for (int i = 0; i < ballNum; i++)
    {
        *ballMatrixVec[i] = glm::mat4(1.0f);
    }
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_pat1_texture_id = load_texture(PAT1_FILEPATH);
    g_pat2_texture_id = load_texture(PAT2_FILEPATH);
    g_ball_texture_id = load_texture(BALL_FILEPATH);
    rightWin_texture_id = load_texture(RIGHTWIN_FILEPATH);
    leftWin_texture_id = load_texture(LEFTWIN_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    if (gamemode == TWOPLAYER) {
        pat1_movement.y = 0.0f;
    }
    pat2_movement.y = 0.0f;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_t: 
                if (gamemode == TWOPLAYER) {
                    gamemode = ONEPLAYER;
                    pat1_movement.y = 1.0f;
                }
                else {
                    gamemode = TWOPLAYER;
                }; break;
            case SDLK_1: 
                ballNum = 1; 
                ball_pos2 = glm::vec3(0.0f, 1.0f, 0.0f),
                ball_movement2 = glm::vec3(1.0f, 0.0f, 0.0f),
                ball_pos3 = glm::vec3(0.0f, -1.0f, 0.0f),
                ball_movement3 = glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case SDLK_2: 
                ballNum = 2;  
                ball_pos3 = glm::vec3(0.0f, -1.0f, 0.0f),
                ball_movement3 = glm::vec3(1.0f, 0.0f, 0.0f); 
                break;
            case SDLK_3: ballNum = 3; break;
            case SDLK_r:
                if (gamestate == LEFTPATWIN || gamestate == RIGHTPATWIN) {
                    pat1_movement = glm::vec3(0.0f, 0.0f, 0.0f),
                    pat1_pos = glm::vec3(4.8f, 0.0f, 0.0f),
                    pat2_movement = glm::vec3(0.0f, 0.0f, 0.0f),
                    pat2_pos = glm::vec3(-4.8f, 0.0f, 0.0f),
                    ball_pos1 = glm::vec3(0.0f, 0.0f, 0.0f),
                    ball_movement1 = glm::vec3(1.0f, 0.0f, 0.0f),
                    ball_pos2 = glm::vec3(0.0f, 1.0f, 0.0f),
                    ball_movement2 = glm::vec3(1.0f, 0.0f, 0.0f),
                    ball_pos3 = glm::vec3(0.0f, -1.0f, 0.0f),
                    ball_movement3 = glm::vec3(1.0f, 0.0f, 0.0f);
                    gamemode = TWOPLAYER;
                    ballNum = 1;
                    gamestate = PLAYING;
                }
                break;

            default: break;

            }

        default:
            break;
        }

    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);


    if (key_state[SDL_SCANCODE_S] && pat2_pos.y > -BOUND) {
        pat2_movement.y = -1.0f;
    }
    else if (key_state[SDL_SCANCODE_W] && pat2_pos.y < BOUND)
    {
        pat2_movement.y = 1.0f;
    }

    //player one movement
    if (key_state[SDL_SCANCODE_DOWN] && pat1_pos.y > -BOUND && gamemode == TWOPLAYER)
    {
        pat1_movement.y = -1.0f;
    }
    else if (key_state[SDL_SCANCODE_UP] && pat1_pos.y < BOUND && gamemode == TWOPLAYER)
    {
        pat1_movement.y = 1.0f;
    }
}

bool checkCollision(glm::vec3 entity1_pos, glm::vec3 entity2_pos, glm::vec3 entity1_scale, glm::vec3 entity2_scale) {

    float x_distance = fabs(entity1_pos.x - entity2_pos.x) - ((entity1_scale.x + entity2_scale.x) / 2.0f);
    float y_distance = fabs(entity1_pos.y - entity2_pos.y) - ((entity1_scale.y + entity2_scale.y) / 2.0f);
    if (x_distance < 0 && y_distance < 0) {
        return true;
    }
    else {
        return false;
    }
}



void update()
{
    if (gamestate == PLAYING) {
        float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = ticks - g_previous_ticks;
        g_previous_ticks = ticks;

        g_pat1_matrix = glm::mat4(1.0f);
        g_pat2_matrix = glm::mat4(1.0f);
        for (int i = 0; i < ballNum; i++)
        {
            *ballMatrixVec[i] = glm::mat4(1.0f);
        }


        for ( int i = 0; i < ballNum; i++)
        {
            if (checkCollision(*ballPosVec[i], pat1_pos, INIT_BALL_SCALE, INIT_PAT1_SCALE)) {
                ballMovementVec[i]->x = -1.0f;
                ballMovementVec[i]->y += pat1_movement.y;
                *ballMovementVec[i] = glm::normalize(*ballMovementVec[i]);
            };
            if (checkCollision(*ballPosVec[i], pat2_pos, INIT_BALL_SCALE, INIT_PAT2_SCALE)) {
                ballMovementVec[i]->x = 1.0f;
                ballMovementVec[i]->y += pat2_movement.y;
                *ballMovementVec[i] = glm::normalize(*ballMovementVec[i]);
            }
            *ballPosVec[i] += *ballMovementVec[i] * delta_time * BALLSPEEDFACTOR;
            if (ballPosVec[i]->y > BOUND) {
                ballPosVec[i]->y = 3.49f;
                ballMovementVec[i]->y = -ballMovementVec[i]->y;
            }
            if (ballPosVec[i]->y < -BOUND) {
                ballPosVec[i]->y = -3.49f;
                ballMovementVec[i]->y = -ballMovementVec[i]->y;
            }
            if (ballPosVec[i]->x > 4.8f && gamestate == PLAYING) {
                gamestate = LEFTPATWIN;
            }
            else if (ballPosVec[i]->x < -4.8f && gamestate == PLAYING) {
                gamestate = RIGHTPATWIN;
            }
            *ballMatrixVec[i] = glm::translate(*ballMatrixVec[i], *ballPosVec[i]);
            *ballMatrixVec[i] = glm::scale(*ballMatrixVec[i], INIT_BALL_SCALE);
        }
        

        if (gamemode == ONEPLAYER) {
            if (pat1_pos.y > BOUND) {
                pat1_pos.y = 3.49f;
                pat1_movement.y = -pat1_movement.y;
            }
            if (pat1_pos.y < -BOUND) {
                pat1_pos.y = -3.49f;
                pat1_movement.y = -pat1_movement.y;
            }
        }

        pat1_pos += pat1_movement * delta_time * SPEEDFACTOR;
        pat2_pos += pat2_movement * delta_time * SPEEDFACTOR;

        //translate
        g_pat1_matrix = glm::translate(g_pat1_matrix, pat1_pos);
        g_pat2_matrix = glm::translate(g_pat2_matrix, pat2_pos);

        //scale
        g_pat1_matrix = glm::scale(g_pat1_matrix, INIT_PAT1_SCALE);
        g_pat2_matrix = glm::scale(g_pat2_matrix, INIT_PAT2_SCALE);
        
    }
    else if (gamestate == RIGHTPATWIN) {
        rightWin_matrix = glm::mat4(1.0f);
        rightWin_matrix = glm::scale(rightWin_matrix, INIT_WINPIC_SCALE);
    }
    else if (gamestate == LEFTPATWIN){
        leftWin_matrix = glm::mat4(1.0f);
        leftWin_matrix = glm::scale(leftWin_matrix, INIT_WINPIC_SCALE);
    }
    else {
        return;
    }
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    if (gamestate == PLAYING) {
        draw_object(g_pat1_matrix, g_pat1_texture_id);
        draw_object(g_pat2_matrix, g_pat2_texture_id);
        for (size_t i = 0; i < ballNum; i++)
        {
            draw_object(*ballMatrixVec[i], g_ball_texture_id);
        }       
    }
    else if (gamestate == RIGHTPATWIN){
        draw_object(rightWin_matrix, rightWin_texture_id);
    }
    else {
        draw_object(leftWin_matrix, leftWin_texture_id);
    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}