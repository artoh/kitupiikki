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

#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "raportoija.h"
#include "raporttirivi.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include <QJsonDocument>

Raportoija::Raportoija(const QString &raportinNimi, const QString &kieli, QObject *parent) :
    Raportteri (parent),
    kieli_(kieli),    
    tyyppi_ ( VIRHEELLINEN )
{
    if( raportinNimi.startsWith("tase/"))
        tyyppi_ = TASE;
    else if( raportinNimi.startsWith("tulos/"))
        tyyppi_ = TULOSLASKELMA;

    QString kaava = kp()->asetukset()->asetus(raportinNimi);
    QJsonDocument doc = QJsonDocument::fromJson( kaava.toUtf8() );
    kmap_ = doc.toVariant().toMap();

    rk.asetaOtsikko( kmap_.value("nimi").toMap().value(kieli).toString() );


}

Raportoija::~Raportoija()
{

}

void Raportoija::lisaaKausi(const QDate &alkaa, const QDate &paattyy, int tyyppi)
{
    alkuPaivat_.append(alkaa);
    loppuPaivat_.append(paattyy);
    sarakeTyypit_.append(tyyppi);
}

void Raportoija::lisaaTasepaiva(const QDate &pvm)
{
    loppuPaivat_.append(pvm);
    sarakeTyypit_.append(TOTEUTUNUT);
}



void Raportoija::dataSaapuu(int sarake, QVariant *variant)
{
    QVariantMap map = variant->toMap();

    qDebug() << " -- Sarakkeelle " << sarake << "  "  << map.count() << "  tiliä ";

    QMapIterator<QString,QVariant> iter(map);
    while( iter.hasNext()) {
        iter.next();
        int tili = iter.key().toInt();
        qlonglong sentit = qRound( iter.value().toDouble() * 100 );

        if( !snt_.contains(tili) )
            snt_.insert(tili, QVector<qlonglong>(  sarakemaara_ ) );

        snt_[tili][sarake] = sentit;
    }

    tilausLaskuri_--;

    // Jos ollaan nollassa, niin sitten päästään kirjoittamaan ;)
    if( !tilausLaskuri_ && !snt_.isEmpty())
    {
        dataSaapunut();
        kirjoitaDatasta();
        emit valmis( rk );
    }
}

void Raportoija::dataSaapunut()
{
    QList<int> tilinumerot = snt_.keys();
    tilit_.reserve( tilinumerot.count() + 1 );
    tilit_.append("0"); // Aloitusindeksi
    for(int tilinumero : tilinumerot) {
        tilit_.append( QString::number(tilinumero) );
    }
    tilit_.sort();

    qDebug() << tilit_;
}


void Raportoija::kirjoitaYlatunnisteet()
{

    // Jos raportissa erikoissarakkeita "budjetti", "budjettiero", "budjettiero%", niin niille oma rivi
    bool erikoissarakkeita = false;
    for(int i=0; i < sarakeTyypit_.count(); i++)
        if( sarakeTyypit_.value(i) != TOTEUTUNUT)
            erikoissarakkeita = true;

    rk.lisaaVenyvaSarake();
    for( int i=0; i < loppuPaivat_.count(); i++)
        rk.lisaaEurosarake();

    // CSV-kausiraportissa kuitenkin kaikki yhdelle riville
    if( onkoKausiraportti() )
    {
        RaporttiRivi csvrivi(RaporttiRivi::CSV);
        csvrivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
        {
            QString tyyppiteksti = erikoissarakkeita ? sarakeTyyppiTeksti(i) : QString();
            csvrivi.lisaa( QString("%1 - %2 %3").arg( alkuPaivat_.at(i).toString("dd.MM.yyyy"))
                                              .arg( loppuPaivat_.at(i).toString("dd.MM.yyyy"))
                                              .arg( tyyppiteksti ), 1, true );
        }

        rk.lisaaOtsake(csvrivi);

        RaporttiRivi orivi(RaporttiRivi::EICSV);
        orivi.lisaa("");
        for(int i=0; i < alkuPaivat_.count(); i++)
            orivi.lisaa( QString("%1 -").arg( alkuPaivat_.at(i).toString("dd.MM.yyyy") ), 1, true );
        rk.lisaaOtsake(orivi);

    }
    // Tasepäivät tai loppupäivät
    RaporttiRivi olrivi(RaporttiRivi::EICSV);
    olrivi.lisaa("");
    for(int i=0; i < loppuPaivat_.count(); i++)
        olrivi.lisaa( loppuPaivat_.at(i).toString("dd.MM.yyyy"), 1, true );
    rk.lisaaOtsake(olrivi);


    if( erikoissarakkeita )
    {
        RaporttiRivi tyyppirivi(RaporttiRivi::EICSV);
        tyyppirivi.lisaa("");
        for(int i=0; i < sarakeTyypit_.count(); i++)
            tyyppirivi.lisaa( sarakeTyyppiTeksti(i), 1, true );
        rk.lisaaOtsake( tyyppirivi);
    }

}

