#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include "cylinder.h"

#define PI 3.1415927

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Michael Linsenbigler Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;           // Handle for the vertex array object
        GLuint vbo;           // Handles for the vertex buffer objects
        GLuint nXboxVertices;    // Number of vertices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMesh;

    // Textures
    GLuint tableTex;
    GLuint vidGameTex;
    GLuint ventTex;
    GLuint canTex;
    GLuint canTopTex;
    GLuint speakerTex;

    // Texture scale
    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_CLAMP;

    // Shader program
    GLuint gProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // light color
    glm::vec3 gLightColor(1.0f, 1.0f, 0.8f);
    glm::vec3 gFillLightColor(1.0f, 0.8f, 0.8f);

    // Light position and scale
    glm::vec3 keyLightPos(-5.0f, 5.0f, -5.0f);
    glm::vec3 fillLightPos(3.0f, -5.0f, 0.0f);

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool orthoP = false;


const GLchar* lightVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Object Fragment Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor; // For outgoing object color to the GPU

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 lightColor;
    uniform vec3 fillLightColor;
    uniform vec3 keyLightPos;
    uniform vec3 fillLightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture; // Useful when working with multiple textures

    vec4 calculateKeyLight();
    vec4 calculateFillLight();

void main()
{

    vec4 keyPhong = calculateKeyLight();
    vec4 fillPhong = calculateFillLight();

    fragmentColor = keyPhong + fillPhong; // Send lighting results to GPU
}

vec4 calculateKeyLight() {
    //Calculate Ambient lighting*/
    float ambientStrength = 0.10f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on objects
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.4f; // Set specular light strength
    float highlightSize = 1.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    return vec4(phong, 1.0);
}

