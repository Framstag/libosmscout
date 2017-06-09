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

namespace osmscout
{
  class OpenGLMapData
  {
  private:
    std::vector<GLfloat> Vertices;
    std::vector<GLuint> Elements;

    GLuint shaderProgram;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    std::vector<GLuint> Attributes;
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

    std::string LoadShader(std::string name)
    {
      std::string result;
      std::string line;
      std::string filePath = std::string(__FILE__);

      #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        std::string dirPath = filePath.substr(0, filePath.rfind("\\"));
      #else
        std::string dirPath = filePath.substr(0, filePath.rfind("/"));
      #endif

      std::ifstream myfile (dirPath + "/private/" + name);
      if (myfile.is_open())
      {
        while ( getline (myfile,line) )
        {
          result.append(line + "\n");
        }
        myfile.close();
      }

      return result;

    }

    /**
     *
     */
    void LoadVBO()
    {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), &Vertices[0], GL_STATIC_DRAW);
    }

    /**
     *
     */
    void LoadEBO()
    {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Elements), &Elements[0],  GL_STATIC_DRAW);
    }

  public:

    /**
     * Initialize OpenGL context by creating the buffers, shaders, and program.
     */
    bool InitContext()
    {
      glGenVertexArrays(1, &VAO);
      glBindVertexArray(VAO);
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &EBO);

      VertexShader = glCreateShader(GL_VERTEX_SHADER);
      const char* VertexSourceC = VertexShaderSource.c_str();
      glShaderSource(VertexShader, 1, &VertexSourceC, &VertexShaderLength);
      glCompileShader(VertexShader);

      GLint isCompiled = 0;
      glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &isCompiled);
      if(isCompiled == GL_FALSE)
      {
        GLint maxLength = 0;
        glGetShaderiv(VertexShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(VertexShader, maxLength, &maxLength, &errorLog[0]);

        for(int i = 0; i < errorLog.size(); i++)
          std::cout << errorLog.at(i);

        std::cout << std::endl;

        return false;
      }

      FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
      const char* FragmentSourceC = FragmentShaderSource.c_str();
      glShaderSource(FragmentShader, 1, &FragmentSourceC, &FragmentShaderLength);
      glCompileShader(FragmentShader);

      isCompiled = 0;
      glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &isCompiled);
      if(isCompiled == GL_FALSE)
      {
        GLint maxLength = 0;
        glGetShaderiv(FragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(FragmentShader, maxLength, &maxLength, &errorLog[0]);

        for(int i = 0; i < errorLog.size(); i++)
          std::cout << errorLog.at(i);

        std::cout << std::endl;

        return false;
      }

      return true;

    }

    /**
     *
     * @param fileName
     */
    void LoadFragmentShader(std::string fileName)
    {
      FragmentShaderSource = this->LoadShader(fileName);
      FragmentShaderLength = FragmentShaderSource.length();
    }

    /**
     *
     * @param fileName
     */
    void LoadVertexShader(std::string fileName)
    {
      VertexShaderSource = this->LoadShader(fileName);
      VertexShaderLength = VertexShaderSource.length();
    }


    /**
     *
     * @param Vertices
     */
    void LoadVertices(std::vector<GLfloat> Vertices)
    {
      this->Vertices = Vertices;
      LoadVBO();
    }

    /**
     *
     * @param Elements
     */
    void LoadElements(std::vector<GLuint> Elements)
    {
      this->Elements = Elements;
      LoadEBO();
    }

    /**
     *
     */
    void LoadProgram()
    {
      shaderProgram = glCreateProgram();
      glAttachShader(shaderProgram, VertexShader);
      glAttachShader(shaderProgram, FragmentShader);
      glBindFragDataLocation(shaderProgram, 0, "outColor");
      glLinkProgram(shaderProgram);
      glUseProgram(shaderProgram);
    }

    /**
     *
     */
    void setModel()
    {
      //TODO
    }

    /**
     *
     */
    void setView()
    {
      //TODO
    }

    /**
     *
     */
    void setProjection()
    {
      //TODO
    }

    /**
     *
     * @param attribName
     * @param length
     * @param type
     * @param what
     * @param startPosition
     * @param positionOffset
     */
    void AddAttrib(std::string attribName, size_t length, GLuint type, size_t count, size_t positionOffset)
    {
      GLuint attrib = glGetAttribLocation(shaderProgram, attribName.c_str());
      glEnableVertexAttribArray(attrib);
      glVertexAttribPointer(attrib, length, type, GL_FALSE, count, (void*) positionOffset);
      this->Attributes.push_back(attrib);
    }

    /**
     *
     * @param shaderProgram
     * @param uniformName
     * @param value
     */
    void AddUniform(GLuint shaderProgram, std::string uniformName, float value)
    {
      GLuint uniform = glGetUniformLocation(shaderProgram, uniformName.c_str());
      glUniform1f(uniform, value);
      this->Uniforms.push_back(uniform);
    }

    /**
     *
     * @param shaderProgram
     * @param uniformName
     * @param value
     */
    void AddUniform(GLuint shaderProgram, std::string uniformName, int value)
    {
      GLuint uniform = glGetUniformLocation(shaderProgram, uniformName.c_str());
      glUniform1i(uniform, value);
      this->Uniforms.push_back(uniform);
    }

    /**
     *
     */
    void Draw()
    {
      glUseProgram(shaderProgram);
      glBindVertexArray(VAO);
      glDrawElements(GL_TRIANGLES, (GLsizei) Elements.size(), GL_UNSIGNED_INT, 0);
    }

    ~OpenGLMapData()
    {
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
