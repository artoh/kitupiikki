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
#include "oterivi.h"
#include "validator/viitevalidator.h"
#include "validator/ibanvalidator.h"

#include "tuonti/tuontiapu.h"

#include <QRegularExpression>

#include <iostream>

namespace Tuonti {


OteRivi::OteRivi()
{

}

void OteRivi::kasittele(const QString &teksti, TilioteOtsake::Tyyppi tyyppi, int rivi, const QDate &loppupvm)
{

//    qDebug() << "  " << TilioteOtsake::tyyppiTeksti(tyyppi) << " " << teksti;

    // Jos IBAN niin IBAN!
    QRegularExpressionMatch ibanMats = ibanRe__.match(teksti);
    if( ibanMats.hasMatch() && IbanValidator::kelpaako(ibanMats.captured())) {
        iban_ = ibanMats.captured();
        return;
    }

    switch (tyyppi) {
    case TilioteOtsake::ARKISTOTUNNUS:
        setArkistotunnus(teksti, rivi);
        break;
    case TilioteOtsake::SAAJAMAKSAJA:
        setSaajaMaksaja(teksti, rivi);
        break;
    case TilioteOtsake::SELITE:
        setSelite(teksti, rivi);
        break;
    case TilioteOtsake::PVM:
        setPvm(teksti, loppupvm);
        break;
    case TilioteOtsake::VIITE:
        setViite(teksti);
        break;
    case TilioteOtsake::EURO:
        setEuro(teksti);
        break;
    case TilioteOtsake::YLEINEN:
        lisaaYleinen(teksti, rivi);
        break;
    case TilioteOtsake::TUNTEMATON:
        kasitteleTuntematon(teksti, rivi, loppupvm);
        break;
    default:
        break;
    }
}

void OteRivi::setEuro(const QString maara)
{
    if( !euro_) {
        qlonglong sentit = TuontiApu::sentteina(maara);
        euro_ = Euro(sentit);
    }
}

void OteRivi::setArkistotunnus(QString arkistotunnus, int rivi)
{
    if(arkistoTunnusRivi_ && rivi > arkistoTunnusRivi_ + 1) return;
    if( arkistotunnus.contains("REGISTR")) return;

    Iban iban(arkistotunnus);
    if(!arkistotunnus_.isEmpty() && iban.isValid()) {
        iban_ = iban;
        return;
    }
    if( !arkistotunnus_.isEmpty() && arkistotunnus.contains("-")) {
        return;         // Vanhanmallinen tilinumero
    }


    for( const auto& ohitettava : ohitettavat__) {
        if( arkistotunnus.toUpper() == ohitettava) {
            return;
        }
    }

    arkistotunnus_.append(arkistotunnus.remove(QRegularExpression("\\s")));    
    arkistoTunnusRivi_ = rivi;

}

void OteRivi::setSaajaMaksaja(const QString &nimi, int rivi)
{
    if( nimi.length() < 6 || !nimi.contains(QRegularExpression("[A-z]"))) {
        if( ViiteValidator::kelpaako(nimi) && viite_.isEmpty() && viesti_.isEmpty() ) {
            setViite(nimi);
        }
        return;
    }

    QString iso = nimi.toUpper();
    for( const auto& ohitettava : ohitettavat__) {
        if( iso.contains(ohitettava)) {
            return;
        }
    }

    if( saajaMaksajaRivi_ == rivi) {
        saajamaksaja_.append(" " + nimi);
    } else if( saajamaksaja_.isEmpty()) {
        saajamaksaja_ = nimi;
        saajaMaksajaRivi_ = rivi;
    } else
        lisaaYleinen(nimi, rivi);
}

void OteRivi::setSelite(const QString &selite, int rivi)
{
    int kto = ktoKoodi(selite);
    if( !kto_ && kto)
        setKTO(kto);
    else
        lisaaYleinen(selite, rivi);

}

void OteRivi::setViite(QString viite)
{
    viite.remove(QRegularExpression("^0+"));
    viite.remove(QRegularExpression("\\s"));
    if( ViiteValidator::kelpaako(viite))
        viite_ = viite;
}

void OteRivi::setIban(const Iban &iban)
{
    if( iban.isValid()) {
        iban_ = iban;
    }

}

void OteRivi::setPvm(const QString &str, const QDate &loppupvm)
{
    QDate pvm = strPvm(str, loppupvm);
    if(pvm.isValid())
        pvm_ = pvm;
}



bool OteRivi::valmis() const
{
    return euro_ && arkistotunnus_.length() > 6 &&
            arkistotunnus_.contains(QRegularExpression(nroRe__));
}

void OteRivi::kasitteleTuntematon(const QString &teksti, int rivi, const QDate& loppupvm)
{
    if( !rivi && !pvm_.isValid()) {
        const QDate paiva = strPvm(teksti, loppupvm);
        if( paiva.isValid()) {
            pvm_ = paiva;
            return;
        }
    }
    if( teksti.length() > 10) {
        lisaaYleinen(teksti, rivi);
    }

}

void OteRivi::lisaaYleinen(const QString &teksti, int rivi)
{


    QString iso = teksti.toUpper();    
    for( const auto& ohitettava : ohitettavat__) {
        if( iso.contains(ohitettava)) {
            return;
        }
    }

    // Päivämäärä ei kuulu tähän sarakkeeseen
    if( saajamaksaja_.isEmpty() && iso.length() > 3 && iso.length() < 11 && strPvm(iso, QDate::currentDate()).isValid())
        return;    

    if( rivi == saajaMaksajaRivi_) {
        setSaajaMaksaja(teksti, rivi);
    } else if( iso.contains("MAKSAJAN")) {
        tila = MAKSAJANVIITE;
    } else if( iso.contains("VIESTI") || iso.contains("MAKSUN TIEDOT")) {
        tila = VIESTI;
    } else if( iso == "OSTOPVM") {
        tila = OSTOPVM;
    } else if( tila == OSTOPVM) {
        lisaaOstoPvm(iso);
        tila = NORMAALI;
    } else if( iso.contains("OSTOPVM") && ostopvm_.isNull()) {
        lisaaOstoPvm(iso);
    } else if( iso.contains("ARKISTOINTITUNNUS") && arkistotunnus_.isEmpty()) {
        setArkistotunnus( iso.mid(18), rivi );
    } else if( iso.contains("ARKISTOVIITE") && arkistotunnus_.isEmpty()) {
        setArkistotunnus( iso.mid(iso.indexOf(":")), rivi );
    } else if( Iban(teksti).isValid() ) {
        if( iban_.isEmpty() )
            iban_ = teksti;
    } else if( tila == OHITALOPPUUN) {
        ;
    } else if( tila == VIESTI) {
        if( !viesti_.isEmpty())
            viesti_.append(" ");
        viesti_.append(teksti);
    } else if( !kto_ && ktoKoodi(teksti)) {
        kto_ = ktoKoodi(teksti);
    } else if( saajamaksaja_.isEmpty()) {
        if( iso.contains("PALVELUMAKSU") || iso.contains("SERVICEAVGIFT")) {
            viesti_ = teksti;
            tila = OHITALOPPUUN;
        } else {
            setSaajaMaksaja(teksti, rivi);
        }
    } else {
        QRegularExpression viiteRe("(RF\\d{2}\\s?)?\\d+(\\s\\d+)*");
        QRegularExpressionMatch match = viiteRe.match(teksti);

        if( match.hasMatch()) {
            QString ehdokas = match.captured();
            if( viite_.isEmpty() && ViiteValidator::kelpaako(ehdokas) && kto_ != 721) {
                setViite(ehdokas);
                return;
            }
        } else if(viesti_.isEmpty() && kto_ &&
                  !saajamaksaja_.startsWith(teksti)) {
            viesti_.append(teksti);
        }
    }
}

int OteRivi::ktoKoodi(const QString &teksti)
{
    QRegularExpression ktoRe("(7[0-8][0-6])\\s\\w+");
    QRegularExpressionMatch mats = ktoRe.match(teksti);
    if( mats.hasMatch())
        return mats.captured(1).toInt();

    if( teksti.contains("TILISIIRTO") ||
        teksti.contains("VIITESIIRTO") ||
        teksti.contains("MAKSUPALVELU") ||
        teksti.contains("ULKOM MAKSU"))
        return 700;
    else if(teksti.contains("PANO") ||
            teksti.contains("TALLETUS"))
        return 710;
    else if(teksti.contains("OTTO")
            || teksti.contains("NOSTO"))
        return 720;
    else if(teksti.contains("KORTTIOSTO") ||
            teksti.contains("KORTTIMAKSU"))
        return 721;
    else if(teksti.contains("PALVELUMAKSU"))
        return 730;
    else if(teksti.contains("KORKOHYVITYS"))
        return 750;
    return 0;
}

QDate OteRivi::strPvm(const QString &str, const QDate &loppupvm)
{
    QRegularExpressionMatch mats = QRegularExpression("\\b(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})[.](?<vvvv>\\d{4})\\b").match(str);
    if( mats.hasMatch() ) {        
        return QDate(mats.captured("vvvv").toInt(),
                     mats.captured("kk").toInt(),
                     mats.captured("pp").toInt());
    }

    mats = QRegularExpression("\\b(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})[.](?<vv>\\d{2})\\b").match(str);
    if( mats.hasMatch() ) {
        return QDate(2000 + mats.captured("vv").toInt(),
                     mats.captured("kk").toInt(),
                     mats.captured("pp").toInt());
    }

    QDate pvm;
    mats = QRegularExpression("\\b(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})\\b").match(str);
    if( !mats.hasMatch()) mats = QRegularExpression("\\b(?<pp>\\d{1,2})(?<kk>\\d{1,2})\\b").match(str);
    if( mats.hasMatch() ) {        
        pvm = QDate(loppupvm.year(),
                     mats.captured("kk").toInt(),
                     mats.captured("pp").toInt());
        if( pvm > loppupvm)
            pvm = pvm.addYears(-1);
        return pvm;
    }
    return QDate();
}

void OteRivi::tyhjenna()
{

    tila = NORMAALI;
    euro_ = 0;
    arkistotunnus_.clear();
    saajamaksaja_.clear();
    kto_ = 0;
    viite_.clear();
    iban_ = Iban();
    viesti_.clear();
    pvm_ = QDate();
    ostopvm_ = QDate();
    arkistoTunnusRivi_ = 0;
    saajaMaksajaRivi_ = -1;
}

QVariantMap OteRivi::map(const QDate &kirjauspvm) const
{
    if( !pvm_.isValid() && !kirjauspvm.isValid())
        return QVariantMap();

    QVariantMap map;
    map.insert("euro", euro_.toString());
    map.insert("arkistotunnus", arkistotunnus_.trimmed());

    if(!saajamaksaja_.isEmpty())
        map.insert("saajamaksaja", saajamaksaja_.trimmed());
    if(kto_)
        map.insert("ktokoodi", kto_);
    if(!viite_.isEmpty())
        map.insert("viite", viite_);
    if(iban_.isValid())
        map.insert("iban", iban_.valeitta());


    if( kirjauspvm.isValid())
        map.insert("pvm", kirjauspvm);
    else if(pvm_.isValid())
        map.insert("pvm", pvm_);

    if( !viesti_.isEmpty())
        map.insert("selite", viesti_.trimmed());
    if( ostopvm_.isValid())
        map.insert("ostopvm", ostopvm_);

    return map;

}

void OteRivi::lisaaOstoPvm(const QString &teksti)
{

    QRegularExpression pisteRe("(OSTOPVM\\s*)?(?<pp>\\d{2})[.](?<kk>\\d{2})[.](?<vvvv>\\d{4})");
    QRegularExpressionMatch pisteMatch = pisteRe.match(teksti);
    QRegularExpression ilmanRe("(OSTOPVM\\s*)?(?<vv>\\d{2})(?<kk>\\d{2})(?<pp>\\d{2})");
    QRegularExpressionMatch ilmanMatch = ilmanRe.match(teksti);

    if( pisteMatch.hasMatch()) {
        ostopvm_ = QDate( pisteMatch.captured("vvvv").toInt(),
                          pisteMatch.captured("kk").toInt(),
                          pisteMatch.captured("pp").toInt());
    } else if(  ilmanMatch.hasMatch()) {
        ostopvm_ = QDate( 2000 + ilmanMatch.captured("vv").toInt(),
                          ilmanMatch.captured("kk").toInt(),
                          ilmanMatch.captured("pp").toInt());
    }

}

std::vector<QString> OteRivi::ohitettavat__ =
    {"NOTPROVIDED", "VARMENTAJA", "MF NRO",
     "HELSFIHH", "DABAFIHH", "DABAFIHX", "HANDFIHH", "NDEAFIHH",
     "OKOYFIHH", "SBANFIHH", "AABAFI22", "POPFI22", "ITELFIHH",
     "BIGKFIH1", "CITIFIHX", "DNBAFIHX", "HOLVFIHH", "ESSEFIHX",
     "SWEDIFIHH", "VPAYFIH2", "MAKSUPÄIVÄ","****","SEPA-MAKSU","IBAN","BIC"};



QRegularExpression OteRivi::ibanRe__("\\b[A-Z]{2}\\d{2}\\s?(\\w{4}\\s?){3,6}\\w{1,4}\\b");
QRegularExpression OteRivi::nroRe__("\\d+");
}
