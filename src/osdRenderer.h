#pragma once

#include "platform.h"

#include "fontBase.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class OsdRenderer {
    private:
        std::vector<std::vector<uint16_t>> screen;
        GLuint compileShader(GLenum type, const char* source);
        bool createShader();
        void intQuad();
        void loadTextureArray(std::vector<std::vector<uint8_t>> textures,  int width, int height);
        void drawCharacter(int layer, float x, float y, int width, int height, int windowWidth, int windowHeight);
        
        uint textureWidth = 0;
        uint textureHeight = 0;
        std::string currentFontName = "";
        GLuint shader;
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;
        GLuint textureArray;
        GLint transformLoc;
        GLint layerLoc;
        
        static glm::vec2 pixelToWorldCoords(int x, int y, int width, int heigth);

    public:
        OsdRenderer();
        ~OsdRenderer();

        void clearScreen();
        void setCharacter(int row, int col, uint16_t character);
        void LoadFont(std::shared_ptr<FontBase> font);
        void render(int rows, int cols);
};