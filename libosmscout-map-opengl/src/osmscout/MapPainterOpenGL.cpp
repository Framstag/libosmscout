/*
  This source is part of the libosmscout-map library
  Copyright (C) 2013  Tim Teulings
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
#include <osmscout/MapPainterOpenGL.h>
#include <iostream>

namespace osmscout {

  osmscout::MapPainterOpenGL::MapPainterOpenGL()
  {
    glewExperimental = GL_TRUE;
    glewInit();
  }

  osmscout::MapPainterOpenGL::~MapPainterOpenGL()
  {

  }

  void osmscout::MapPainterOpenGL::loadData(const osmscout::MapData &data, const osmscout::MapParameter &parameter, const osmscout::Projection &projection, const osmscout::StyleConfigRef &styleConfig)
  {
    styleConfig.get()->GetLandFillStyle(projection,landFill);
    processAreaData(data);
  }

  void osmscout::MapPainterOpenGL::processAreaData(const osmscout::MapData &data)
  {
    AreaRenderer.LoadVertexShader("AreaVertexShader.vert");
    AreaRenderer.LoadFragmentShader("AreaFragmentShader.frag");
    bool success = AreaRenderer.InitContext();
    if(!success)
    {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }
    //AreaRenderer.loadProgram();
    //AreaRenderer.AddAttrib("position",2, GL_FLOAT, 2 * sizeof(GLfloat), 0);

    PathRenderer.LoadVertexShader("PathVertexShader.vert");
    PathRenderer.LoadFragmentShader("PathFragmentShader.frag");
    success = PathRenderer.InitContext();
    if(!success)
    {
      std::cerr << "Could not initialize context for path rendering!" << std::endl;
      return;
    }

  }

  void osmscout::MapPainterOpenGL::DrawMap()
  {
    glClearColor(this->landFill.get()->GetFillColor().GetR(),
                 this->landFill.get()->GetFillColor().GetB(),
                 this->landFill.get()->GetFillColor().GetG(),
                 this->landFill.get()->GetFillColor().GetA());
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    AreaRenderer.Draw();
    //PathRenderer.Draw();
  }

}
