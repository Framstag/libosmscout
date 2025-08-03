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

#include <osmscoutmapopengl/ShaderUtils.h>
#include <osmscoutmapopengl/OpenGLProjection.h>

#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/MapPainter.h>

#include <osmscout/io/File.h>

#include <iostream>
#include <fstream>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

namespace osmscout {

  class OpenGLTexture {
  public:
    size_t width=0;
    size_t height=0;
    size_t size=0;
    unsigned char *data=nullptr; // owned, have to be allocated by new []

    size_t fromOriginY=0;

    OpenGLTexture() = default;

    OpenGLTexture(const OpenGLTexture&) = delete;
    OpenGLTexture(OpenGLTexture&&) = delete;
    OpenGLTexture &operator=(const OpenGLTexture&) = delete;
    OpenGLTexture &operator=(OpenGLTexture&&) = delete;

    ~OpenGLTexture(){
      if (data!=nullptr) {
        delete[] data;
        data=nullptr;
      }
    }

  };

  typedef std::shared_ptr<OpenGLTexture> OpenGLTextureRef;

  /**
   *
   * @tparam TexturePixelType GL_RGBA, GL_RED...
   * @tparam TexturePixelSize pixel byte size (4 for GL_RGBA, 1 for GL_RED)
   */
  template <int TexturePixelType, unsigned int TexturePixelSize>
  class OpenGLMapData {
  private:

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> verticesBuffer;
    std::vector<GLuint> elements;
    std::vector<GLuint> elementsBuffer;
    unsigned char *textures=nullptr;
    std::vector<OpenGLTextureRef> texturesBuffer;
    int textureSize=0;
    int textureSizeBuffer=0;
    int textureWidth=0;
    int textureWidthBuffer=0;
    int textureHeight=14;

    GLuint shaderProgram=0;
    GLuint vao=0;
    GLuint vbo=0;
    GLuint ebo=0;
    GLuint tex=0;

    int verticesSize;
    float zoom;

    GLuint vertexShader=0;
    GLuint fragmentShader=0;

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    void LoadVBO() {
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
    }

