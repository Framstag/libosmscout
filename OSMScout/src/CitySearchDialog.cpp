/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

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

#include "CitySearchDialog.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

CitySearchDialog::CitySearchDialog(QWidget* parentWindow)
 : QDialog(parentWindow,Qt::Dialog),
   cityName(new QLineEdit()),
   results(new QListView())
{
  requestResultTimer.setSingleShot(true);
  results->setUniformItemSizes(true);

  QVBoxLayout *mainLayout=new QVBoxLayout();
  QFormLayout *formLayout=new QFormLayout();

  formLayout->addRow("City:",cityName);
  formLayout->addRow("Hits:",results);

  mainLayout->addLayout(formLayout);

  QDialogButtonBox *buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Cancel);

  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  connect(cityName,SIGNAL(textChanged(QString)),this, SLOT(OnCityNameChange(QString)));
  connect(&requestResultTimer,SIGNAL(timeout()),this, SLOT(RequestResults()));
  connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
}

CitySearchDialog::~CitySearchDialog()
{
  // no code
}

void CitySearchDialog::OnCityNameChange(const QString& text)
{
  requestResultTimer.stop();
  requestResultTimer.start(2000);
}

void CitySearchDialog::RequestResults()
{
  if (cityName->text().length()>0) {
    std::cout << "Request results..." << std::endl;
  }
}

#include "moc_CitySearchDialog.cpp"
