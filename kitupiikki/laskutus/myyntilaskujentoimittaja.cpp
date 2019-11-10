/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "myyntilaskujentoimittaja.h"
#include "laskudialogi.h"
#include "myyntilaskuntulostaja.h"
#include "model/tosite.h"

#include "db/kirjanpito.h"

#include <QVariantMap>
#include <QList>

#include <QPrintDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>
#include <QFileDialog>

bool MyyntiLaskujenToimittaja::toimitaLaskut(const QList<QVariantMap> &laskut)
{
   laskuja_ = laskut.count();

    // Ensin lajitellaan
    for( QVariantMap lasku : laskut)
    {
        int toimitustapa = lasku.value("lasku").toMap().value("laskutapa").toInt();
        if( toimitustapa == LaskuDialogi::TULOSTETTAVA)
            tulostettavat_.append(lasku);
        else if( toimitustapa == LaskuDialogi::PDF)
            tallennettavat_.append(lasku);
    }

    if( !tulostettavat_.isEmpty())
        tulosta();
    if( !tallennettavat_.isEmpty())
        tallenna();

    return true;

}

void MyyntiLaskujenToimittaja::toimitaLaskut(const QList<int> &tositteet)
{
    if( !tositteet.isEmpty()) {
        tilattavat_ = tositteet;
        tilaaSeuraavaLasku();
    }
}

void MyyntiLaskujenToimittaja::toimitettu()
{
    toimitetut_++;
    if( toimitetut_ == laskuja_)
        emit laskutToimitettu();
}

void MyyntiLaskujenToimittaja::tositeSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    toimitettavat_.append(map);
    if( tilattavat_.isEmpty())
        toimitaLaskut( toimitettavat_ );
    else
        tilaaSeuraavaLasku();
}

bool MyyntiLaskujenToimittaja::tulosta()
{
    QPageLayout vanhaleiska = kp()->printer()->pageLayout();
    QPageLayout uusileiska = vanhaleiska;
    uusileiska.setUnits(QPageLayout::Millimeter);
    uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
    kp()->printer()->setPageLayout(uusileiska);



    QPrintDialog printDialog( kp()->printer() );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer() );
        for(QVariantMap tulostettava : tulostettavat_)
        {
            MyyntiLaskunTulostaja::tulosta(tulostettava, kp()->printer(), &painter, true);
            merkkaaToimitetuksi( tulostettava.value("id").toInt() );
        }
    }

    kp()->printer()->setPageLayout(vanhaleiska);
    return true;
}

bool MyyntiLaskujenToimittaja::tallenna()
{
    QString hakemistopolku = QFileDialog::getExistingDirectory(nullptr, tr("Tallennettavien laskujen kansio"));
    if( !hakemistopolku.isEmpty()) {
        QDir hakemisto( hakemistopolku );
        for( QVariantMap tallennettava : tallennettavat_) {
            int laskunumero = tallennettava.value("lasku").toMap().value("numero").toInt();
            QFile ulos( hakemisto.filePath( QString("lasku%1.pdf").arg(laskunumero,8,10,QChar('0'))) );
            if( ulos.open(QIODevice::WriteOnly) ) {
                ulos.write( MyyntiLaskunTulostaja::pdf(tallennettava) );
                merkkaaToimitetuksi( tallennettava.value("id").toInt() );
            }
        }
    }
    return true;
}

void MyyntiLaskujenToimittaja::merkkaaToimitetuksi(int tositeId)
{
        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::PATCH);
        QVariantMap map;
        map.insert("tila", Tosite::LAHETETTYLASKU);
        connect( kysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::toimitettu);
        kysely->kysy(map);
}

void MyyntiLaskujenToimittaja::tilaaSeuraavaLasku()
{
    int id = tilattavat_.takeFirst();
    KpKysely* kysely = kpk(QString("/tositteet/%1").arg(id));
    connect( kysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::tositeSaapuu );
    kysely->kysy();
}

MyyntiLaskujenToimittaja::MyyntiLaskujenToimittaja(QObject *parent) : QObject(parent)
{

}