    void LoadEBO() {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLfloat) * elements.size(), elements.data(), GL_DYNAMIC_DRAW);
    }

  public:
    OpenGLMapData() = default;

    ~OpenGLMapData()
    {
      clearData();

      if (shaderProgram!=0) {
        glDeleteProgram(shaderProgram);
      }
      if (fragmentShader!=0) {
        glDeleteShader(fragmentShader);
      }
      if (vertexShader!=0) {
        glDeleteShader(vertexShader);
      }

      if (ebo!=0) {
        glDeleteBuffers(1, &ebo);
      }
      if (vbo!=0) {
        glDeleteBuffers(1, &vbo);
      }

      if (vao!=0) {
        glDeleteVertexArrays(1, &vao);
      }
    }

    OpenGLMapData(const OpenGLMapData&) = delete;
    OpenGLMapData(OpenGLMapData&&) = delete;
    OpenGLMapData &operator=(const OpenGLMapData&) = delete;
    OpenGLMapData &operator=(OpenGLMapData&&) = delete;

    void SwapData() {
      delete[] textures;
      vertices = std::move(verticesBuffer);
      verticesBuffer.clear();
      elements = std::move(elementsBuffer);
      elementsBuffer.clear();

      textureSize = textureSizeBuffer;
      textureSizeBuffer = 0;
      textureWidth = textureWidthBuffer;
      textureWidthBuffer = 0;

      textures = new unsigned char[textureWidth * textureHeight * TexturePixelSize];

      int index = 0;
      for (int i = 0; i < textureHeight; i++) {
        for (unsigned int j = 0; j < texturesBuffer.size(); j++) {
          int start = i * texturesBuffer[j]->width * TexturePixelSize;
          for (unsigned int k = start; k < start + (texturesBuffer[j]->width * TexturePixelSize); k++) {
            textures[index] = (texturesBuffer[j]->data[k]);
            index++;
          }
        }
      }

      texturesBuffer.clear();
    }

    void clearData() {
      vertices.clear();
      elements.clear();
      if (textures != nullptr) {
        delete[] textures;
        textures = nullptr;
      }
      texturesBuffer.clear();
      textureSize = 0;
      textureWidth = 0;
    }

    void BindBuffers() {
      glBindVertexArray(vao);
    }

    bool InitContext(const std::string &shaderDir,
                     const std::string &vertexShaderFileName,
                     const std::string &fragmentShaderFileName,
                     GLuint projectionShader) {
      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ebo);
      glGenTextures(1, &tex);

      zoom = 45.0f;
      model = glm::mat4(1.0f);
      textureSize = 0;
      textureSizeBuffer = 0;
      textureWidth = 0;
      textureWidthBuffer = 0;
      textureHeight = 0;

      if (std::string vertexShaderSource;
          !(LoadShaderSource(shaderDir, vertexShaderFileName, vertexShaderSource) &&
            LoadShader(vertexShader, GL_VERTEX_SHADER, "vertex", vertexShaderSource))) {
        return false;
      }

      if (std::string fragmentShaderSource;
          !(LoadShaderSource(shaderDir, fragmentShaderFileName, fragmentShaderSource) &&
            LoadShader(fragmentShader, GL_FRAGMENT_SHADER, "fragment", fragmentShaderSource))) {
        return false;
      }

      glEnable(GL_PROGRAM_POINT_SIZE);

      shaderProgram = glCreateProgram();
      glAttachShader(shaderProgram, vertexShader);
      glAttachShader(shaderProgram, fragmentShader);
      glAttachShader(shaderProgram, projectionShader);
      glBindFragDataLocation(shaderProgram, 0, "outColor");
      glLinkProgram(shaderProgram);

      GLint isLinked = 0;
      glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
      if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, errorLog.data());
        assert(!errorLog.empty() && errorLog.back() == 0);
        log.Error() << "Error while linking shader program: " << errorLog.data();

        return false;
      }

      // Always detach shaders after a successful link.
      glDetachShader(shaderProgram, vertexShader);
      glDetachShader(shaderProgram, fragmentShader);
      glDetachShader(shaderProgram, projectionShader);

      return true;
    }

    void LoadTextures() {
      glActiveTexture(GL_TEXTURE0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, TexturePixelSize);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, TexturePixelType, textureWidth, textureHeight, 0, TexturePixelType, GL_UNSIGNED_BYTE, textures);
    }

    void LoadVertices() {
      LoadVBO();
      LoadEBO();
    }

    void AddNewVertex(GLfloat vertex) {
      this->verticesBuffer.push_back(vertex);
    }

    void AddNewElement(GLuint element) {
      this->elementsBuffer.push_back(element);
    }

    int GetVerticesNumber() {
      return (this->verticesBuffer.size()) / (this->verticesSize);
    }

    void UseProgram() {
      glUseProgram(shaderProgram);
    }

    void SetVerticesSize(int size) {
      this->verticesSize = size;
    }

    int GetNumOfVertices() {
      return this->verticesBuffer.size();
    }

    /*void AddNewTexture(OpenGLTexture *texture) {
      TexturesBuffer.push_back(texture);

      textureWidthBuffer += texture->width;

      textureSizeBuffer++;
    }*/

    void AddNewTexture(OpenGLTextureRef texture) {
      texturesBuffer.push_back(texture);

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
      return texturesBuffer[index]->width;
    }

    int GetTextureWidthSum(int index){
      int sum = 0;
      for(int i = 0; i < index+1; i++)
        sum += texturesBuffer[i]->width;

      return sum;
    }

    void SetTextureHeight(int textheight){
      textureHeight = textheight;
    }

    void SetModel() {
      GLuint uniform = glGetUniformLocation(shaderProgram, "Model");
      glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(model));
    }

    void SetView(float /*lookX*/, float /*lookY*/) {
      view = glm::lookAt(
          glm::vec3(0.0, 0.0, 1.0f), //position
          glm::vec3(0.0f, 0.0f, 0.0f), //look
          glm::vec3(0.0f, 1.0f, 0.0f) //up
      );
      GLint uniView = glGetUniformLocation(shaderProgram, "View");
      glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
    }

    void SetProjection(float width, float height) {
      projection = glm::perspective(glm::radians(60.0f), (float) width / (float) height, 0.1f, 10.0f);
      GLint uniProj = glGetUniformLocation(shaderProgram, "Projection");
      glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(projection));
    }

    void AddAttrib(std::string attribName, GLint length, GLuint type, size_t positionOffset) {
      GLint attrib = glGetAttribLocation(shaderProgram, attribName.c_str());
      glEnableVertexAttribArray(attrib);
      glVertexAttribPointer(attrib, length, type, GL_FALSE, verticesSize * sizeof(GLfloat), (void *) positionOffset);
    }

    void AddUniform(const char *uniformName, float value) {
      GLuint uniform = glGetUniformLocation(shaderProgram, uniformName);
      glUniform1f(uniform, value);
    }

    void SetMapProjection(const OpenGLProjection &mapProjection)
    {
      mapProjection.SetShaderUniforms(shaderProgram);
    }

    GLuint getVAO() {
      return this->vao;
    }

    GLuint GetTexture() {
      return this->tex;
    }

    GLuint getShaderProgram() {
      return this->shaderProgram;
    }

    const glm::mat4 &GetModel() const {
      return model;
    }

    const glm::mat4 &GetView() const {
      return view;
    }

    const glm::mat4 &GetProjection() const {
      return projection;
    }

    void Draw() {
      glDrawElements(GL_TRIANGLES, (GLsizei) elements.size(), GL_UNSIGNED_INT, 0);
    }

  };
}

#endif //LIBOSMSCOUT_OPENGLMAPDATA_H
