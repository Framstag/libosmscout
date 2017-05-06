/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/StyleDescription.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  Style::~Style()
  {
    // no code
  }

  void Style::SetBoolValue(int /*attribute*/,
                           bool /*value*/)
  {
    assert(false);
  }

  void Style::SetStringValue(int /*attribute*/,
                             const std::string& /*value*/)
  {
    assert(false);
  }

  void Style::SetColorValue(int /*attribute*/,
                            const Color& /*value*/)
  {
    assert(false);
  }

  void Style::SetMagnificationValue(int /*attribute*/,
                                    const Magnification& /*value*/)
  {
    assert(false);
  }

  void Style::SetDoubleValue(int /*attribute*/,
                             double /*value*/)
  {
    assert(false);
  }

  void Style::SetDoubleArrayValue(int /*attribute*/,
                                  const std::vector<double>& /*value*/)
  {
    assert(false);
  }

  void Style::SetSymbolValue(int /*attribute*/,
                             const SymbolRef& /*value*/)
  {
    assert(false);
  }

  void Style::SetIntValue(int /*attribute*/,
                          int /*value*/)
  {
    assert(false);
  }

  void Style::SetUIntValue(int /*attribute*/,
                           size_t /*value*/)
  {
    assert(false);
  }

  void Style::SetLabelValue(int /*attribute*/,
                            const LabelProviderRef& /*value*/)
  {
    assert(false);
  }

  StyleAttributeDescriptor::StyleAttributeDescriptor(StyleAttributeType type,
                                                     const std::string& name,
                                                     int attribute)
  : type(type),
    name(name),
    attribute(attribute)
  {
    // no code
  }

  StyleAttributeDescriptor::~StyleAttributeDescriptor()
  {
    // no code
  }

  void StyleDescriptor::AddAttribute(const StyleAttributeDescriptorRef& attribute)
  {
    attributeMap[attribute->GetName()]=attribute;
  }
}