void Raportoija::kirjoitaDatasta()
{

    QRegularExpression tiliRe("(?<alku>\\d{1,8})(\\.\\.)?(?<loppu>\\d{0,8})");

    // Välisummien käsittelyä = varten
    QVector<qlonglong> kokosumma( sarakemaara_ );


    QVariantList rivilista = kmap_.value("rivit").toList();

    for(QVariant riviVariant : rivilista)
    {
        QVariantMap map = riviVariant.toMap();

        QString kaava = map.value("L").toString();
        QString teksti = map.value(kieli_).toString();

        for(int i=0; i < map.value("V").toInt(); i++)
            rk.lisaaTyhjaRivi();

        qDebug() << "*" << kaava << "  |  " << teksti;

        RaporttiRivi rr;

        if( kaava.isEmpty() )
        {
            // Jos pelkkää tekstiä, niin se on sitten otsikko
            rr.lisaa( teksti );
            rk.lisaaRivi(rr);
            continue;
        }

        QString loppurivi = kaava;     // Aloittava tyhjä mukaan!
        int sisennys = map.value("S").toInt() * 4;

        RivinTyyppi rivityyppi = SUMMA;
        bool naytaTyhjarivi = kaava.contains('S') || kaava.contains('h');
        bool laskevalisummaan = !kaava.contains("==");
        bool lisaavalisumma = kaava.contains("=") && !kaava.contains("==");
        bool naytaErittely = kaava.contains('*');
        bool vainmenot = kaava.contains("-");
        bool vaintulot = kaava.contains('+');

        int erittelySisennys = 4;

        if( map.value("M").toString().contains("bold"))
            rr.lihavoi();
        if( kaava.contains("h"))
            rivityyppi = OTSIKKO;

        // Sisennys paikoilleen!
        QString sisennysStr;
        for( int i=0; i < sisennys; i++)
            sisennysStr.append(' ');

        rr.lisaa( sisennysStr + teksti );   // Lisätään teksti


        QList<int> rivinTilit;      // Tilit, jotka kuuluvat tälle välille

        QRegularExpressionMatchIterator ri = tiliRe.globalMatch(loppurivi );

        while( ri.hasNext())
        {
            QRegularExpressionMatch tiliMats = ri.next();
            QString alku = tiliMats.captured("alku");
            QString loppu = tiliMats.captured("loppu");

            // Sitten etsitään tilit listalle
            int alkumerkit = alku.length();

            int alaraja = 1;
            int ylaraja = tilit_.length() - 1;
            int indeksi = 1 + (ylaraja-1) / 2;

            while( indeksi && ylaraja != alaraja) {


                if( tilit_[indeksi-1].left(alkumerkit) < alku && tilit_[indeksi].left(alkumerkit) >= alku )
                    break;
                // Nyt indeksissä on ensimmäinen sopiva tilinumero
                if( tilit_[indeksi].left(alkumerkit) >= alku)
                    ylaraja = indeksi;
                else
                    alaraja = indeksi+1;

                indeksi = alaraja + (ylaraja - alaraja) / 2;
            }

            if( loppu.isEmpty() )
            {
                // Haetaan tilejä tietyllä tilinumeron alulla
                while( indeksi < tilit_.length() &&  tilit_[indeksi].startsWith(alku))
                {
                    rivinTilit.append( tilit_[indeksi].toInt() );
                    indeksi++;
                }
            } else {
                while( indeksi < tilit_.length() && tilit_[indeksi].left(alkumerkit) <= loppu) {
                    rivinTilit.append( tilit_[indeksi].toInt() );
                    indeksi++;
                }
            }

        }

        // Lasketaan summiin
        QVector<qlonglong> summa( sarakemaara_);

        for(int tili : rivinTilit)
        {
            if( vainmenot || vaintulot) {
                const Tili& tamaTili = kp()->tilit()->tiliNumerolla(tili);
                if( (vainmenot && !tamaTili.onko(TiliLaji::MENO))  ||
                    (vaintulot && !tamaTili.onko(TiliLaji::TULO)) )
                    continue;
            }
            for(int i=0; i < sarakemaara_; i++)
                summa[i] += snt_.value(tili,QVector<qlonglong>(sarakemaara_))[i];
        }

        if( laskevalisummaan )  {
            for(int i=0; i < sarakemaara_; i++)
                kokosumma[i] += summa[i];
        }

        if( lisaavalisumma )
        {
            for(int i=0; i < sarakemaara_; i++)
                summa[i] += kokosumma[i];
        }

        bool kirjauksia = false;
        for( int i=0; i < sarakemaara_; i++) {
            if( summa[i]) {
                kirjauksia = true;
                break;
            }
        }

         if( !naytaTyhjarivi && !kirjauksia )
            continue;

        // header tulostaa vain otsikon
        if( rivityyppi != OTSIKKO  )
        {

            int taulukkoindeksi = 0;
            // Sitten kirjoitetaan summat riville
            for( int sarake=0; sarake < loppuPaivat_.count(); sarake++)
            {
                // Since 1.1: Saraketyypin mukaisesti


                switch (sarakeTyypit_.at(sarake)) {

                case TOTEUTUNUT :
                    rr.lisaa( summa[taulukkoindeksi], true);
                    break;
                case BUDJETTI:
                    rr.lisaa( summa[taulukkoindeksi], false);
                    break;
                case BUDJETTIERO:                    
                    rr.lisaa( summa[taulukkoindeksi] - summa[taulukkoindeksi+1], true );
                    taulukkoindeksi++;
                    break;
                case TOTEUMAPROSENTTI:
                    if( !summa[taulukkoindeksi+1] )
                        rr.lisaa("");
                    else
                        rr.lisaa( 10000 * summa[taulukkoindeksi] / summa[taulukkoindeksi+1], true );
                    taulukkoindeksi++;
                }
                taulukkoindeksi++;
            }
        }

        rk.lisaaRivi(rr);

        if( naytaErittely && erittelyt_ )
        {
            // eriSisennysStr on erittelyrivin aloitussisennys, joka *-rivillä kasvaa edellisen rivin sisennyksestä
            QString eriSisennysStr = sisennysStr;
            for( int i=0; i < erittelySisennys; i++)
                eriSisennysStr.append(' ');

            for( int tiliNumero : rivinTilit) {

                if( vainmenot || vaintulot) {
                    const Tili& tamaTili = kp()->tilit()->tiliNumerolla(tiliNumero);
                    if( (vainmenot && !tamaTili.onko(TiliLaji::MENO))  ||
                        (vaintulot && !tamaTili.onko(TiliLaji::TULO)) )
                        continue;
                }

                RaporttiRivi er;
                Tili* tili = kp()->tilit()->tili(tiliNumero);
                if( !tili )
                    continue;

                er.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(),
                                  eriSisennysStr + tili->nimiNumero( kieli_ ));


                int taulukkoindeksi = 0;
                // Sitten kirjoitetaan summat riville
                for( int sarake=0; sarake < loppuPaivat_.count(); sarake++)
                {
                    switch (sarakeTyypit_.at(sarake)) {

                    case TOTEUTUNUT:
                        er.lisaa( snt_.value(tiliNumero).value(taulukkoindeksi), true );
                        break;
                    case BUDJETTI :
                        er.lisaa( snt_.value(tiliNumero).value(taulukkoindeksi), false );
                        break;
                    case BUDJETTIERO:
                        er.lisaa( snt_.value(tiliNumero).value(taulukkoindeksi) - snt_.value(tiliNumero).value(taulukkoindeksi+1), true );
                        taulukkoindeksi++;
                        break;
                    case TOTEUMAPROSENTTI:
                        if( !snt_.value(tiliNumero).value(taulukkoindeksi+1) )
                            er.lisaa("");
                        else
                            er.lisaa( 10000 * snt_.value(tiliNumero).value(taulukkoindeksi) / snt_.value(tiliNumero).value(taulukkoindeksi+1), true );
                        taulukkoindeksi++;
                    }
                    taulukkoindeksi++;
                }

                rk.lisaaRivi(er);
            }
        }

    }
}



