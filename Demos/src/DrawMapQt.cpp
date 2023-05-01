/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <DrawMap.h>

#include <iostream>

#include <QApplication>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>

#include <osmscout/Database.h>
#include <osmscoutmap/MapService.h>

#include <osmscout/feature/ConstructionYearFeature.h>

#include <osmscoutmapqt/MapPainterQt.h>

/*
  Example for the nordrhein-westfalen.osm.pbf:

  Demos/DrawMapQt ../maps/Dortmund ../stylesheets/standard.oss 1000 800 51.514 7.465 40000 test.png
*/

static const double MIN_YEAR=1800;
static const double MAX_YEAR=2020;

class ConstructionProcessor : public osmscout::FillStyleProcessor
{
private:
  osmscout::NameFeatureLabelReader             labelReader;
  osmscout::ConstructionYearFeatureValueReader reader;

public:
  explicit ConstructionProcessor(const osmscout::TypeConfig& typeConfig)
  : labelReader(typeConfig),
    reader(typeConfig)
  {
    // no code
  }

  osmscout::FillStyleRef Process(const osmscout::FeatureValueBuffer& features,
                                 const osmscout::FillStyleRef& fillStyle) const override
  {

    const osmscout::ConstructionYearFeatureValue* value=reader.GetValue(features);

    if (value!=nullptr)  {
      std::cout << labelReader.GetLabel(features) << ": " << value->GetStartYear() << "-" << value->GetEndYear()
                << std::endl;

      if (value->GetStartYear()>=MIN_YEAR && value->GetStartYear()<=MAX_YEAR) {
        double factor=(value->GetStartYear()-MIN_YEAR)/(MAX_YEAR-MIN_YEAR+1);
        auto   customStyle=std::make_shared<osmscout::FillStyle>();

        customStyle->SetFillColor(osmscout::Color(factor,0.0,0.0));

        return customStyle;
      }
    }

    return fillStyle;
  }
};

class AddressProcessor : public osmscout::FillStyleProcessor
{
private:
  osmscout::NameFeatureLabelReader    labelReader;
  osmscout::AddressFeatureValueReader reader;

public:
  explicit AddressProcessor(const osmscout::TypeConfig& typeConfig)
    : labelReader(typeConfig),
      reader(typeConfig)
  {
    // no code
  }

  osmscout::FillStyleRef Process(const osmscout::FeatureValueBuffer& features,
                                 const osmscout::FillStyleRef& fillStyle) const override
  {

    const osmscout::AddressFeatureValue* value=reader.GetValue(features);

    if (value!=nullptr)  {
      std::cout << labelReader.GetLabel(features) << ": " << value->GetAddress() << std::endl;

      size_t addressNumber;

      if (osmscout::StringToNumber(value->GetAddress(),addressNumber)) {
        auto customStyle=std::make_shared<osmscout::FillStyle>();

        if (addressNumber%2==0) {
          customStyle->SetFillColor(osmscout::Color::BLUE);
        }
        else {
          customStyle->SetFillColor(osmscout::Color::GREEN);
        }

        return customStyle;
      }
      else {
        auto customStyle=std::make_shared<osmscout::FillStyle>();

        customStyle->SetFillColor(osmscout::Color::RED);

        return customStyle;
      }
    }

    return fillStyle;
  }
};

int main(int argc, char* argv[])
{
  QApplication application(argc,argv,true);

  assert(QGuiApplication::primaryScreen());
  DrawMapDemo drawDemo("DrawMapQt", argc, argv,
                       QGuiApplication::primaryScreen()->physicalDotsPerInch());

  if (!drawDemo.OpenDatabase()){
    return 2;
  }

  Arguments args = drawDemo.GetArguments();

  auto *pixmap=new QPixmap(static_cast<int>(args.width),
                           static_cast<int>(args.height));

  auto* painter=new QPainter(pixmap);

  osmscout::MapPainterQt        mapPainter(drawDemo.styleConfig);

  osmscout::TypeInfoRef         buildingType=drawDemo.database->GetTypeConfig()->GetTypeInfo("building");


  if (buildingType!=nullptr) {
    /*
    osmscout::FillStyleProcessorRef constructionProcessor=std::make_shared<ConstructionProcessor>(*database->GetTypeConfig());
    drawParameter.RegisterFillStyleProcessor(buildingType->GetIndex(),
                                             constructionProcessor);*/

    /*
    osmscout::FillStyleProcessorRef addressProcessor=std::make_shared<AddressProcessor>(*database->GetTypeConfig());
    drawParameter.RegisterFillStyleProcessor(buildingType->GetIndex(),
                                          addressProcessor);*/
  }

  drawDemo.LoadData();

  if (mapPainter.DrawMap(drawDemo.projection,
                         drawDemo.drawParameter,
                         drawDemo.data,
                         painter)) {
    if (!pixmap->save(QString::fromStdString(args.output),"PNG",-1)) {
      std::cerr << "Cannot write PNG" << std::endl;
    }
  }

  delete painter;
  delete pixmap;

  return 0;
}
