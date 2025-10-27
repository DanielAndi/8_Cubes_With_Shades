#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <atomic>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/shader_m.h"
#include "include/camera.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

// Forward declarations
class TextRenderer;
extern TextRenderer* textRenderer;

// function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void renderText(const std::string& text, float worldX, float worldY, float worldZ, float scale, glm::mat4 projection, glm::mat4 view);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera - positioned to clearly view all cubes and text from an optimal angle
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// === New globals for interactive focus mode ===
std::atomic<int> g_focusIndex{-1}; // -1 means show all cubes
std::atomic<bool> g_keepReading{true};

// Shininess list for mapping user input
static const float kShininess[8] = {2.f, 4.f, 8.f, 16.f, 32.f, 64.f, 128.f, 256.f};

// Helper: map shininess value to cube index
int findShininessIndex(int val) {
    for (int i = 0; i < 8; ++i)
        if ((int)kShininess[i] == val)
            return i;
    return -1;
}


// Proper FreeType-based text rendering system
struct Character {
    unsigned int textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};

struct TextRenderer {
    std::map<char, Character> characters;
    unsigned int textVAO, textVBO;
    unsigned int textShaderProgram;
    FT_Library ft;
    FT_Face face;
    
    TextRenderer() {
        // Initialize FreeType
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return;
        }
        
        // Load font (try common system fonts)
        const char* fontPaths[] = {
            "/usr/share/fonts/TTF/Hack-Regular.ttf",
            "/usr/share/fonts/TTF/OpenSans-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/System/Library/Fonts/Arial.ttf",  // macOS
            "C:/Windows/Fonts/arial.ttf"        // Windows
        };
        
        bool fontLoaded = false;
        for (const char* fontPath : fontPaths) {
            if (FT_New_Face(ft, fontPath, 0, &face) == 0) {
                fontLoaded = true;
                std::cout << "Loaded font: " << fontPath << std::endl;
                break;
            }
        }
        
        if (!fontLoaded) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return;
        }
        
        // Set font size
        FT_Set_Pixel_Sizes(face, 0, 48);
        
        // Create shader program for text rendering (3D world space)
        const char* textVertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;
            out vec2 TexCoords;
            void main()
            {
                gl_Position = projection * view * model * vec4(vertex.xy, 0.0, 1.0);
                TexCoords = vertex.zw;
            }
        )";

        const char* textFragmentShaderSource = R"(
            #version 330 core
            in vec2 TexCoords;
            out vec4 color;
            uniform sampler2D text;
            uniform vec3 textColor;
            void main()
            {
                vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
                color = vec4(textColor, 1.0) * sampled;
            }
        )";
        
        // Compile shaders
        unsigned int textVertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(textVertexShader, 1, &textVertexShaderSource, NULL);
        glCompileShader(textVertexShader);
        
        unsigned int textFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(textFragmentShader, 1, &textFragmentShaderSource, NULL);
        glCompileShader(textFragmentShader);
        
        textShaderProgram = glCreateProgram();
        glAttachShader(textShaderProgram, textVertexShader);
        glAttachShader(textShaderProgram, textFragmentShader);
        glLinkProgram(textShaderProgram);
        
        glDeleteShader(textVertexShader);
        glDeleteShader(textFragmentShader);
        
        // Create VAO and VBO for text rendering
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);

        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // Generate character textures
        generateCharacterTextures();
    }
    
    ~TextRenderer() {
        // Clean up textures
        for (auto& pair : characters) {
            glDeleteTextures(1, &pair.second.textureID);
        }
        glDeleteVertexArrays(1, &textVAO);
        glDeleteBuffers(1, &textVBO);
        glDeleteProgram(textShaderProgram);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
    
    void generateCharacterTextures() {
        // Disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYPE: Failed to load Glyph: " << (int)c << std::endl;
                continue;
            }

            // Generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            // Create texture with proper format
            if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0) {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
                );
            } else {
                // Create a small 1x1 texture for empty characters
                unsigned char emptyPixel = 0;
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &emptyPixel);
            }

            // Set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    void renderText(std::string text, float worldX, float worldY, float worldZ, float scale, glm::mat4 projection, glm::mat4 view) {
        // Enable 3D rendering state for text - disable depth test so text always appears on top
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Activate corresponding render state
        glUseProgram(textShaderProgram);
        glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), 1.0f, 1.0f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(textVAO);

        // Set up matrices for 3D world space
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Position text slightly below the cube in world space and center horizontally
        float textY = worldY - 0.8f; // Position below the cube
        float textZ = worldZ; // Same Z as cubes for consistent depth

        // Calculate total text width and center it horizontally
        float totalWidth = 0.0f;
        for (char c : text) {
            Character ch = characters[c];
            totalWidth += (ch.advance >> 6) * scale * 0.005f;
        }
        float currentX = worldX - totalWidth * 0.5f; // Center the text

        // Iterate through all characters
        std::string::const_iterator c;
        for (c = text.begin(); c != text.end(); c++) {
            Character ch = characters[*c];

            // Create model matrix for this character's position in 3D space
            // Position the text in world space
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(currentX, textY, textZ));

            // Rotate text upwards towards the camera (around X-axis)
            model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(1.0f, 0.0f, 0.0f));

            // Apply scaling
            model = glm::scale(model, glm::vec3(scale * 0.005f)); // Scale to appropriate size

            glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            float xpos = ch.bearing.x;
            float ypos = -(ch.size.y - ch.bearing.y); // Flip Y for proper orientation

            float w = ch.size.x;
            float h = ch.size.y;

            // Update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };

            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureID);

            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            currentX += (ch.advance >> 6) * scale * 0.005f; // Bitshift by 6 to get value in pixels (2^6 = 64)
        }

        // Clean up state
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST); // Restore depth test for 3D scene
    }
};

