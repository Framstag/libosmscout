#ifndef OSMSCOUT_MAP_LABELLAYOUTER_H
#define OSMSCOUT_MAP_LABELLAYOUTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Tim Teulings

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

#include <memory>
#include <set>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/StyleConfig.h>

namespace osmscout {

  class OSMSCOUT_MAP_API LabelData
  {
  public:
    size_t                   id;       //!< Id of this label, multiple labels with the same id do not intersect with each other
    size_t                   priority; //!< Priority of the entry
    double                   bx1;      //!< Dimensions of bounding box
    double                   by1;      //!< Dimensions of bounding box
    double                   bx2;      //!< Dimensions of bounding box
    double                   by2;      //!< Dimensions of bounding box

    double                   x;        //!< Coordinate of the left, top edge of the text
    double                   y;        //!< Coordinate of the left, top edge of the text
    double                   alpha;    //!< Alpha value of the label
    double                   fontSize; //!< Font size to be used
    LabelStyleRef            style;    //!< Style for drawing
    std::string              text;     //!< The label text

  public:
    LabelData();
    virtual ~LabelData();
  };

  typedef std::list<LabelData>::iterator LabelDataRef;

  struct OSMSCOUT_MAP_API LabelEvent
  {
    double       y;
    double       x;
    LabelDataRef label;

    inline bool operator<(const LabelEvent& other) const
    {
      if (y!=other.y) {
        return y<other.y;
      }

      if (x!=other.x) {
        return x<other.x;
      }

      return label->id<other.label->id;
    }
  };

  class OSMSCOUT_MAP_API LabelLayouter
  {
  private:


  private:
    std::list<LabelData> labels;
    std::set<LabelEvent> events;
    double               width;
    double               height;
    double               labelSpace;
    double               shieldLabelSpace;
    double               sameLabelSpace;
    double               maxSpace;
    bool                 dropNotVisiblePointLabels;
    size_t               labelsAdded;

  private:
    void DeleteEventsForLabel(const std::set<LabelEvent>::iterator& eventRef);
    bool Intersects(const LabelData& first, const LabelData& second) const;

  public:
    LabelLayouter();
    virtual ~LabelLayouter();

    void Initialize(const Projection& projection,
                    const MapParameter& parameter);

    bool Placelabel(const LabelData& label,
                    LabelDataRef& labelRef);

    inline std::list<LabelData>::const_iterator begin() const
    {
      return labels.begin();
    }

    inline std::list<LabelData>::const_iterator end() const
    {
      return labels.end();
    }

    inline size_t Size() const
    {
      return labels.size();
    }

    inline size_t GetLabelsAdded() const
    {
      return labelsAdded;
    }
  };
}

#endif
