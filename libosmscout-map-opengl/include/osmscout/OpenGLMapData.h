#ifndef LIBOSMSCOUT_OPENGLMAPDATA_H
#define LIBOSMSCOUT_OPENGLMAPDATA_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017  Fanny Monori

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/


#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <osmscout/MapParameter.h>
#include <osmscout/MapPainter.h>
#include <iostream>
#include <fstream>

namespace osmscout {

  class OpenGLTexture {
  public:
    size_t width;
    size_t height;
    size_t size;
    unsigned char *data;

    size_t fromOriginY;

  };

  class OpenGLMapData {
  private:

    std::vector<GLfloat> Vertices;
    std::vector<GLfloat> VerticesBuffer;
    std::vector<GLuint> Elements;
    std::vector<GLuint> ElementsBuffer;
    unsigned char *Textures;
    std::vector<OpenGLTexture*> TexturesBuffer;
    int textureSize;
    int textureSizeBuffer;
    int textureWidth;
    int textureWidthBuffer;
    int textureHeight;

    GLuint shaderProgram;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLuint Tex;

    int VerticesSize;
    float zoom;

    GLuint VertexShader;
    GLuint FragmentShader;

    std::string VertexShaderSource;
    std::string FragmentShaderSource;
    int VertexShaderLength;
    int FragmentShaderLength;

    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Projection;

    std::string LoadShader(std::string name) {
      std::string result;
      std::string line;
      std::string filePath = std::string(__FILE__);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
      std::string dirPath = filePath.substr(0, filePath.rfind("\\"));
#else
      std::string dirPath = filePath.substr(0, filePath.rfind("/"));
#endif

      std::ifstream myfile(dirPath + "/private/" + name);
      if (myfile.is_open()) {
        while (getline(myfile, line)) {
          result.append(line + "\n");
        }
        myfile.close();
      }

      return result;

    }

    void LoadVBO() {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(std::vector<GLfloat>) + (sizeof(GLfloat) * Vertices.size()), &Vertices[0],
                   GL_DYNAMIC_DRAW);
    }

