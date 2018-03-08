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

#include <QDialog>

#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>

#include <QMessageBox>

#include <QRegularExpression>

#include "paivitakirjanpito.h"

#include "db/kirjanpito.h"
#include "db/tili.h"
#include "uusikirjanpito.h"

#include "ui_tkpaivitys.h"
#include "ui_paivityskorvaa.h"

#include "skripti.h"


QString PaivitaKirjanpito::sisainenPaivitys()
{
    QString tiedostonnimi = kp()->asetukset()->asetus("VakioTilikartta");
    if( tiedostonnimi.isEmpty())
        return QString();

    QMap<QString, QStringList> ktk = UusiKirjanpito::lueKtkTiedosto(":/tilikartat/" + tiedostonnimi);
    QDate paivays = QDate::fromString(ktk.value("TilikarttaPvm").join(""), Qt::ISODate );
    if( !paivays.isValid() || paivays <= kp()->asetukset()->pvm("TilikarttaPvm"))
        return QString();

    return ktk.value("TilikarttaNimi").join("") + " " +
            paivays.toString(Qt::SystemLocaleShortDate);
}

bool PaivitaKirjanpito::paivitaTilikartta()
{

    QDialog dlg;
    Ui::TilikarttaPaivitys ui;
    ui.setupUi(&dlg);

    QString sispaivitys = sisainenPaivitys();

    ui.sisainen->setText( ui.sisainen->text() + "\n" + sispaivitys );

    ui.sisainen->setEnabled( !sispaivitys.isEmpty());
    ui.sisainen->setChecked( !sispaivitys.isEmpty());
    ui.tiedosto->setChecked( sispaivitys.isEmpty());
    connect( ui.ohjeNappi, &QPushButton::clicked ,  [] { kp()->ohje("aloitus");  ;} );

    if( dlg.exec() == QDialog::Accepted)
    {
        QString tilikarttaTiedosto;
        if( ui.sisainen->isChecked())
        {
            tilikarttaTiedosto = ":/tilikartat/" + kp()->asetukset()->asetus("VakioTilikartta");
        }
        else
        {
            tilikarttaTiedosto = QFileDialog::getOpenFileName(0, tr("Valitse tilikarttatiedosto, johon päivitetään"),
                                                              QDir::homePath(), "Tilikartta (*.kpk)");
        }
        if( !tilikarttaTiedosto.isEmpty())
            return lataaPaivitys( tilikarttaTiedosto);
    }

    return false;
}


