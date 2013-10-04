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

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <osmscout/util/String.h>

#include <iostream>

SettingsDialog::SettingsDialog(QWidget* parentWindow,
                               const SettingsRef& settings)
 : QDialog(parentWindow,Qt::Dialog),
   settings(settings),
   dpi(new QSpinBox())
{

  std::cout << "DPI: " << settings->GetDPI() << std::endl;

  dpi->setRange(1,400);
  dpi->setValue(settings->GetDPI());
  dpi->setSuffix(" DPI");

  QVBoxLayout *mainLayout=new QVBoxLayout();
  QFormLayout *formLayout=new QFormLayout();

  formLayout->addRow("DPI:",dpi);

  mainLayout->addLayout(formLayout);

  QDialogButtonBox *buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Cancel);

  okButton=buttonBox->button(QDialogButtonBox::Ok);

  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));

  connect(okButton,SIGNAL(clicked()),this,SLOT(Save()));
}

SettingsDialog::~SettingsDialog()
{
  // no code
}

void SettingsDialog::Save()
{
  std::cout << "Save()" << std::endl;

  if (dpi->hasAcceptableInput()) {
    std::cout << "DPI: " << dpi->value() << std::endl;

    settings->SetDPI((size_t)dpi->value());
  }
}

#include "moc_SettingsDialog.cpp"
