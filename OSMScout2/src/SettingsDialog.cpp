/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2013  Tim Teulings

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

#include "SettingsDialog.h"

/*
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>*/

#include <osmscout/util/String.h>

#include <iostream>

SettingsDialog::SettingsDialog(const SettingsRef& settings)
 : settings(settings)/*,
   dpi(new QSpinBox())*/
{
  std::cout << "SettingsDialog::SettingsDialog()" << std::endl;

  setSource(QUrl::fromLocalFile("settings.qml"));

  QQuickItem* root=rootObject();

  std::cout << "Root: " << root << std::endl;

  /*
  dpi->setRange(1,400);
  dpi->setValue(settings->GetDPI());
  dpi->setSuffix(" DPI");

  QVBoxLayout *mainLayout=new QVBoxLayout();
  QFormLayout *formLayout=new QFormLayout();

  formLayout->addRow("DPI:",dpi);

  mainLayout->addLayout(formLayout);

  QDialogButtonBox *buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Cancel);

  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));

  connect(this,SIGNAL(accepted()),this,SLOT(Save()));*/
}

SettingsDialog::~SettingsDialog()
{
  // no code
  std::cout << "SettingsDialog::~SettingsDialog()" << std::endl;
}

void SettingsDialog::Save()
{
  std::cout << "Save()" << std::endl;

  /*
  if (dpi->hasAcceptableInput()) {
    std::cout << "DPI: " << dpi->value() << std::endl;

    settings->SetDPI((size_t)dpi->value());
  }*/
}
