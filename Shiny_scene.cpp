#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// ------------ Shader utility ------------
static GLuint makeShader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(id,512,nullptr,log);
        std::cerr << "Shader error:\n" << log << std::endl;
    }
    return id;
}
static GLuint makeProgram(const char* vs, const char* fs) {
    GLuint v = makeShader(GL_VERTEX_SHADER, vs);
    GLuint f = makeShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// ------------ Vertex & Fragment shader source ------------
const char* vShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos,1.0);
}
)";

const char* fShaderSrc = R"(
#version 330 core
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main() {
    // Ambient
    vec3 ambient = light.ambient * material.ambient;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";

// ------------ Cube vertex data (pos + normal) ------------
float vertices[] = {
    // positions         // normals
   -0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,
    0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,
    0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,
    0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,
   -0.5f, 0.5f,-0.5f,  0.0f,0.0f,-1.0f,
   -0.5f,-0.5f,-0.5f,  0.0f,0.0f,-1.0f,

   -0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,
    0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,
    0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,
    0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,
   -0.5f, 0.5f, 0.5f,  0.0f,0.0f,1.0f,
   -0.5f,-0.5f, 0.5f,  0.0f,0.0f,1.0f,

   -0.5f, 0.5f, 0.5f, -1.0f,0.0f,0.0f,
   -0.5f, 0.5f,-0.5f, -1.0f,0.0f,0.0f,
   -0.5f,-0.5f,-0.5f, -1.0f,0.0f,0.0f,
   -0.5f,-0.5f,-0.5f, -1.0f,0.0f,0.0f,
   -0.5f,-0.5f, 0.5f, -1.0f,0.0f,0.0f,
   -0.5f, 0.5f, 0.5f, -1.0f,0.0f,0.0f,

    0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,
    0.5f, 0.5f,-0.5f,  1.0f,0.0f,0.0f,
    0.5f,-0.5f,-0.5f,  1.0f,0.0f,0.0f,
    0.5f,-0.5f,-0.5f,  1.0f,0.0f,0.0f,
    0.5f,-0.5f, 0.5f,  1.0f,0.0f,0.0f,
    0.5f, 0.5f, 0.5f,  1.0f,0.0f,0.0f,

   -0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,
    0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,
    0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,
    0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,
   -0.5f,-0.5f, 0.5f,  0.0f,-1.0f,0.0f,
   -0.5f,-0.5f,-0.5f,  0.0f,-1.0f,0.0f,

   -0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,
    0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f,
    0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,
    0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,
   -0.5f, 0.5f, 0.5f,  0.0f,1.0f,0.0f,
   -0.5f, 0.5f,-0.5f,  0.0f,1.0f,0.0f
};

int main() {
    if (!glfwInit()) {
    std::cerr << "❌ Failed to initialize GLFW" << std::endl;
    return -1;
}

glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

GLFWwindow* window = glfwCreateWindow(1000, 700, "Phong Shininess Demo", nullptr, nullptr);
if (!window) {
    std::cerr << "❌ Failed to create GLFW window (check your X server / OpenGL drivers)" << std::endl;
    glfwTerminate();
    return -1;
}

glfwMakeContextCurrent(window);

if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "❌ Failed to initialize GLAD" << std::endl;
    glfwTerminate();
    return -1;
}

std::cout << "✅ OpenGL initialized successfully!" << std::endl;
glEnable(GL_DEPTH_TEST);

    glEnable(GL_DEPTH_TEST);

    GLuint program = makeProgram(vShaderSrc,fShaderSrc);

    // Cube VAO/VBO
    GLuint VBO, VAO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Uniform setup
    glUseProgram(program);
    GLint modelLoc = glGetUniformLocation(program,"model");
    GLint viewLoc = glGetUniformLocation(program,"view");
    GLint projLoc = glGetUniformLocation(program,"projection");

    // Light + material
    glUniform3f(glGetUniformLocation(program,"light.position"),1.2f,1.0f,2.0f);
    glUniform3f(glGetUniformLocation(program,"light.ambient"),0.1f,0.1f,0.1f);
    glUniform3f(glGetUniformLocation(program,"light.diffuse"),0.8f,0.8f,0.8f);
    glUniform3f(glGetUniformLocation(program,"light.specular"),1.0f,1.0f,1.0f);
    glUniform3f(glGetUniformLocation(program,"viewPos"),0.0f,0.0f,6.0f);

    glm::vec3 ambient(1.0f,0.5f,0.31f);
    glm::vec3 diffuse(1.0f,0.5f,0.31f);
    glm::vec3 specular(0.5f,0.5f,0.5f);
    glUniform3fv(glGetUniformLocation(program,"material.ambient"),1,glm::value_ptr(ambient));
    glUniform3fv(glGetUniformLocation(program,"material.diffuse"),1,glm::value_ptr(diffuse));
    glUniform3fv(glGetUniformLocation(program,"material.specular"),1,glm::value_ptr(specular));

    std::vector<float> shininess = {2,4,8,16,32,64,128,256};

    while(!glfwWindowShouldClose(window)){
        glClearColor(0.12f,0.12f,0.12f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),1000.0f/700.0f,0.1f,100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0,0,6),glm::vec3(0,0,0),glm::vec3(0,1,0));
        glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));

        glBindVertexArray(VAO);
        int rows=2, cols=4;
        float gapX=2.0f, gapY=2.0f;
        float startX = -((cols-1)*gapX)/2.0f;
        float startY = 0.8f;
        int i=0;
        for(int r=0;r<rows;++r){
            for(int c=0;c<cols;++c){
                glm::mat4 model(1.0f);
                model = glm::translate(model, glm::vec3(startX + c*gapX, startY - r*gapY, 0.0f));
                model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0,1,0));
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));
                glUniform1f(glGetUniformLocation(program,"material.shininess"), shininess[i]);
                glDrawArrays(GL_TRIANGLES,0,36);
                i++;
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
