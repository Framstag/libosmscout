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
#include <glm/gtc/type_ptr.hpp>
#include <osmscout/MapParameter.h>
#include <osmscout/MapPainter.h>
#include <iostream>
#include <fstream>

namespace osmscout {
  class OpenGLMapData {
  private:
    std::vector<GLfloat> Vertices;
    std::vector<GLuint> Elements;

    GLuint shaderProgram;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    int VerticesSize;
    float zoom;

    std::vector<GLint> Attributes;
    std::vector<GLuint> Uniforms;

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

    void clearData() {
      Vertices.clear();
      Elements.clear();
    }

    bool InitContext() {
      glGenVertexArrays(1, &VAO);
      glBindVertexArray(VAO);
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &EBO);

      zoom = 45.0f;

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

        for (int i = 0; i < errorLog.size(); i++)
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

        for (int i = 0; i < errorLog.size(); i++)
          std::cout << errorLog.at(i);

        std::cout << std::endl;

        return false;
      }

      return true;

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
      int i = 0;
      LoadVBO();
      LoadEBO();
    }

    void AddNewVertex(GLfloat vertex) {
      this->Vertices.push_back(vertex);
    }

    void AddNewElement(GLuint element) {
      this->Elements.push_back(element);
    }


    int GetVerticesNumber() {
      return (this->Vertices.size()) / (this->VerticesSize);
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

    int GetVerticesSize() {
      return this->VerticesSize;
    }

    int GetNumOfVertices() {
      return this->Vertices.size();
    }

    void SetModel() {
      Model = glm::mat4(1.0f);
      GLuint uniform = glGetUniformLocation(shaderProgram, "Model");
      glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(Model));
      this->Uniforms.push_back(uniform);
    }

    void SetView(float lookX, float lookY) {
      View = glm::lookAt(
          glm::vec3(lookX, lookY, 1.0f), //position
          glm::vec3(lookX, lookY, 0.0f), //look
          glm::vec3(0.0f, 1.0f, 0.0f) //up
      );
      GLint uniView = glGetUniformLocation(shaderProgram, "View");
      glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(View));
    }

    void SetProjection(float width, float height) {
      Projection = glm::perspective(glm::radians((float) zoom), (float) width / (float) height, 0.1f, 10.0f);
      // Projection = glm::ortho(0.0f, (float) width, 0.0f, (float) height, -1.0f, 100.0f);
      GLint uniProj = glGetUniformLocation(shaderProgram, "Projection");
      glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(Projection));
    }

    void SetZoom(float zoom) {
      this->zoom = zoom;
    }

    float GetZoom() {
      return this->zoom;
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

    void Draw() {
      glUseProgram(shaderProgram);
      glBindVertexArray(VAO);
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
