/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>

#include "ktpvienti.h"

#include "ktpintro.h"
#include "ktpperustiedot.h"
#include "ktpkuvaus.h"
#include "ktpaloitusteksti.h"

#include "db/kirjanpito.h"


KtpVienti::KtpVienti()
{
    setWindowTitle( tr("Tilikarttatiedoston luominen"));
    setPixmap( WatermarkPixmap, QPixmap(":/pic/salkkupossu.png"));

    addPage(new KtpIntro);
    addPage(new KtpPerustiedot);
    addPage(new KtpKuvaus);
    addPage(new KtpAloitusTeksti);

}

void KtpVienti::accept()
{
    // Koko velho on näytetty. Nyt kysytään vielä tiedosto.
    QString tiedostoPolku = QFileDialog::getSaveFileName(this, tr("Tallenna uusi tilikarttatiedosto"),
                                                    QDir::homePath(), tr("Kitupiikin tilikartta (*.kpk)"));
    if( tiedostoPolku.isEmpty())
    {
        return;
    }

    QFileInfo info(tiedostoPolku);
    if( info.suffix().isEmpty())
        tiedostoPolku.append(".kpk");

    // Tallennetaan tiedosto

    QFile tiedosto(tiedostoPolku);
    if( !tiedosto.open( QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("Virhe tilikarttatiedoston luomisessa"),
                              tr("Ei voi kirjoittaa tiedostoon %1\n%2")
                              .arg(tiedostoPolku).arg( qPrintable(tiedosto.errorString()) ));
        return;
    }
    QTextStream out(&tiedosto);
    out.setCodec("UTF-8");

    // TIETOKENTÄT

    out << "[TilikarttaNimi]\n";
    out << field("nimi").toString() << "\n";

    if( !field("nimi").toString().isEmpty())
    {
        out << "[TilikarttaTekija]\n";
        out << field("tekija").toString() << "\n";
    }

    out << "[TilikarttaPvm]\n";
    out << field("pvm").toDate().toString(Qt::ISODate) << "\n";

    out << "[kuvaus]\n";
    out << field("kuvaus").toString() << "\n";

    out << "[TilikarttaOhje]\n";
    out << field("introteksti").toString() << "\n";

    out << "[TilikarttaLuontiVersio]\n";
    out << qApp->applicationVersion() << "\n";

    out << "[KpVersio]\n";
    out << Kirjanpito::TIETOKANTAVERSIO << "\n";

    // TILIT
    // AP* 1911 {json} Suosikkitili
    // C- 3001 {json} Piilotettu tili

    out << "[tilit]\n";
    for( int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++ )
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);
        int tilinro = tili.numero();
        QString nimi = tili.nimi();
        QString tyyppi = tili.tyyppiKoodi();

        if( tili.otsikkotaso() )
            tyyppi = QString("H%1").arg(tili.otsikkotaso());

        int tila = tili.tila();

        QString tilamerkki;
        if( tila == 2)
            tilamerkki ="*";
        else if( tila == 0)
            tilamerkki = "-";

        out << QString("%1%2 %3 %4 %5\n")
               .arg(tyyppi)
               .arg(tilamerkki)
               .arg(tilinro)
               .arg( QString(tili.json()->toJson()))
               .arg( nimi );

    }

    // Tositelajit
    // Viedään tositelajista kaksi eteenpäin: Ei järjestelmää (*) eikä Muu tosite -oletustositetta
    out << "[tositelajit]\n";
    for( int i=0; i < kp()->tositelajit()->rowCount(QModelIndex()); i++)
    {
        QModelIndex indeksi = kp()->tositelajit()->index(i, 0);

        // OL {json} Ostolaskut
        QString tunnus = indeksi.data(TositelajiModel::TunnusRooli).toString();

        if( !tunnus.isEmpty() && tunnus != "*")
            out << QString("%1 %2 %3\n")
                   .arg( tunnus )
                   .arg( QString(indeksi.data(TositelajiModel::JsonRooli).toByteArray()))
                   .arg( indeksi.data(TositelajiModel::NimiRooli).toString() );

    }

    // Raportit ja joitakin muita asetuksia
    QStringList avaimet;
    avaimet << "AlvVelvollinen"
            << "TilinpaatosPohja" << "TilinpaatosValinnat"
            << "LaskuTositelaji" << "LaskuKirjausperuste"
            << "LaskuSaatavatili" << "LaskuKateistili"
            << "LaskuMaksuaika" << "LaskuHuomautusaika"
            << "ArkistoRaportit";

    avaimet << kp()->asetukset()->avaimet("Raportti/");


    foreach (QString avain, avaimet)
    {
        if( kp()->asetukset()->onko(avain))
            out << QString("[%1]\n%2\n").arg(avain).arg( kp()->asetukset()->asetus(avain) );
    }


    tiedosto.close();

    QDialog::accept();
}