QString Raportoija::sarakeTyyppiTeksti(int sarake)
{
    switch (sarakeTyypit_.value(sarake))
    {
        case TOTEUTUNUT:
            return tr("Toteutunut");
        case BUDJETTI:
            return tr("Budjetti");
        case BUDJETTIERO:
            return tr("Budjettiero €");
        case TOTEUMAPROSENTTI:
            return tr("Toteutunut %");
    }
    return  QString();
}


void Raportoija::kirjoita(bool tulostaErittelyt, int kohdennuksella)
{
    erittelyt_ = tulostaErittelyt;
    kirjoitaYlatunnisteet();

    if( kohdennuksella > -1)
        rk.asetaOtsikko( rk.otsikko() + " (" + kp()->kohdennukset()->kohdennus(kohdennuksella).nimi(kieli_) + ")" );

    QList<KpKysely*> kyselyt;
    // Sitten tilataan tarvittava data
    if( tyyppi() == TASE ) {
        for(int i=0; i < loppuPaivat_.count(); i++) {
            tilausLaskuri_++;
            KpKysely* kysely = kpk("/saldot");
            kysely->lisaaAttribuutti("pvm",loppuPaivat_.at(i));
            kysely->lisaaAttribuutti("tase");
            int sarake = ++sarakemaara_ -1;
            connect(kysely, &KpKysely::vastaus,
                    [this,sarake] (QVariant* vastaus) { this->dataSaapuu(sarake, vastaus); });
            kyselyt.append(kysely);
        }
    } else if( tyyppi() == TULOSLASKELMA) {
        for( int i=0; i < loppuPaivat_.count(); i++) {

            if( sarakeTyypit_[i] != BUDJETTI)
            {
                tilausLaskuri_++;
                KpKysely* kysely = kpk("/saldot");
                kysely->lisaaAttribuutti("alkupvm", alkuPaivat_.value(i));
                kysely->lisaaAttribuutti("pvm", loppuPaivat_.value(i));
                kysely->lisaaAttribuutti("tuloslaskelma");
                if( kohdennuksella > -1)
                    kysely->lisaaAttribuutti("kohdennus", kohdennuksella);
                int sarake = ++sarakemaara_ -1;
                connect(kysely, &KpKysely::vastaus,
                        [this,sarake] (QVariant* vastaus) { this->dataSaapuu(sarake, vastaus); });
                kyselyt.append(kysely);
            }
            if( sarakeTyypit_[i] != TOTEUTUNUT)
            {
                tilausLaskuri_++;
                KpKysely* kysely = kpk( QString("/budjetti/%1").arg( loppuPaivat_.value(i).toString(Qt::ISODate) ));
                if( kohdennuksella > -1)
                    kysely->lisaaAttribuutti("kohdennus", kohdennuksella);
                int sarake = ++sarakemaara_ -1;
                connect(kysely, &KpKysely::vastaus,
                        [this,sarake] (QVariant* vastaus) { this->dataSaapuu(sarake, vastaus); });
                kyselyt.append(kysely);
            }
        }
    }
    for(auto kysely : kyselyt)
        kysely->kysy();

    qDebug() << "Lähetetty " << kyselyt.count() << "kyselyä ";
}

QString Raportoija::nimi() const
{
    return kmap_.value("nimi").toMap().value(kieli_).toString();
}