vec4 calculateFillLight() {
    //Calculate Ambient lighting*/
    float ambientStrength = 0.10f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * fillLightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(fillLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on objects
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * fillLightColor; // Generate diffuse light colora

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 1.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * fillLightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    return vec4(phong, 1.0);
}

);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh);

    // Create the shader program
    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "white-tiles1.jpg";
    if (!UCreateTexture(texFilename, vidGameTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "vent.jpg";
    if (!UCreateTexture(texFilename, ventTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "vert-table.jfif";
    if (!UCreateTexture(texFilename, tableTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "red.jfif";
    if (!UCreateTexture(texFilename, canTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "can-top1.jpg";
    if (!UCreateTexture(texFilename, canTopTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "blue tex.jpg";
    if (!UCreateTexture(texFilename, speakerTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to
    glUseProgram(gProgramId);
    // We set xbox as 0
    glUniform1i(glGetUniformLocation(gProgramId, "uXbox"), 0);
    // We set table as 1
    glUniform1i(glGetUniformLocation(gProgramId, "uTable"), 1);
    // Set vent as 2
    glUniform1i(glGetUniformLocation(gProgramId, "uVent"), 2);
    // Set can as 3
    glUniform1i(glGetUniformLocation(gProgramId, "uCan"), 3);
    // Set can top as 4
    glUniform1i(glGetUniformLocation(gProgramId, "uCanTop"), 4);


    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release shader program
    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        orthoP = !orthoP;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 0.5
    glm::mat4 scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
    // 2. Rotates shape by 37 degrees on the x axis
    glm::mat4 rotation = glm::rotate(37.0f, glm::vec3(1.0, 0.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection;
    if (orthoP)
    {
        projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f); //switch camera to orthographic view
        //cout << "orthographic" << endl;
    }
    else {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        //cout << "perspective" << endl;
    }

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Shader program for the cub color, light color, light position, and camera position
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint fillLightColorLoc = glGetUniformLocation(gProgramId, "fillLightColor");
    GLint keyLightPositionLoc = glGetUniformLocation(gProgramId, "keyLightPos");
    GLint fillLightPositionLoc = glGetUniformLocation(gProgramId, "fillLightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(fillLightColorLoc, gFillLightColor.r, gFillLightColor.g, gFillLightColor.b);
    glUniform3f(keyLightPositionLoc, keyLightPos.x, keyLightPos.y, keyLightPos.z);
    glUniform3f(fillLightPositionLoc, fillLightPos.x, fillLightPos.y, fillLightPos.z);

    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);

    //tex and draw xbox
    glBindTexture(GL_TEXTURE_2D, vidGameTex);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //tex and draw the table
    glBindTexture(GL_TEXTURE_2D, tableTex);
    glDrawArrays(GL_TRIANGLES, 78, 6);

    //tex vent
    glBindTexture(GL_TEXTURE_2D, ventTex);

    //tex and draw vent
    glBindTexture(GL_TEXTURE_2D, ventTex);
    glDrawArrays(GL_TRIANGLES, 36, 18);

    //place speaker
    translation = glm::translate(glm::vec3(0.1f, -1.2f, 0.0f));
    //rotation = glm::rotate(.0f, glm::vec3(0.8f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //tex and draw speaker
    glBindTexture(GL_TEXTURE_2D, speakerTex);
    //glDrawArrays(GL_TRIANGLES, 54, 24);
    glDrawArrays(GL_TRIANGLES, 54, 24);

    //tex can
    glBindTexture(GL_TEXTURE_2D, canTex);

    //place can
    translation = glm::translate(glm::vec3(-1.0f, 0.0f, 0.1f));
    rotation = glm::rotate(45.0f, glm::vec3(0.8f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //draw can
    static_meshes_3D::Cylinder C(0.65, 36, 2.0, true, true, true);
    C.render();

    //tex can top
    glBindTexture(GL_TEXTURE_2D, canTopTex);

    //place can top   
    translation = glm::translate(glm::vec3(-1.0f, 0.0f, 0.12f));
    rotation = glm::rotate(45.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //draw can top
    static_meshes_3D::Cylinder C1(0.60, 36, 2.1, true, true, true);
    C1.render();

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);
}

//Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    const float REPEAT = 3;

    // Position and Color data
    GLfloat xboxVerts[] = {
        // Vertex Positions        // Normals          //Texture

         2.8f,  2.0f,  0.0f,    0.0f,  1.0f,  0.0f,   0.0f,   0.0f,   // top right 0
         2.8f,  0.2f,  0.0f,    0.0f,  1.0f,  0.0f,   0.0f,   REPEAT, // bottom right 1
        -0.8f,  2.0f,  0.0f,    0.0f,  1.0f,  0.0f,   REPEAT, REPEAT, // top left 3
        -0.8f,  2.0f,  0.0f,    0.0f,  1.0f,  0.0f,   REPEAT, 0.0f,   // top left 3
         2.8f,  0.2f,  0.0f,    0.0f,  1.0f,  0.0f,   0.0f,   0.0f,   // bottom right 1
        -0.8f,  0.2f,  0.0f,    0.0f,  1.0f,  0.0f,   REPEAT, REPEAT, // bottom left 2


         2.8f,  2.0f, -1.0f,    0.0f, -1.0f,  0.0f,   0.0f, 0.0f,     // 4 back top right
         2.8f,  0.2f, -1.0f,    0.0f, -1.0f,  0.0f,   REPEAT, 0.0f,   // 5 back bot  right
        -0.8f,  2.0f, -1.0f,    0.0f, -1.0f,  0.0f,   REPEAT, REPEAT, // 7 back top left
        -0.8f,  2.0f, -1.0f,    0.0f, -1.0f,  0.0f,   0.0f, 0.0f,     // 7 back top left
         2.8f,  0.2f, -1.0f,    0.0f, -1.0f,  0.0f,   0.0f, REPEAT,   // 5 back bot  right
        -0.8f,  0.2f, -1.0f,    0.0f, -1.0f,  0.0f,   REPEAT, REPEAT, // 6 back bot left


        -0.8f,  2.0f, -1.0f,    0.0f,  0.0f, -1.0f,   0.0f, 0.0f,     // 7 back top left
         2.8f,  2.0f, -1.0f,    0.0f,  0.0f, -1.0f,   0.0f, REPEAT,   // 4 back top right
         2.8f,  2.0f,  0.0f,    0.0f,  0.0f, -1.0f,   REPEAT, REPEAT, // top right 0
         2.8f,  2.0f,  0.0f,    0.0f,  0.0f, -1.0f,   0.0f, 0.0f,     // top right 0
        -0.8f,  2.0f,  0.0f,    0.0f,  0.0f, -1.0f,   0.0f, REPEAT,   // top left 3
        -0.8f,  2.0f, -1.0f,    0.0f,  0.0f, -1.0f,   REPEAT, REPEAT, // 7 back top left


         2.8f,  0.2f,  0.0f,    0.0f,  0.0f,  1.0f,    REPEAT, REPEAT, // bottom right 1
        -0.8f,  0.2f, -1.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,     // 6 back bot left
         2.8f,  0.2f, -1.0f,    0.0f,  0.0f,  1.0f,    REPEAT, 0.0f,   // 5 back bot  right 
        -0.8f,  0.2f, -1.0f,    0.0f,  0.0f,  1.0f,    REPEAT, REPEAT, // 6 back bot left
         2.8f,  0.2f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,     // bottom right 1
        -0.8f,  0.2f,  0.0f,    0.0f,  0.0f,  1.0f,    REPEAT, 0.0f,   // bottom left 2


        -0.8f,  2.0f, -1.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,     // 7 back top left
        -0.8f,  2.0f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, REPEAT,   // top left 3
        -0.8f,  0.2f,  0.0f,   -1.0f,  0.0f,  0.0f,    REPEAT, REPEAT, // bottom left 2 
        -0.8f,  0.2f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,     // bottom left 2
        -0.8f,  0.2f, -1.0,    -1.0f,  0.0f,  0.0f,    0.0f, REPEAT,   // 6 back bot left
        -0.8f,  2.0f, -1.0f,   -1.0f,  0.0f,  0.0f,    REPEAT, REPEAT, // 7 back top left


         2.8f,  0.2f, -1.0f,    1.0f,  0.0f,  0.0f,    REPEAT, REPEAT, // 5 back bot  right
         2.8f,  2.0f,  0.0f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,     // top right 0 
         2.8f,  0.2f,  0.0f,    1.0f,  0.0f,  0.0f,    REPEAT, 0.0f,   // bottom right 1
         2.8f,  2.0f,  0.0f,    1.0f,  0.0f,  0.0f,    REPEAT, REPEAT, // top right 0 
         2.8f,  0.2f, -1.0f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,     // 5 back bot  right
         2.8f,  2.0f, -1.0f,    1.0f,  0.0f,  0.0f,    REPEAT, 0.0f,   // 4 back top right


        // xbox vent
         1.6f,  1.8f,  0.0f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    // 8 top of vent pyramid
         2.3f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    // 9 right side of vent pyramid
         1.6f,  1.1f,  0.1f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    // 11 center apex

         1.6f,  0.3f,  0.0f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         2.3f,  1.0f,  0.0f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    // 9 right side of vent pyramid  
         1.6f,  1.1f,  0.1f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    // 11 center apex

         1.6f,  0.3f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         1.0f,  1.0f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    // 12 left side of vent pyramid
         1.6f,  1.1f,  0.1f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    // 11 center apex

         1.0f,  1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 12 left side of vent pyramid
         1.6f,  1.1f,  0.1f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    // 11 center apex
         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 8 top of vent pyramid
 
         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 8 top of vent pyramid
         1.6f,  0.3f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         1.0f,  1.0f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    // 12 left side of vent pyramid

         1.6f,  1.8f,  0.0f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    // 8 top of vent pyramid
         1.6f,  0.3f,  0.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         2.3f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    // 9 right side of vent pyramid


         // Speaker
         1.6f,  1.8f,  0.0f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    // 8 top of vent pyramid
         2.3f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    // 9 right side of vent pyramid
         1.9f,  1.2f,  0.8f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    // 13 right center apex

         1.9f,  1.2f,  0.8f,    0.5f,  0.0f,  0.0f,    1.0f, 1.0f,    // 13 right center apex
         1.2f, -1.0f,  0.0f,    0.5f,  0.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         2.3f,  1.0f,  0.0f,    0.5f,  0.0f,  0.0f,    1.0f, 0.0f,    // 9 right side of vent pyramid  
           
         1.2f, -1.0f,  0.0f,    0.5f,  0.0f,  0.0f,    1.0f, 1.0f,    // 10 bottom of vent pyramid
         1.9f,  1.2f,  0.8f,    0.5f,  0.0f,  0.0f,    0.0f, 0.0f,    // 13 right center apex
         0.8f, -0.3f,  0.8f,    0.5f,  0.0f,  0.0f,    1.0f, 0.0f,    // 11 left center apex

         1.2f, -1.0f,  0.0f,    1.0f,  1.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         0.2f,  0.0f,  0.0f,    1.0f,  1.0f,  0.0f,    0.0f, 1.0f,    // 12 left side of vent pyramid
         0.8f, -0.3f,  0.8f,    1.0f,  1.0f,  0.0f,    1.0f, 1.0f,    // 11 left center apex

         0.2f,  0.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 12 left side of vent pyramid
         0.8f, -0.3f,  0.8f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 11 left center apex
         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    // 8 top of vent pyramid

         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 8 top of vent pyramid
         1.9f,  1.2f,  0.8f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    // 13 right center apex
         0.8f, -0.3f,  0.8f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 11 left center apex

         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 8 top of vent pyramid
         1.2f, -1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         0.2f,  0.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,    // 12 left side of vent pyramid

         1.6f,  1.8f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    // 8 top of vent pyramid
         1.2f, -1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    // 10 bottom of vent pyramid
         2.3f,  1.0f,  0.0f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    // 9 right side of vent pyramid


        -5.0f,  5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    // plane for table
         5.0f,  5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
        -5.0f, -5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,
         5.0f,  5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,
         5.0f, -5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
        -5.0f, -5.0f, -1.1f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNorm = 3;
    const GLuint floatsPerUV = 2;


    mesh.nXboxVertices = sizeof(xboxVerts) / (sizeof(xboxVerts[0]) * (floatsPerVertex + floatsPerUV + floatsPerNorm));


    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(xboxVerts), xboxVerts, GL_STATIC_DRAW);// Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV + floatsPerNorm);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNorm, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNorm)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); //Unbind the VAO
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, &mesh.vbo);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}