    void LoadEBO() {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(std::vector<GLfloat>) + (sizeof(GLfloat) * Elements.size()),
                   &Elements[0], GL_DYNAMIC_DRAW);
    }

  public:

    void SwapData() {
      this->Vertices.clear();
      this->Vertices = this->VerticesBuffer;
      this->VerticesBuffer.clear();
      this->Elements.clear();
      this->Elements = this->ElementsBuffer;
      this->ElementsBuffer.clear();

      textureSize = textureSizeBuffer;
      textureSizeBuffer = 0;
      textureWidth = textureWidthBuffer;
      textureWidthBuffer = 0;

      textureHeight = 14;

      this->Textures = new unsigned char[textureWidth*textureHeight*4];

      int index = 0;
      for (int i = 0; i < textureHeight; i++) {
        for (unsigned int j = 0; j < TexturesBuffer.size(); j++) {
          int start = i * TexturesBuffer[j]->width * 4;
          for (unsigned int k = start; k < start + (TexturesBuffer[j]->width * 4); k++) {
            Textures[index] = (TexturesBuffer[j]->data[k]);
            index++;
          }
        }
      }

      TexturesBuffer.clear();
    }

    void SwapData(int stride) {
      this->Vertices.clear();
      this->Vertices = this->VerticesBuffer;
      this->VerticesBuffer.clear();
      this->Elements.clear();
      this->Elements = this->ElementsBuffer;
      this->ElementsBuffer.clear();

      textureSize = textureSizeBuffer;
      textureSizeBuffer = 0;
      textureWidth = textureWidthBuffer;
      textureWidthBuffer = 0;

      this->Textures = new unsigned char[textureWidth*textureHeight*stride];

      int index = 0;
      for (int i = 0; i < textureHeight; i++) {
        for (unsigned int j = 0; j < TexturesBuffer.size(); j++) {
          int start = i * TexturesBuffer[j]->width * stride;
          for (unsigned int k = start; k < start + (TexturesBuffer[j]->width * stride); k++) {
            Textures[index] = (TexturesBuffer[j]->data[k]);
            index++;
          }
        }
      }

      TexturesBuffer.clear();
    }

    void clearData() {
      Vertices.clear();
      Elements.clear();
      Textures = NULL;
      TexturesBuffer.clear();
      textureSize = 0;
      textureWidth = 0;
    }

    void BindBuffers() {
      glBindVertexArray(VAO);
    }

    bool InitContext() {
      glGenVertexArrays(1, &VAO);
      glBindVertexArray(VAO);
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &EBO);
      glGenTextures(1, &Tex);

      zoom = 45.0f;
      Model = glm::mat4(1.0f);
      textureSize = 0;
      textureSizeBuffer = 0;
      textureWidth = 0;
      textureWidthBuffer = 0;
      textureHeight = 0;

      VertexShader = glCreateShader(GL_VERTEX_SHADER);
      const char *VertexSourceC = VertexShaderSource.c_str();
      glShaderSource(VertexShader, 1, &VertexSourceC, &VertexShaderLength);
      glCompileShader(VertexShader);

      GLint isCompiled = 0;
      glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &isCompiled);
      if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(VertexShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(VertexShader, maxLength, &maxLength, &errorLog[0]);

        for (glm::uint i = 0; i < errorLog.size(); i++)
          std::cout << errorLog.at(i);

        std::cout << std::endl;

        return false;
      }

      FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
      const char *FragmentSourceC = FragmentShaderSource.c_str();
      glShaderSource(FragmentShader, 1, &FragmentSourceC, &FragmentShaderLength);
      glCompileShader(FragmentShader);

      isCompiled = 0;
      glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &isCompiled);
      if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(FragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(FragmentShader, maxLength, &maxLength, &errorLog[0]);

        for (glm::uint i = 0; i < errorLog.size(); i++)
          std::cout << errorLog.at(i);

        std::cout << std::endl;

        return false;
      }

      glEnable(GL_PROGRAM_POINT_SIZE);

      return true;

    }

    void LoadTextures() {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, Tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Textures);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    void LoadGreyTextures() {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, Tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, Textures);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    void LoadFragmentShader(std::string fileName) {
      FragmentShaderSource = this->LoadShader(fileName);
      FragmentShaderLength = FragmentShaderSource.length();
    }

    void LoadVertexShader(std::string fileName) {
      VertexShaderSource = this->LoadShader(fileName);
      VertexShaderLength = VertexShaderSource.length();
    }

    void LoadVertices() {
      LoadVBO();
      LoadEBO();
    }

    void AddNewVertex(GLfloat vertex) {
      this->VerticesBuffer.push_back(vertex);
    }

    void AddNewElement(GLuint element) {
      this->ElementsBuffer.push_back(element);
    }

    int GetVerticesNumber() {
      return (this->VerticesBuffer.size()) / (this->VerticesSize);
    }

    void LoadProgram() {
      shaderProgram = glCreateProgram();
      glAttachShader(shaderProgram, VertexShader);
      glAttachShader(shaderProgram, FragmentShader);
      glBindFragDataLocation(shaderProgram, 0, "outColor");
      glLinkProgram(shaderProgram);
      glUseProgram(shaderProgram);
    }

    void SetVerticesSize(int size) {
      this->VerticesSize = size;
    }

    int GetNumOfVertices() {
      return this->VerticesBuffer.size();
    }

    void AddNewTexture(OpenGLTexture *texture) {
      TexturesBuffer.push_back(texture);

      textureWidthBuffer += texture->width;

      textureSizeBuffer++;
    }

    int GetTextureWidth(){
      return textureWidth;
    }

    int GetTextureHeight(){
      return textureHeight;
    }

    int GetTextureWidth(int index){
      return TexturesBuffer[index]->width;
    }

    int GetTextureWidthSum(int index){
      int sum = 0;
      for(int i = 0; i < index+1; i++)
        sum += TexturesBuffer[i]->width;

      return sum;
    }

    void SetTextureHeight(int textheight){
      textureHeight = textheight;
    }

    void SetModel() {
      GLuint uniform = glGetUniformLocation(shaderProgram, "Model");
      glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(Model));
    }

    void SetView(float /*lookX*/, float /*lookY*/) {
      View = glm::lookAt(
          glm::vec3(0.0, 0.0, 1.0f), //position
          glm::vec3(0.0f, 0.0f, 0.0f), //look
          glm::vec3(0.0f, 1.0f, 0.0f) //up
      );
      GLint uniView = glGetUniformLocation(shaderProgram, "View");
      glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(View));
    }

    void SetProjection(float width, float height) {
      Projection = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.1f, 10.0f);
      GLint uniProj = glGetUniformLocation(shaderProgram, "Projection");
      glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(Projection));
    }

    void AddAttrib(std::string attribName, GLint length, GLuint type, size_t positionOffset) {
      GLint attrib = glGetAttribLocation(shaderProgram, attribName.c_str());
      glEnableVertexAttribArray(attrib);
      glVertexAttribPointer(attrib, length, type, GL_FALSE, VerticesSize * sizeof(GLfloat), (void *) positionOffset);
    }

    void AddUniform(std::string uniformName, float value) {
      GLuint uniform = glGetUniformLocation(shaderProgram, uniformName.c_str());
      glUniform1f(uniform, value);
    }

    GLuint getVAO() {
      return this->VAO;
    }

    GLuint GetTexture() {
      return this->Tex;
    }

    GLuint getShaderProgram() {
      return this->shaderProgram;
    }

    const glm::mat4 &GetModel() const {
      return Model;
    }

    const glm::mat4 &GetView() const {
      return View;
    }

    const glm::mat4 &GetProjection() const {
      return Projection;
    }

    void Draw() {
      glDrawElements(GL_TRIANGLES, (GLsizei) Elements.size(), GL_UNSIGNED_INT, 0);
    }

    ~OpenGLMapData() {
      glDeleteProgram(shaderProgram);
      glDeleteShader(FragmentShader);
      glDeleteShader(VertexShader);

      glDeleteBuffers(1, &EBO);
      glDeleteBuffers(1, &VBO);

      glDeleteVertexArrays(1, &VAO);
    }

  };
}

#endif //LIBOSMSCOUT_OPENGLMAPDATA_H