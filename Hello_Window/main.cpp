#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"
#include "model.h"


#include <iostream>
#include "stb_image.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


unsigned int loadTexture(const char* path, bool gammaCorrection);

void renderQuad();
void renderCube();
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//CAMERA PARAMETERS
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//INITIAL TIMIMGS
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Deferred Shading by Ali Viktor Dias", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    //COMPILING SHADER PROGRAMS USING SHADER CLASS
    Shader shaderGeometryPass("../g_buffer.vs", "../g_buffer.fs");
    Shader shaderLightingPass("../deferred_shading.vs", "../deferred_shading.fs");
    Shader shaderLightBox("../deferred_light_box.vs", "../deferred_light_box.fs");


    Model newModel("../resources/objects/newmodel/City center at night .obj");

    //ADDING TWO 3D MODELS
    std::vector<glm::vec3> objectPositions;
    objectPositions.push_back(glm::vec3(-0.1, 0, -0.5));
    objectPositions.push_back(glm::vec3(-0.1, 0, 1));




    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);


    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);


    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);


    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    //renderbuffer
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);




    //LIGHTING PARAMETERS
    const unsigned int NR_LIGHTS = 16;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(11);




    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        //INITIAL POSITIONS
        float xPos = ((rand() % 100) / 87.0) * 3;
        float yPos = ((rand() % 100) / 87.0) * 2.1;
        float zPos = ((rand() % 100) / 87.0) * 2.1;
        lightPositions.push_back(glm::vec3(xPos, yPos, zPos));

        //COLORS
        float rColor = 0.4;
        float gColor = 0.4;
        float bColor = 1;
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedoSpec", 2);


    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);


        //STATIC LIGHTS

        glm::vec3 fixedPositions[8] = {
            glm::vec3(-0.3, 0.31f, 1.622f),
            glm::vec3(-0.3, 0.31f, 1.182f),

                                    glm::vec3(-0.31, 0.31f, 0.131f),//pair1
                                    glm::vec3(-0.31, 0.31f, -0.321f),//pair2

            glm::vec3(0.1, 0.31f, 1.622f),
            glm::vec3(0.1, 0.31f, 1.182f),

                                    glm::vec3(0.121, 0.31f, 0.131f),//pair1
                                    glm::vec3(0.121, 0.31f, -0.321f)//pair 2
        };



        //DYNAMIC LIGHTS


        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            float radius = 1.5f;
            float orbitSpeed = 0.5f;

            if (i < 8)
            {
                lightPositions[i] = fixedPositions[i];
            }
            else
            {
                // Calculate new positions for the remaining cubes in a circular orbit
                float xPos = cos(orbitSpeed * currentFrame + (i - 8) * 5) * radius;
                float yPos = fixedPositions[0].y; // Keep the original y position
                float zPos = sin(orbitSpeed * currentFrame + (i - 8) * 0.5f) * radius;

                lightPositions[i] = glm::vec3(xPos, yPos, zPos);
            }
        }
        //RENDERING
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




        //STEP 1. Geometry Pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shaderGeometryPass.use();
        shaderGeometryPass.setMat4("projection", projection); //passing projection matrix
        shaderGeometryPass.setMat4("view", view);//passing view matrix


        for (unsigned int i = 0; i < objectPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, objectPositions[i]);
            model = glm::scale(model, glm::vec3(0.25f));
            shaderGeometryPass.setMat4("model", model);
            newModel.Draw(shaderGeometryPass);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);



        //STEP 2. Lighting Pass. Calculating lighting parameters for whole screen using information from g-buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderLightingPass.use(); //using correct shader for lighting pass
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);



        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);

            const float constant = 1.0;
            const float linear = 2.0;
            const float quadratic = 3.0;

            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);


            const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
            float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
            shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
        }
        shaderLightingPass.setVec3("viewPos", camera.Position);


        renderQuad();


        // Copying depth buffer contents from g-buffer into main framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);



        //Rendering light sources themselves
        shaderLightBox.use();
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPositions[i]);
            model = glm::scale(model, glm::vec3(0.125f));
            shaderLightBox.setMat4("model", model);
            shaderLightBox.setVec3("lightColor", lightColors[i]);
            renderCube();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}



unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            //back face
            -0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
             0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.4f, 0.4f,
             0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.4f, 0.0f,
             0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.4f, 0.4f,
            -0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
            -0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 0.0f, 0.4f,

            //front face
           -0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
            0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.4f, 0.0f,
            0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.4f, 0.4f,
            0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.4f, 0.4f,
           -0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 0.4f,
           -0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

           // left face
          -0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 0.4f, 0.0f,
          -0.1f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 0.4f, 0.4f,
          -0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 0.4f,
          -0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 0.4f,
          -0.1f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
          -0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 0.4f, 0.0f,

          //right face
          0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f, 0.4f, 0.0f,
          0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f, 0.0f, 0.4f,
          0.1f,  0.1f, -0.1f,  1.0f,  0.0f,  0.0f, 0.4f, 0.4f,
          0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f, 0.0f, 0.4f,
          0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f, 0.4f, 0.0f,
          0.1f, -0.1f,  0.1f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

          //bottom face
         -0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 0.4f,
          0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 0.4f, 0.4f,
          0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.4f, 0.0f,
          0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.4f, 0.0f,
         -0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         -0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 0.0f, 0.4f,

         //upper face
        -0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 0.4f,
         0.1f,  0.1f , 0.1f,  0.0f,  1.0f,  0.0f, 0.4f, 0.0f,
         0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 0.4f, 0.4f,
         0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 0.4f, 0.0f,
        -0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 0.4f,
        -0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
        };
        glGenVertexArrays(1, &cubeVAO); //creating vertex array obj
        glGenBuffers(1, &cubeVBO);//creating vertex buffer obj

        //filling vbo
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
           -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
           -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
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
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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
    camera.ProcessMouseScroll(yoffset);
}