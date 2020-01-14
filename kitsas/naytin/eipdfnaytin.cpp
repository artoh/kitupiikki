/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "eipdfnaytin.h"

#include "db/kirjanpito.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <QFile>
#include <QDesktopServices>
#include <QMessageBox>

Naytin::EiPdfNaytin::EiPdfNaytin(const QByteArray &pdf, QObject *parent) :
    AbstraktiNaytin (parent),
    data_(pdf),
    widget_( new QWidget)
{
    QLabel *teksti = new QLabel( tr("Käyttäjä on poistanut käytöstä pdf-tiedostojen katselun\n"
                                "Esikatselun voi ottaa uudelleen käyttöön sivulta Määritykset / Perusvalinnat"));
    QPushButton *nappi = new QPushButton(QIcon(":/pic/pdf.png"), tr("Avaa ohjelmalla..."));
    QHBoxLayout *leiska = new QHBoxLayout();
    leiska->addWidget(teksti);
    leiska->addWidget(nappi);
    widget_->setLayout(leiska);

    connect( nappi, &QPushButton::clicked, this, &EiPdfNaytin::avaaTiedostolla);
}

QByteArray Naytin::EiPdfNaytin::data() const
{
    return data_;
}

void Naytin::EiPdfNaytin::paivita() const
{

}

void Naytin::EiPdfNaytin::tulosta(QPrinter * /* printer */) const
{

}

void Naytin::EiPdfNaytin::avaaTiedostolla()
{
    // Luo tilapäisen tiedoston
    QString tiedostonnimi = kp()->tilapainen( QString("liite-XXXX.pdf") );

    QFile tiedosto( tiedostonnimi);
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( data());
    tiedosto.close();

    if( !QDesktopServices::openUrl( QUrl::fromLocalFile(tiedosto.fileName()) ))
        QMessageBox::critical( nullptr, tr("Tiedoston avaaminen"), tr("Pdf-tiedostoja näyttävän ohjelman käynnistäminen ei onnistunut") );
}
