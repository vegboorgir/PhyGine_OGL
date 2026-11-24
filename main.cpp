#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "shader.h"
#include "camera.h"
#include "rigidbox.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void stepPhysics(float dt);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(4.0f, 2.0f, 7.0f), glm::vec3(0.0f, 1.0f, 0.0f), -120.0f, -15.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

RigidBox cube;
RigidBox ground;

const float GRAVITY = -9.8f;
const float GROUND_Y = -2.0f;

const float PHYSICS_DT = 1.0f / 120.0f;
float accumulator = 0.0f;

int fbw = SCR_WIDTH;
int fbh = SCR_HEIGHT;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Red Cube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    cube.position = glm::vec3(0.0f, 3.0f, 0.0f);
    cube.color = glm::vec3(1.0f, 0.2f, 0.3f);

    ground.position = glm::vec3(0.0f, GROUND_Y - 0.05f, 0.0f);
    ground.halfSize = glm::vec3(10.0f, 0.05f, 10.0f);
    ground.color = glm::vec3(0.3f, 0.6f, 0.3f);
    ground.isStatic = true;

    Shader ourShader("shader.vs", "shader.fs");


    // positions only (3 floats per vertex)
    float vertices[] = {
        // -z face (fixed)
        -0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,

        // +z face (unchanged)
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,

        // -x face (unchanged)
        -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,

        // +x face (fixed)
         0.5f,  0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,   0.5f,  0.5f,  0.5f,   0.5f, -0.5f,  0.5f,

        // -y face (unchanged)
        -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,

        // +y face (fixed)
        -0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f,  0.5f,  0.5f
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        accumulator += std::min(deltaTime, 0.25f);
        while (accumulator >= PHYSICS_DT) {
            stepPhysics(PHYSICS_DT);
            accumulator -= PHYSICS_DT;
        }
        ourShader.use();
        
        float aspect = fbh > 0 ? (float)fbw / fbh : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            aspect, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", camera.GetViewMatrix());

        glBindVertexArray(VAO);

        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::translate(groundModel, ground.position);
        groundModel = glm::scale(groundModel, ground.halfSize * 2.0f);
        ourShader.setMat4("model", groundModel);
        ourShader.setVec3("color", ground.color);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cube.position);
        ourShader.setMat4("model", model);
        ourShader.setVec3("color", cube.color);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(ourShader.ID);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    fbw = width;
    fbh = height;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void stepPhysics(float dt) {
    float groundTop = ground.position.y + ground.halfSize.y;
    cube.velocity.y += GRAVITY * dt;
    cube.position.y += cube.velocity.y * dt;

    if (cube.position.y - cube.halfSize.y < groundTop) {
        cube.position.y = groundTop + cube.halfSize.y;
        cube.velocity.y = -cube.velocity.y * 0.6f;
        if (fabs(cube.velocity.y) < 0.5f) cube.velocity.y = 0.0f;
    }
}