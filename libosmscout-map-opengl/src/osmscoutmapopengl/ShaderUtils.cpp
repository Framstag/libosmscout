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
#include <osmscout/io/File.h>
#include <osmscout/log/Logger.h>

#include <string>
#include <fstream>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

namespace osmscout {

bool LoadShaderSource(const std::string &dirPath, const std::string &name, std::string &result) {
  std::string filePath = dirPath + "/" + name;
  if (!ExistsInFilesystem(filePath)) {
    log.Error() << "Shader file " << filePath << " doesn't exists";
    return false;
  }

  std::string line;
  std::ifstream myfile(filePath);
  if (!myfile.is_open()) {
    return false;
  }

  while (getline(myfile, line)) {
    result.append(line + "\n");
  }
  myfile.close();

  return true;
}

bool LoadShader(GLuint &shader,
                GLenum type,
                const std::string &name,
                const std::string &shaderSource)
{
  static_assert(std::is_same<GLchar, char>::value, "GLchar must be char for usage with logger");

  shader = glCreateShader(type);
  const char *sourceC = shaderSource.c_str();
  int shaderLength = shaderSource.length();
  glShaderSource(shader, 1, &sourceC, &shaderLength);
  glCompileShader(shader);

  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE) {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog.data());
    assert(!errorLog.empty() && errorLog.back() == 0);
    log.Error() << "Error while loading " << name << " shader: " << errorLog.data();

    return false;
  }
  return true;
}

}
