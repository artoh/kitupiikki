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
#include "palkkatilimaaritys.h"
#include "db/kirjanpito.h"
#include "tools/tilicombo.h"
#include <QJsonDocument>
#include <QDebug>
#include "ui_palkkatilimaaritys.h"

PalkkatiliMaaritys::PalkkatiliMaaritys()
{
    ui = new Ui::PalkkatiliMaaritys;
    ui->setupUi(this);
}

bool PalkkatiliMaaritys::nollaa()
{
    QVariantMap palkkatilit = QJsonDocument::fromJson( kp()->asetus("palkkatilit").toUtf8() ).toVariant().toMap();

    for( TiliCombo* tcombo : findChildren<TiliCombo*>() )  {
        QString koodi = tcombo->property("pkoodi").toString();
        int tili = palkkatilit.value(koodi).toInt();
        tcombo->suodataTyypilla("[ABCD].*",true);
        tcombo->valitseTili(tili);
        connect( tcombo, &TiliCombo::tiliValittu, this, &PalkkatiliMaaritys::ilmoitaMuokattu);
    }
    return true;
}

bool PalkkatiliMaaritys::tallenna()
{
    QString asetus = QString::fromUtf8(QJsonDocument::fromVariant(taulu()).toJson(QJsonDocument::Compact));
    kp()->asetukset()->aseta("palkkatilit", asetus);
    emit tallennaKaytossa(false);
    return true;
}

bool PalkkatiliMaaritys::onkoMuokattu()
{
    QString asetus = QString::fromUtf8(QJsonDocument::fromVariant(taulu()).toJson(QJsonDocument::Compact));
    QString verrokki = QString::fromUtf8(QJsonDocument::fromJson(kp()->asetus("palkkatilit").toUtf8()).toJson(QJsonDocument::Compact));
    qDebug() << asetus;
    qDebug() << verrokki;
    return asetus != verrokki;
}

QVariantMap PalkkatiliMaaritys::taulu() const
{
    QVariantMap tilit;
    for( TiliCombo* tcombo : findChildren<TiliCombo*>() )  {
        QString koodi = tcombo->property("pkoodi").toString();
        tilit.insert(koodi, tcombo->valittuTilinumero());
    }
    return tilit;
}

void PalkkatiliMaaritys::ilmoitaMuokattu()
{
    emit tallennaKaytossa(onkoMuokattu());
}
