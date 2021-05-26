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
#include "rekisterinvienti.h"
#include "rekisteri/asiakastoimittajalistamodel.h"
#include "db/kirjanpito.h"
#include "asiakastoimittajadlg.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

void RekisterinVienti::vieRekisteri(QAbstractItemModel *model, const QString &tiedostonnimi)
{
    new RekisterinVienti(model, tiedostonnimi);
}

RekisterinVienti::RekisterinVienti(QAbstractItemModel *model, const QString &tiedostonnimi) :
    tiedosto_(tiedostonnimi)
{
    for(int i=0; i < model->rowCount(); i++) {
        idt_.enqueue(model->index(i,0).data(AsiakasToimittajaListaModel::IdRooli).toInt());
    }

    for(int i=0; i<14; i++)
        rk_.lisaaVenyvaSarake(100 / 14);

    RaporttiRivi rivi;
    rivi.lisaa("id");
    rivi.lisaa("nimi");
    rivi.lisaa("osoite");
    rivi.lisaa("postinumero");
    rivi.lisaa("kaupunki");
    rivi.lisaa("maa");
    rivi.lisaa("email");
    rivi.lisaa("puhelin");
    rivi.lisaa("kieli");
    rivi.lisaa("alvtunnus");
    rivi.lisaa("ytunnus");
    rivi.lisaa("lisätiedot");
    rivi.lisaa("ovt");
    rivi.lisaa("operaattori");
    rk_.lisaaOtsake(rivi);

    haeSeuraava();
}

void RekisterinVienti::haeSeuraava()
{
    if( idt_.isEmpty()) {
        valmis();
    } else {
        QString url = QString("/kumppanit/%1").arg(idt_.dequeue());
        KpKysely *kysely = kpk(url);
        connect(kysely, &KpKysely::vastaus, this, &RekisterinVienti::kumppaniSaapuu);
        kysely->kysy();
    }
}

void RekisterinVienti::kumppaniSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();

    RaporttiRivi rivi;
    rivi.lisaa(map.value("id").toString());
    rivi.lisaa(map.value("nimi").toString());
    rivi.lisaa(map.value("osoite").toString());
    rivi.lisaa(map.value("postinumero").toString());
    rivi.lisaa(map.value("kaupunki").toString());
    rivi.lisaa(map.value("maa").toString());
    rivi.lisaa(map.value("email").toString());
    rivi.lisaa(map.value("puhelin").toString());
    rivi.lisaa(map.value("kieli").toString());
    rivi.lisaa(map.value("alvtunnus").toString());
    rivi.lisaa( AsiakasToimittajaDlg::alvToY( map.value("alvtunnus").toString() ) );
    rivi.lisaa(map.value("lisatiedot").toString());
    rivi.lisaa(map.value("ovt").toString());
    rivi.lisaa(map.value("operaattori").toString());
    rk_.lisaaRivi(rivi);

    haeSeuraava();
}

void RekisterinVienti::valmis()
{
    QFile tiedosto(tiedosto_);
    if( tiedosto.open(QIODevice::WriteOnly)) {
        tiedosto.write(rk_.csv());
        emit kp()->onni(tr("%1 yhteystietoa viety").arg(rk_.riveja()));
    } else {
        QMessageBox::critical(nullptr, tr("Rekisterin vienti"),
                              tr("Tiedostoon %1 kirjoittaminen epäonnistui").arg(tiedosto_));
    }
    deleteLater();
}