// === Console input thread for interactive queries ===
void consoleReader() {
    std::cout
        << "\n=== Controls (console) ===\n"
        << "Enter shininess value {2,4,8,16,32,64,128,256} to focus.\n"
        << "Enter 0 to show ALL cubes again.\n"
        << "Enter q to quit the app.\n\n> " << std::flush;

    std::string line;
    while (g_keepReading && std::getline(std::cin, line)) {
        if (line == "q" || line == "Q") {
            std::cout << "Requested quit. Close the window to exit.\n> " << std::flush;
            continue;
        }

        std::istringstream iss(line);
        int v;
        if (!(iss >> v)) {
            std::cout << "Invalid input. Try again.\n> " << std::flush;
            continue;
        }

        if (v == 0) {
            g_focusIndex = -1;
            std::cout << "Showing ALL cubes.\n> " << std::flush;
            continue;
        }

        int idx = findShininessIndex(v);
        if (idx >= 0) {
            g_focusIndex = idx;
            std::cout << "Focusing shininess " << v << ".\n> " << std::flush;
        } else {
            std::cout << "Unknown shininess. Use {2,4,8,16,32,64,128,256} or 0.\n> " << std::flush;
        }
    }
}

// Global text renderer instance
TextRenderer* textRenderer = nullptr;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // Fix GTK/libdecor warning on Linux
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "8 Cubes with Lights + Camera", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    
    // Print OpenGL version info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // build shaders (check paths)
    std::cout << "Loading shaders..." << std::endl;
    Shader lightingShader("shaders/2.2.basic_lighting.vs", "shaders/2.2.basic_lighting.fs");
    Shader lightCubeShader("shaders/2.2.light_cube.vs", "shaders/2.2.light_cube.fs");
    std::cout << "Shaders loaded successfully!" << std::endl;

    // cube vertices (position + normal)
    float vertices[] = {
        -0.5f,-0.5f,-0.5f,0.0f,0.0f,-1.0f,  0.5f,-0.5f,-0.5f,0.0f,0.0f,-1.0f,  0.5f,0.5f,-0.5f,0.0f,0.0f,-1.0f,
         0.5f,0.5f,-0.5f,0.0f,0.0f,-1.0f, -0.5f,0.5f,-0.5f,0.0f,0.0f,-1.0f, -0.5f,-0.5f,-0.5f,0.0f,0.0f,-1.0f,

        -0.5f,-0.5f,0.5f,0.0f,0.0f,1.0f,  0.5f,-0.5f,0.5f,0.0f,0.0f,1.0f,  0.5f,0.5f,0.5f,0.0f,0.0f,1.0f,
         0.5f,0.5f,0.5f,0.0f,0.0f,1.0f, -0.5f,0.5f,0.5f,0.0f,0.0f,1.0f, -0.5f,-0.5f,0.5f,0.0f,0.0f,1.0f,

        -0.5f,0.5f,0.5f,-1.0f,0.0f,0.0f, -0.5f,0.5f,-0.5f,-1.0f,0.0f,0.0f, -0.5f,-0.5f,-0.5f,-1.0f,0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,-1.0f,0.0f,0.0f, -0.5f,-0.5f,0.5f,-1.0f,0.0f,0.0f, -0.5f,0.5f,0.5f,-1.0f,0.0f,0.0f,

         0.5f,0.5f,0.5f,1.0f,0.0f,0.0f,  0.5f,0.5f,-0.5f,1.0f,0.0f,0.0f,  0.5f,-0.5f,-0.5f,1.0f,0.0f,0.0f,
         0.5f,-0.5f,-0.5f,1.0f,0.0f,0.0f,  0.5f,-0.5f,0.5f,1.0f,0.0f,0.0f,  0.5f,0.5f,0.5f,1.0f,0.0f,0.0f,

        -0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,  0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,  0.5f,-0.5f,0.5f,0.0f,-1.0f,0.0f,
         0.5f,-0.5f,0.5f,0.0f,-1.0f,0.0f, -0.5f,-0.5f,0.5f,0.0f,-1.0f,0.0f, -0.5f,-0.5f,-0.5f,0.0f,-1.0f,0.0f,

        -0.5f,0.5f,-0.5f,0.0f,1.0f,0.0f,  0.5f,0.5f,-0.5f,0.0f,1.0f,0.0f,  0.5f,0.5f,0.5f,0.0f,1.0f,0.0f,
         0.5f,0.5f,0.5f,0.0f,1.0f,0.0f, -0.5f,0.5f,0.5f,0.0f,1.0f,0.0f, -0.5f,0.5f,-0.5f,0.0f,1.0f,0.0f
    };

    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::vec3 cubePositions[8] = {
{-3.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, -1.0f}, {3.0f, 0.0f, -1.0f},
{-3.0f, -2.0f, -1.0f}, {-1.0f, -2.0f, -1.0f}, {1.0f, -2.0f, -1.0f}, {3.0f, -2.0f, -1.0f}
};
glm::vec3 lightPositions[8] = {
{-3.0f, 0.0f, 10.0f}, {-1.0f, 0.0f, 10.0f}, {1.0f, 0.0f, 10.0f}, {3.0f, 0.0f, 10.0f},
{-3.0f, -2.0f, 10.0f}, {-1.0f, -2.0f, 10.0f}, {1.0f, -2.0f, 10.0f}, {3.0f, -2.0f, 10.0f}
};

    
    // Shininess values matching the reference image
    float shininessValues[8] = {2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f};

    // Initialize text renderer
    // Initialize text renderer
