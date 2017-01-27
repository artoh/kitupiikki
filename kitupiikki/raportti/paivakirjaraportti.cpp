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

#include <QDateEdit>

#include <QSqlQuery>

#include "paivakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

PaivakirjaRaportti::PaivakirjaRaportti()
{
    ui = new Ui::Paivakirja;
    ui->setupUi(this);
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}

void PaivakirjaRaportti::alustaLomake()
{
    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());
}

void PaivakirjaRaportti::tulosta(QPrinter *printer)
{
    QPainter painter(printer);

    QSqlQuery kysely;
    QString kysymys = QString("SELECT pvm, tili, debetsnt, kreditsnt, selite, tunniste, nimi from vientivw "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" ORDER BY pvm")
                              .arg( ui->alkupvm->date().toString(Qt::ISODate) )
                              .arg( ui->loppupvm->date().toString(Qt::ISODate));
    kysely.exec(kysymys);

    painter.setFont(QFont("Sans",10));
    int rivinkorkeus = painter.fontMetrics().height();

    int sivu = 1;
    int sivunleveys = painter.window().width();
    int sivunkorkeus = painter.window().height();

    int pvmleveys = painter.fontMetrics().width("99.99.9999 ");
    int tilileveys = painter.fontMetrics().width("999999 Tilinnimi tarkenteineen ");
    int tositeleveys = painter.fontMetrics().width("AB123456 ");
    int euroleveys = painter.fontMetrics().width("-9 999 999,99€ ");

    // Pvm         Tosite       Tili              Selite            Debet       Kredit
    // 10.01.2017  A037         1911 Käteisvarat  Seliseliseli         25,00€



    while(kysely.next())
    {

        QRect seliterect = painter.boundingRect(pvmleveys + tositeleveys + tilileveys, 0,
                                          sivunleveys - pvmleveys - tositeleveys - tilileveys - 2 * euroleveys, rivinkorkeus * 3,
                                           Qt::TextWordWrap, kysely.value("selite").toString());

        if( painter.transform().dy() > sivunkorkeus - seliterect.height() )
        {
            // Sivu tulee täyteen
            printer->newPage();
            sivu = sivu + 1;
            painter.restore();
        }

        if( painter.transform().dy() == 0)
        {
            painter.save();

            tulostaYlatunniste( &painter, sivu, "PÄIVÄKIRJA",tr("%1 - %2").arg( ui->alkupvm->date().toString(Qt::SystemLocaleShortDate) )
                                .arg( ui->loppupvm->date().toString(Qt::SystemLocaleShortDate)));

            painter.translate(0, rivinkorkeus );

            painter.drawText( QRect(0,0,pvmleveys,rivinkorkeus), Qt::AlignLeft, tr("Pvm")  );
            painter.drawText( QRect( pvmleveys, 0, tositeleveys, rivinkorkeus ), Qt::AlignLeft, tr("Tosite") );
            painter.drawText( QRect( pvmleveys + tositeleveys, 0, tilileveys, rivinkorkeus), Qt::AlignLeft, tr("Tili"));
            painter.drawText( QRect( pvmleveys + tositeleveys + tilileveys, 0, tilileveys, rivinkorkeus), Qt::AlignLeft,  tr("Selite"));
            painter.drawText( QRect( sivunleveys - 2 * euroleveys, 0, euroleveys, rivinkorkeus), Qt::AlignRight, tr("Debet €") );
            painter.drawText( QRect( sivunleveys - euroleveys, 0, euroleveys, rivinkorkeus), Qt::AlignRight, tr("Kredit €") );

            painter.translate(0, rivinkorkeus );
            painter.drawLine(0,0,sivunleveys,0);



            // Jos taustan raidoittaminen on valittu, niin raidoitetaan aina neljä riviä ja sitten neljä ilman raitaa taustalle
            if( ui->raidoita->isChecked())
            {
                painter.save();
                painter.setBrush(QBrush(Qt::lightGray));
                painter.setPen(Qt::NoPen);
                painter.translate(0,rivinkorkeus*4);
                while( painter.transform().dy() < sivunkorkeus - rivinkorkeus * 4)
                {
                    painter.drawRect(0,0,sivunleveys,rivinkorkeus*4);
                    painter.translate(0, rivinkorkeus*8);
                }
                painter.restore();
            }
        }


        painter.drawText( QRect(0,0,pvmleveys,rivinkorkeus), Qt::AlignLeft, kysely.value("pvm").toDate().toString(Qt::SystemLocaleShortDate)  );
        painter.drawText( QRect( pvmleveys, 0, tositeleveys, rivinkorkeus ), Qt::AlignLeft, kysely.value("tunniste").toString() );
        painter.drawText( QRect( pvmleveys + tositeleveys, 0, tilileveys, rivinkorkeus), Qt::AlignLeft, tr("%1 %2").arg(kysely.value("tili").toString()).arg(kysely.value("nimi").toString()));
        painter.drawText( seliterect, Qt::TextWordWrap, kysely.value("selite").toString());
        if( kysely.value("debetsnt").toInt() )
            painter.drawText( QRect( sivunleveys - 2 * euroleveys, 0, euroleveys, rivinkorkeus), Qt::AlignRight, QString("%L1").arg( kysely.value("debetsnt").toDouble() / 100.0 ,0,'f',2 ) );
        if( kysely.value("kreditsnt").toInt())
            painter.drawText( QRect( sivunleveys - euroleveys, 0, euroleveys, rivinkorkeus), Qt::AlignRight, QString("%L1").arg( kysely.value("kreditsnt").toDouble() / 100.0 ,0,'f',2 ) );

        painter.translate(0, seliterect.height());

    }
    // Tyhjennetään alalaita mahdollisista raidoista
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0,0,sivunleveys, sivunkorkeus - painter.transform().dy());

    painter.restore();

}
