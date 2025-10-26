#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/shader_m.h"
#include "include/camera.h"

#include <iostream>

// function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 7.0f, 0.0f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -90.0f,  -90.0f);  // yaw, pitch
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

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

    glEnable(GL_DEPTH_TEST);

    // build shaders (check paths)
    Shader lightingShader("shaders/2.2.basic_lighting.vs", "shaders/2.2.basic_lighting.fs");
    Shader lightCubeShader("shaders/2.2.light_cube.vs", "shaders/2.2.light_cube.fs");

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
        {-3.0f, 0.0f,  1.0f}, {-1.0f, 0.0f,  1.0f}, {1.0f, 0.0f,  1.0f}, {3.0f, 0.0f,  1.0f}
    };
    glm::vec3 lightPositions[8] = {
        {-3.0f, 1.5f, -1.0f}, {-1.0f, 1.5f, -1.0f}, {1.0f, 1.5f, -1.0f}, {3.0f, 1.5f, -1.0f},
        {-3.0f, 1.5f,  1.0f}, {-1.0f, 1.5f,  1.0f}, {1.0f, 1.5f,  1.0f}, {3.0f, 1.5f,  1.0f}
    };

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

        for (int i = 0; i < 8; ++i)
        {
            float dim = 1.0f - (i * 0.1f);
            if (dim < 0.3f) dim = 0.3f;

            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
            lightingShader.setVec3("lightColor", dim, dim, dim);
            lightingShader.setVec3("lightPos", lightPositions[i]);
            lightingShader.setVec3("viewPos", camera.Position);
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
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
