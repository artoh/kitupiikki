/*
   Copyright (C) 2017 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "nimisivu.h"
#include "validator/ibanvalidator.h"
#include "uusikirjanpito.h"

NimiSivu::NimiSivu()
{
    setTitle("Organisaation tiedot");
    ui = new Ui::NimiSivu();
    ui->setupUi(this);

    ui->tiliLine->setValidator(new IbanValidator());

    registerField("nimi*",ui->nimiEdit);
    registerField("ytunnus",ui->ytunnusEdit);
    registerField("iban", ui->tiliLine);

    // Piilotettu kenttä muodolle
    QLineEdit *piilo = new  QLineEdit(this);
    piilo->hide();
    registerField("muoto*", piilo);

    connect( ui->muotoList, SIGNAL(itemSelectionChanged()),
             this, SLOT(valittu()));
}

NimiSivu::~NimiSivu()
{
    delete ui;

}

void NimiSivu::initializePage()
{
    // Haetaan valitusta tilikartasta sen sisältämät muodot
    QString polku = field("tilikartta").toString();
    QMap<QString,QStringList> ktk = UusiKirjanpito::lueKtkTiedosto(polku);

    QMapIterator<QString, QStringList> iter(ktk);

    ui->muotoList->clear();

    QString valittuTeksti = ktk.value("Muoto").join("");

    while( iter.hasNext() )
    {
        iter.next();
        if( iter.key() == "MuotoTeksti")
            ui->muotoLabel->setText( iter.value().join(" "));
        else if( iter.key().startsWith("MuotoOn/"))
        {
            QString nimi = iter.key().mid(8);
            QListWidgetItem *item = new QListWidgetItem(nimi, ui->muotoList);
            if( nimi == valittuTeksti)
            {
                ui->muotoList->setCurrentItem(item);
            }
        }
    }

    ui->muotoLabel->setVisible( ui->muotoList->count() );
    ui->muotoList->setVisible( ui->muotoList->count());

    if( !ui->muotoList->count() )
    {
        // Ei muotoja
        setField("muoto", "-");
    }
}

void NimiSivu::valittu()
{
    QList<QListWidgetItem*> valitut = ui->muotoList->selectedItems();
    if( valitut.count())
        setField("muoto", valitut.first()->text());
}

