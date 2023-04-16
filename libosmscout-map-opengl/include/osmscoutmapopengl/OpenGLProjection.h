#ifndef LIBOSMSCOUT_OPENGLPROJECTION_H
#define LIBOSMSCOUT_OPENGLPROJECTION_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2022  Lukas Karas

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

#include <osmscoutmapopengl/MapOpenGLImportExport.h>

#include <osmscout/projection/MercatorProjection.h>

#include <cassert>

#include <GL/glew.h>

namespace osmscout {

  class OSMSCOUT_MAP_OPENGL_API OpenGLProjection : public MercatorProjection {
    // TODO: add support for inclination
  public:
    OpenGLProjection() = default;
    OpenGLProjection(const OpenGLProjection&) = default;
    OpenGLProjection(OpenGLProjection&&) = default;
    OpenGLProjection& operator=(const OpenGLProjection&) = default;
    OpenGLProjection& operator=(OpenGLProjection&&) = default;
    ~OpenGLProjection() override = default;

    /**
     * Setup projection shader uniforms
     * @param shaderProgram
     */
    void SetShaderUniforms(GLuint shaderProgram) const
    {
      assert(valid);
      assert(width>0);
      assert(height>0);

      auto SetGLf = [shaderProgram](const std::string_view& uniformName, GLfloat f) {
        glUniform1f(glGetUniformLocation(shaderProgram, uniformName.data()), f);
      };
      auto SetGLui = [shaderProgram](const std::string_view& uniformName, GLuint i) {
        glUniform1ui(glGetUniformLocation(shaderProgram, uniformName.data()), i);
      };

      SetGLf("centerLat", center.GetLat());
      SetGLf("centerLon", center.GetLon());
      SetGLf("scaleGradtorad", scaleGradtorad);
      SetGLui("useLinearInterpolation", useLinearInterpolation ? 1 : 0);
      SetGLf("scaledLatDeriv", scaledLatDeriv);
      SetGLf("latOffset", latOffset);
      SetGLf("scale", scale);
      SetGLf("angle", angle);
      SetGLf("angleNegSin", angleNegSin);
      SetGLf("angleNegCos", angleNegCos);
      SetGLf("windowHeight", height);
      SetGLf("windowWidth", width);
    }
  };
}

#endif //LIBOSMSCOUT_OPENGLPROJECTION_H