bool PaivitaKirjanpito::lataaPaivitys(const QString &tiedosto)
{
    QMap<QString,QStringList> ktk = UusiKirjanpito::lueKtkTiedosto(tiedosto);

    QString vakiokartta = ktk.value("VakioTilikartta").join("");
    if( vakiokartta != kp()->asetukset()->asetus("VakioTilikartta"))
    {
        // Vielä varmuudeksi viimeinen tyyppitarkastus !

        if( QMessageBox::warning(0, tr("Tilikartan päivitys"),
                                 tr("Päivitettävän tilikartan tyyppitieto %1 poikkeaa "
                                    "nykyisen tilikartan tyyppitiedosta %2.\n\n"
                                    "Oletko varma, että haluat päivittää tilikartan?"),
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return false;
    }


    // Tarkistetaan, onko raportteja ja tilinpäätöstä muokattu
    QStringList raportit = kp()->asetukset()->avaimet("Raportti/");
    bool rapoYlikirjoita = false;
    QStringList muokatutRaportit;

    for(auto raportti : raportit)
    {
        if( kp()->asetukset()->muokattu(raportti).isValid())
        {
            rapoYlikirjoita = true;
            muokatutRaportit.append( raportti.mid(9));
        }
    }
    bool tpYlikirjoita = kp()->asetukset()->muokattu("TilinpaatosPohja").isValid();

    if( rapoYlikirjoita || tpYlikirjoita)
    {
        QDialog dlg;
        Ui::PaivitysKorvaa ui;
        ui.setupUi(&dlg);

        ui.raporttiGroup->setVisible(rapoYlikirjoita);
        ui.muokatutRaportit->setText( muokatutRaportit.join("\n") );
        ui.kaavaGroup->setVisible(tpYlikirjoita);

        connect( ui.ohjeNappi, &QPushButton::clicked ,  [] { kp()->ohje("aloitus"); ;} );


        if( dlg.exec() != QDialog::Accepted)
            return false;

        // Korvataan raportit tai kaavat
        // Jos ei korvata, niin sitten säilytetään!

        if( rapoYlikirjoita )
            rapoYlikirjoita = ui.korvaaRaportit->isChecked();
        if( tpYlikirjoita )
            tpYlikirjoita = ui.korvaaKaava->isChecked();
    }


    QStringList siirrettavat;
    siirrettavat             << "TilikarttaNimi" << "TilikarttaKuvaus"
                             << "TilikarttaOhje" << "TilikarttaPvm"
                             << "TilikarttaTekija" << "TilikarttaLuontiVersio";

    if( tpYlikirjoita )
        siirrettavat.append("TilinpaatosPohja");

    // Estetään muokkauspäivien tallentuminen asetuksiin
    kp()->asetukset()->tilikarttaMoodiin(true);

    // Siirretään asetuksien muutokset
    QMapIterator<QString,QStringList> i(ktk);
    while (i.hasNext())
    {
        i.next();
        if( siirrettavat.contains( i.key())  )
        {
                kp()->asetukset()->aseta(i.key(), i.value());
        }
        else if( i.key().startsWith("Raportti/"))
        {
            if( rapoYlikirjoita || !kp()->asetukset()->onko(i.key()))
                kp()->asetukset()->aseta(i.key(), i.value());

        }
    }
    kp()->asetukset()->tilikarttaMoodiin(false);

    // Sitten vielä tilit
    QRegularExpression tiliRe("^(?<tyyppi>\\w{1,5})(?<tila>[\\*\\-]?)\\s+(?<nro>\\d{1,8})(\\.\\.(?<asti>\\d{1,8}))?"
                              "\\s*(?<json>\\{.*\\})?\\s(?<nimi>.+)$");

    QStringList tililista = ktk.value("tilit");
    foreach ( QString tilirivi, tililista)
    {
        // Tilitietueet ovat TYYPPI[*-] numero {json} Nimi
        QRegularExpressionMatch mats = tiliRe.match(tilirivi);
        if( mats.hasMatch())
        {
            int numero = mats.captured("nro").toInt();
            QString tyyppi = mats.captured("tyyppi");
            int otsikkotaso = 0;
            if( tyyppi.startsWith('H') && tyyppi.length()==2)
                otsikkotaso = tyyppi.mid(1).toInt();

            if( !kp()->tilit()->tiliNumerolla(numero, otsikkotaso).onkoValidi())
            {
                Tili tili;
                tili.asetaTyyppi( mats.captured("tyyppi"));

                if( mats.captured("tila") == "*")
                    tili.asetaTila(2);
                else if( mats.captured("tila") == "-")
                    tili.asetaTila(0);
                else
                    tili.asetaTila(1);

                tili.asetaNumero( mats.captured("nro").toInt());
                tili.asetaNimi( mats.captured("nimi"));
                tili.json()->fromJson( mats.captured("json").toUtf8());

                if( !mats.captured("asti").isEmpty() )
                    tili.json()->set("Asti", mats.captured("asti").toInt());

                kp()->tilit()->lisaaTili(tili);
            }
            else
            {
                // Ei muuteta käyttäjän muokkaamia tilejä
                if( kp()->tilit()->tiliNumerolla(numero, otsikkotaso).muokkausaika().isValid() )
                    continue;

                int ysiluku = Tili::ysiluku(numero, otsikkotaso);


                // Etsitään oikea tili modelista ja muutetaan sitä
                for(int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
                {
                    if( ysiluku == kp()->tilit()->tiliIndeksilla(i).ysivertailuluku() )
                    {
                        QModelIndex index = kp()->tilit()->index(i, TiliModel::NIMI);

                        kp()->tilit()->setData( index, mats.captured("nimi"), TiliModel::NimiRooli );
                        kp()->tilit()->setData( index, tyyppi, TiliModel::TyyppiRooli);
                        JsonKentta *json = kp()->tilit()->jsonIndeksilla(i);
                        json->fromJson( mats.captured("json").toUtf8() );
                        if( !mats.captured("asti").isEmpty() )
                            json->set("Asti", mats.captured("asti").toInt());

                        break;
                    }
                }
            }
        }
    }   // Tilirivien lukeminen
    if( kp()->tilit()->tallenna(true))
    {
        // Suoritetaan skriptit
        QString muoto = kp()->asetukset()->asetus("Muoto");
        if( !muoto.isEmpty())
          Skripti::suorita( ktk.value("MuotoPois/" + muoto ));
        Skripti::suorita( ktk.value("PaivitysSkripti"));
        if( !muoto.isEmpty())
          Skripti::suorita( ktk.value("MuotoOn/" + muoto));

        QMessageBox::information(0, tr("Kitupiikki"),tr("Tilikartta päivitetty") );
        return true;
    }
    return false;
}
