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

#include <QDebug>

#include <QSqlQuery>
#include <QImage>
#include <QMessageBox>
#include <QSettings>

#include "tuonti.h"
#include "pdftuonti.h"
#include "csvtuonti.h"
#include "titotuonti.h"
#include "palkkafituonti.h"
#include "validator/ytunnusvalidator.h"

#include "kirjaus/kirjauswg.h"
#include "db/tili.h"
#include "db/eranvalintamodel.h"
#include "laskutus/laskumodel.h"
#include "kirjaus/ehdotusmodel.h"

Tuonti::Tuonti(KirjausWg *wg)
    :  kirjausWg_(wg)
{

}

Tuonti::~Tuonti()
{

}

bool Tuonti::tuo(const QString &tiedostonnimi, KirjausWg *wg)
{
    QImage kuvako( tiedostonnimi );
    if( !kuvako.isNull())
        return true;

    QFile tiedosto( tiedostonnimi );
    tiedosto.open( QFile::ReadOnly );

    QByteArray data = tiedosto.readAll();
    tiedosto.close();

    if( data.startsWith("%PDF") && !kp()->settings()->value("PopplerPois").toBool())
    {
        PdfTuonti pdftuonti(wg);
        return pdftuonti.tuo(data);
    }

    QString ytunnus = QString( data.left(11).right(9) );

    if( data.startsWith( "T;") && YTunnusValidator::kelpaako(ytunnus))
    {
        if( !data.startsWith( QString("T;%1").arg( kp()->asetukset()->asetus("Ytunnus") ).toUtf8() ) )
        {
            if( kp()->onkoHarjoitus())
            {
                if( QMessageBox::question(nullptr, kp()->tr("Palkkatositteen tuonti"),
                                          kp()->tr("Palkkatositteessa oleva y-tunnus %1 poikkeaa omasta y-tunnuksesta. Haluatko jatkaa?").arg(ytunnus),
                                          QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
                    return false;
            }
            else
            {
                QMessageBox::critical(nullptr, kp()->tr("Palkkatositteen tuonti"),
                                      kp()->tr("Palkkatositteessa oleva y-tunnus %1 poikkeaa omasta y-tunnuksesta. "
                                               "Palkkatositetta ei voi tuoda.").arg(ytunnus));
                return false;
            }
        }

        PalkkaFiTuonti palkkatuonti(wg);
        return palkkatuonti.tuo(data);
    }
    else if( data.startsWith("T00322100"))  // Konekielisen tiliotteen TITO-tiedoston alkutunniste
    {
        TitoTuonti titotuonti(wg);
        return titotuonti.tuo(data);
    }
    else if( CsvTuonti::onkoCsv(data))
    {
        CsvTuonti csvtuonti(wg);
        return csvtuonti.tuo(data);
    }


    return true;
}

void Tuonti::tuoLasku(qlonglong sentit, QDate laskupvm, QDate toimituspvm, QDate erapvm, QString viite, const QString& tilinumero, const QString& saajanNimi)
{
    QDate pvm = toimituspvm;
    if( !pvm.isValid() || kp()->asetukset()->luku("TuontiOstolaskuPeruste") == LASKUPERUSTEINEN)
        pvm = laskupvm;
    if( !pvm.isValid() || kp()->asetukset()->luku("TuontiOstolaskuPeruste") == MAKSUPERUSTEINEN )
        pvm = erapvm;

    kirjausWg()->gui()->otsikkoEdit->setText(saajanNimi);
    kirjausWg()->gui()->tositePvmEdit->setDate(pvm);


    VientiRivi rivi;
    rivi.pvm = pvm;
    rivi.selite = saajanNimi;
    rivi.asiakas = saajanNimi;

    if( !tilinumero.isEmpty() &&  kp()->tilit()->tiliIbanilla(tilinumero).onkoValidi() )
    {
        // Oma tili eli onkin myyntilasku
        // Tositelajiksi myyntilaskut, mutta toistaiseksi ei muuten kirjaudu
        kirjausWg()->gui()->tositetyyppiCombo->setCurrentIndex(
                    kirjausWg()->gui()->tositetyyppiCombo->findData(TositelajiModel::MYYNTILASKUT, TositelajiModel::KirjausTyyppiRooli) );

        rivi.debetSnt = sentit;
    }
    else
    {
        kirjausWg()->gui()->tositetyyppiCombo->setCurrentIndex(
                    kirjausWg()->gui()->tositetyyppiCombo->findData( kp()->asetukset()->luku("TuontiOstolaskuTositelaji"), TositelajiModel::IdRooli ) );
        rivi.tili = kp()->tilit()->tiliNumerolla( kp()->asetukset()->luku("TuontiOstolaskuTili") );
        rivi.kreditSnt = sentit;
    }

    // Poistetaan viitenumeron etunollat
    viite.replace( QRegularExpression("^0*"),"");

    rivi.viite = viite;
    rivi.ibanTili = tilinumero;
    rivi.erapvm = erapvm;
    rivi.json.set("SaajanNimi", saajanNimi);
    rivi.eraId = TaseEra::UUSIERA;
    rivi.laskupvm = laskupvm;

    kirjausWg()->model()->vientiModel()->lisaaVienti(rivi);
    kirjausWg()->tiedotModeliin();

}

bool Tuonti::tiliote(const QString& iban, QDate mista, QDate mihin)
{
    return tiliote( kp()->tilit()->tiliIbanilla(iban),
                    mista, mihin);
}

bool Tuonti::tiliote(Tili tili, QDate mista, QDate mihin)
{
    tiliotetili_ = tili;
    if( !tiliotetili_.onko(TiliLaji::PANKKITILI))
        return false;

    if( mista > mihin)
    {
        // Joskus päivämäärät sotkeutuvat...
        QDate apu = mihin;
        mihin = mista;
        mista = apu;
    }

    for(int i=0; i < kp()->tositelajit()->rowCount(QModelIndex()); i++)
    {
        QModelIndex index = kp()->tositelajit()->index(i,0);
        if( index.data(TositelajiModel::KirjausTyyppiRooli).toInt() == TositelajiModel::TILIOTE &&
            index.data(TositelajiModel::VastatiliNroRooli).toInt() == tiliotetili().numero())
        {
            // Tämä on kyseisen tiliotteen tositelaji
            kirjausWg()->gui()->tositetyyppiCombo->setCurrentIndex(
                        kirjausWg()->gui()->tositetyyppiCombo->findData( index.data(TositelajiModel::IdRooli), TositelajiModel::IdRooli ));
            break;
        }
    }


    kirjausWg()->gui()->tiliotetiliCombo->setCurrentIndex(
                kirjausWg()->gui()->tiliotetiliCombo->findData( tiliotetili().id(), TiliModel::IdRooli ));
    kirjausWg()->gui()->tilioteBox->setChecked(true);

    if( mista.isValid() && mihin.isValid())
    {
        kirjausWg()->gui()->tiliotealkaenEdit->setDate(mista);
        kirjausWg()->gui()->tilioteloppuenEdit->setDate(mihin);
        kirjausWg()->gui()->otsikkoEdit->setText( kp()->tr("Tiliote %1 - %2")
                                              .arg(mista.toString("dd.MM.yyyy")).arg(mihin.toString("dd.MM.yyyy")));
    }
    else
        kirjausWg()->gui()->otsikkoEdit->setText( kp()->tr("Tiliote"));

    kirjausWg()->gui()->tositePvmEdit->setDate(mihin);

    return true;
}

void Tuonti::oterivi(QDate pvm, qlonglong sentit, const QString& iban, QString viite, const QString& arkistotunnus, QString selite)
{
    // RF-viitteen muunto kansalliseksi, koska laskunnumerona on kansallinen viite
    if( viite.startsWith("RF"))
        viite = viite.mid(4);

    // Etunollien poisto viiterivistä
    viite.replace( QRegularExpression("^0*"),"");


    // Tuplatuonnin esto
    if(!arkistotunnus.isEmpty())
    {
    QSqlQuery tupla( QString("SELECT id FROM vienti WHERE arkistotunnus='%1'").arg(arkistotunnus));
    if( tupla.next() )
        return;
    }

    VientiRivi vastarivi;
    vastarivi.pvm = pvm;

    QStringList omaEhtoistenVerojenTilit;
    omaEhtoistenVerojenTilit << "FI6416603000117625" << "FI5689199710000724" << "FI3550000120253504";

    // Etsitään mahdollinen erä, johon liittyy
    // MYYNTILASKU
    if( sentit > 0 && !viite.isEmpty())
    {
        QSqlQuery kysely( QString("SELECT eraid FROM vienti WHERE viite='%1' AND iban IS NULL ").arg(viite));
        while( kysely.next())
        {
            TaseEra era( kysely.value(0).toInt() );
            if( era.saldoSnt >= sentit )
            {
                // Tällä viittellä on lasku, joka voidaan maksaa
                // Viitteen maksamiseen tarvitaan erän tiedot
                QSqlQuery tilikysely( QString("SELECT tili, selite, kohdennus FROM vienti WHERE id=%1").arg(era.eraId));
                if( tilikysely.next() && tilikysely.value("tili").toInt())
                {
                    vastarivi.tili = kp()->tilit()->tiliIdlla( tilikysely.value("tili").toInt() );
                    vastarivi.kohdennus = kp()->kohdennukset()->kohdennus( tilikysely.value("kohdennus").toInt() );
                    vastarivi.selite = tilikysely.value("kohdennus").toString();
                    vastarivi.eraId = era.eraId;
                    break;
                }
            }

        }

    }
    else if(  sentit < 0 && omaEhtoistenVerojenTilit.contains(iban) )
    {
        // Verojen maksua, kohdistuu Verovelka-tilille
        vastarivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA);

        // Mahdollisen alv-velan kuittaaminen alv-saatavilla
        if( kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).onkoValidi())
        {
            if( kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).saldoPaivalle(pvm) == qAbs(sentit) + kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).saldoPaivalle(pvm) )
            {
                VientiRivi verodebet;
                verodebet.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA);
                verodebet.pvm = pvm;
                verodebet.debetSnt = kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).saldoPaivalle(pvm);
                verodebet.selite = Kirjanpito::tr("Verovelka kuitataan saatavilla");

                VientiRivi verokredit;
                verokredit.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA);
                verokredit.pvm = pvm;
                verokredit.kreditSnt = verodebet.debetSnt;
                verokredit.selite = verodebet.selite;

                EhdotusModel veronkuittaus;
                veronkuittaus.lisaaVienti(verodebet);
                veronkuittaus.lisaaVienti(verokredit);
                veronkuittaus.tallenna( kirjausWg()->model()->vientiModel() );

            }
        }

    }
    else if( sentit < 0 && !iban.isEmpty() && !viite.isEmpty())
    {
        // Ostolasku
        // Kirjataan vanhin lasku, joka täsmää senttimäärään ja joka vielä maksamatta

        QSqlQuery kysely( QString("SELECT id, tili, selite, kohdennus FROM vienti WHERE iban='%1' AND viite='%2' ORDER BY pvm")
                          .arg(iban).arg(viite) );
        while( kysely.next())
        {
            int eraId = kysely.value("id").toInt();
            TaseEra era( eraId );

            if( era.saldoSnt == sentit )
            {
                vastarivi.tili = kp()->tilit()->tiliIdlla( kysely.value("tili").toInt() );
                vastarivi.eraId = era.eraId;
                selite = kysely.value("selite").toString();

                // #123: Kohdennusten sijoittaminen
                if( vastarivi.tili.json()->luku("Kohdennukset"))
                    vastarivi.kohdennus = kp()->kohdennukset()->kohdennus( kysely.value("kohdennus").toInt());

                break;
            }
        }
    }

    VientiRivi rivi;
    rivi.pvm = pvm;
    rivi.tili = tiliotetili();
    rivi.selite = selite;
    vastarivi.selite = selite;

    if( rivi.tili.json()->luku("Kohdennukset") && vastarivi.kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
        rivi.kohdennus = vastarivi.kohdennus;

    if( sentit > 0)
    {
        rivi.debetSnt = sentit;
        vastarivi.kreditSnt = sentit;
    }
    else
    {
        rivi.kreditSnt = 0 - sentit;
        vastarivi.debetSnt = 0 - sentit;
    }

    rivi.arkistotunnus = arkistotunnus;

    EhdotusModel ehdotus;
    ehdotus.lisaaVienti(rivi);
    ehdotus.lisaaVienti(vastarivi);
    ehdotus.viimeisteleMaksuperusteinen();
    ehdotus.tallenna( kirjausWg()->model()->vientiModel() );

}