textRenderer = new TextRenderer();

// Start input thread (runs parallel to render loop)
std::thread inputThread(consoleReader);

std::cout << "Starting render loop..." << std::endl;

    
    std::cout << "Starting render loop..." << std::endl;
    std::cout << "Controls: WASD to move, Arrow keys to look around, ESC to exit" << std::endl;
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        int focus = g_focusIndex.load();
if (focus == -1) {
    // === Default: show all 8 cubes ===
    for (int i = 0; i < 8; ++i)
    {
        float dim = 1.0f - (i * 0.1f);
        if (dim < 0.3f) dim = 0.3f;

        lightingShader.use();
lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);

// Dim only the top row lights (indices 0–3)
float brightnessScale = (i < 4) ? 0.7f : 1.0f;  // top row 40% brightness

glm::vec3 adjustedLightColor = glm::vec3(dim * brightnessScale);
lightingShader.setVec3("lightColor", adjustedLightColor);

// Send other uniforms as usual
lightingShader.setVec3("lightPos", lightPositions[i]);
lightingShader.setVec3("viewPos", camera.Position);
lightingShader.setFloat("shininess", shininessValues[i]);

        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        lightingShader.setMat4("model", model);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPositions[i]);
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);

        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        std::ostringstream oss;
        oss << static_cast<int>(shininessValues[i]);
        renderText(oss.str(), cubePositions[i].x, cubePositions[i].y, cubePositions[i].z, 1.0f, projection, view);
    }
} else {
    // === Focus mode: show only selected cube ===
    int i = focus;
    glm::vec3 target = cubePositions[i];
    camera.Position = target + glm::vec3(0.0f, 0.0f, 3.0f);

    lightingShader.use();
    lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
    float dim = 1.0f - (i * 0.1f); if (dim < 0.3f) dim = 0.3f;
    lightingShader.setVec3("lightColor", dim, dim, dim);
    lightingShader.setVec3("lightPos", lightPositions[i]);
    lightingShader.setVec3("viewPos", camera.Position);
    lightingShader.setFloat("shininess", shininessValues[i]);
    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cubePositions[i]);
    lightingShader.setMat4("model", model);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    lightCubeShader.use();
    lightCubeShader.setMat4("projection", projection);
    lightCubeShader.setMat4("view", view);
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPositions[i]);
    model = glm::scale(model, glm::vec3(0.2f));
    lightCubeShader.setMat4("model", model);
    glBindVertexArray(lightCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    std::ostringstream oss;
    oss << static_cast<int>(shininessValues[i]);
    renderText(oss.str(), cubePositions[i].x, cubePositions[i].y, cubePositions[i].z, 1.5f, projection, view);
}


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);
    
    // Clean up text renderer
    delete textRenderer;
    g_keepReading = false;
    if (inputThread.joinable()) inputThread.detach();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Rotation using arrow keys
    const float rotationSpeed = 50.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.Yaw -= rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.Yaw += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.Pitch += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.Pitch -= rotationSpeed;

    if (camera.Pitch > 89.0f) camera.Pitch = 89.0f;
    if (camera.Pitch < -89.0f) camera.Pitch = -89.0f;

    // Update camera vectors
    camera.ProcessMouseMovement(0.0f, 0.0f);
}

void renderText(const std::string& text, float worldX, float worldY, float worldZ, float scale, glm::mat4 projection, glm::mat4 view)
{
    if (!textRenderer) return;

    // Render the text using the proper FreeType text renderer in 3D space
    textRenderer->renderText(text, worldX, worldY, worldZ, scale, projection, view);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
