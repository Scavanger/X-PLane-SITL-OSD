#include "osdRenderer.h"

#include "helper.h"

#include "osd.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <XPLMDisplay.h>
#include "XPLMGraphics.h"

using namespace Helper;

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 transform;

void main() 
{
    gl_Position = transform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
} 
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2DArray textureArray;
uniform int layer;

void main()
{
	FragColor = texture(textureArray, vec3(TexCoord, layer));
}
)";

const int MARGIN = 30;

OsdRenderer::OsdRenderer()
{
    this->screen = std::vector<std::vector<uint16_t>>(DJI_ROWS, std::vector<uint16_t>(DJI_COLS));

    if (!glfwInit()) {
        Log("Unable to init GLWF");
        return;
    }

    if (glewInit() != GLEW_OK) {
        Log("Unable to init GLEW");
        return;
    }

    if (!this->createShader()) {
        Log("Unable to create OSD Shader");
        return;
    }

    this->intQuad();
}

OsdRenderer::~OsdRenderer()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    glDeleteBuffers(1, &this->EBO);
    glDeleteProgram(this->shader);
    glDeleteTextures(1, &textureArray);
}

GLuint OsdRenderer::compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        Log("Shader compilation failed: ", infoLog);
    }
    return shader;
}

bool OsdRenderer::createShader()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    this->shader = glCreateProgram();
    glAttachShader(this->shader, vertexShader);
    glAttachShader(this->shader, fragmentShader);
    glLinkProgram(this->shader);

    int success;
    glGetProgramiv(this->shader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(this->shader, 512, nullptr, infoLog);
        Log("Shader program linking failed: ", infoLog);
        return false;
    }

    this->transformLoc = glGetUniformLocation(this->shader, "transform");
    this->layerLoc = glGetUniformLocation(this->shader, "layer");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void OsdRenderer::intQuad()
{
    float vertices[] = {
        1.0f,  1.0f, 1.0f, 1.0f,  // right top
        1.0f, -1.0f, 1.0f, 0.0f,  // right bottom
       -1.0f, -1.0f, 0.0f, 0.0f,  // left bottom
       -1.0f,  1.0f, 0.0f, 1.0f   // left top
    };
    unsigned int indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OsdRenderer::loadTextureArray(std::vector<std::vector<uint8_t>> textures, int width, int height)
{
    if (!this->currentFontName.empty()) {
        glDeleteTextures(1, &textureArray);
    }

    XPLMGenerateTextureNumbers(reinterpret_cast<int*>(&this->textureArray), 1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureArray);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, textures.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (size_t i = 0; i < textures.size(); i++) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, textures[i].data());
    }
    
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    this->textureWidth = width;
    this->textureHeight = height;
}

void OsdRenderer::clearScreen()
{
    this->screen = std::vector<std::vector<uint16_t>>(DJI_ROWS, std::vector<uint16_t>(DJI_COLS));
}

void OsdRenderer::setCharacter(int row, int col, uint16_t character)
{
    this->screen[row][col] = character;
}

void OsdRenderer::LoadFont(std::shared_ptr<FontBase> font)
{
    if (this->currentFontName != font->getName()) {
        this->loadTextureArray(font->getTextures(), font->getCharWidth(), font->getCharHeight());
        this->currentFontName = font->getName();
    }
}

void OsdRenderer::drawCharacter(int layer, float x, float y, int width, int height, int windowWidth, int windowHeight)
{
    const float w = width / static_cast<float>(windowWidth);
    const float h = height / static_cast<float>(windowHeight);

    glm::vec2 pos = pixelToWorldCoords(x, y, windowWidth, windowHeight);
    
    // 0,0 = Upper left
    pos.x = pos.x + w;
    pos.y = pos.y - h;

    float transform[16] = {
        w,      0.0f,   0.0f,   0.0f,   // normal width
        0.0f,   h,      0.0f,   0.0f,   // normal height
        0.0f,   0.0f,   1.0f,   0.0f,   // ---
        pos.x,  pos.y,  0.0f,   1.0f    // position
    };

    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);
    glUniform1i(layerLoc, layer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

glm::vec2 OsdRenderer::pixelToWorldCoords(int x, int y, int width, int heigth)
{
    return glm::vec2(
        ((x + 0.5f) / width) * 2.0f - 1.0f,
        1.0f - ((y + 0.5f) / heigth) * 2.0f
    );
}

void OsdRenderer::render(int rows, int cols)
{
    glUseProgram(this->shader);
    glBindVertexArray(this->VAO);
    
    int windowWidth, windowHeight;
    XPLMGetScreenSize(&windowWidth, &windowHeight);

    const float textureAspectRatio = static_cast<float>(this->textureWidth) / static_cast<float>(this ->textureHeight);

    int cellWidth = windowWidth / this->textureWidth ;
    int cellHeight = windowHeight / textureAspectRatio; 

    const int avaiableWidth = windowWidth - 2 * MARGIN;
    const int avaiableHeight = windowHeight - 2 * MARGIN;

    if (cellWidth * cols > avaiableWidth) {
        cellWidth = avaiableWidth / cols;
        cellHeight = cellWidth / textureAspectRatio;
    }

    if (cellHeight * rows > avaiableHeight) {
        cellHeight = avaiableHeight / rows;
        cellWidth = cellHeight * textureAspectRatio;
    }
          
    int xOffset = (windowWidth - cellWidth * cols) / 2.0f ;
    int yOffset = (windowHeight - cellHeight * rows) / 2.0f;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            
            int character = this->screen[y][x];
            if (character == 0x20 || character == 0x00) {
                continue;
            }
            
            float posX = static_cast<float>(x * cellWidth + xOffset);
            float posY = static_cast<float>(y * cellHeight + yOffset); 
            this->drawCharacter(character, posX, posY, cellWidth, cellHeight, windowWidth, windowHeight);         
        }
    }
    
    glBindVertexArray(0);
    glUseProgram(0); 
}
