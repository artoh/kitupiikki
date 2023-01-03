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

#include <QRegularExpression>

#include <iostream>

namespace Tuonti {


OteRivi::OteRivi()
{

}

void OteRivi::setEuro(const Euro &euro)
{
    euro_ = euro;
}

void OteRivi::setArkistotunnus(QString arkistotunnus)
{
    if( arkistotunnusTyhjennyt_)
        return;    

    if( arkistotunnus.contains("REGISTR")) return;

    Iban iban(arkistotunnus);
    if(!arkistotunnus_.isEmpty() && iban.isValid()) {
        iban_ = iban;
        return;
    }
    if( !arkistotunnus_.isEmpty() && arkistotunnus.contains("-")) {
        return;         // Vanhanmallinen tilinumero
    }

    if( arkistotunnus.isEmpty()) {
        arkistotunnusTyhjennyt_ = true;
        return;
    }

    for( const auto& ohitettava : ohitettavat__) {
        if( arkistotunnus.toUpper() == ohitettava) {
            arkistotunnusTyhjennyt_ = true;
            return;
        }
    }

    arkistotunnus_.append(arkistotunnus.remove(QRegularExpression("\\s")));    

}

void OteRivi::setSaajaMaksaja(const QString &nimi)
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

    if( saajamaksaja_.isEmpty())
        saajamaksaja_ = nimi;
    else
        lisaaYleinen(nimi);
}

void OteRivi::setSelite(const QString &selite)
{
    int kto = ktoKoodi(selite);
    if( !kto_ && kto)
        setKTO(kto);
    else
        lisaaYleinen(selite);

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
            arkistotunnus_.contains(QRegularExpression("\\d"));
}

void OteRivi::lisaaYleinen(const QString &teksti)
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

    if( iso.contains("MAKSAJAN")) {
        tila = MAKSAJANVIITE;
    } else if( iso.contains("VIESTI") || iso.contains("MAKSUN TIEDOT")) {
        tila = VIESTI;
    } else if( iso.contains("OSTOPVM") && ostopvm_.isNull()) {
        lisaaOstoPvm(iso);
    } else if( iso.contains("ARKISTOINTITUNNUS") && arkistotunnus_.isEmpty()) {
        setArkistotunnus( iso.mid(18) );
    } else if( iso.contains("ARKISTOVIITE") && arkistotunnus_.isEmpty()) {
        setArkistotunnus( iso.mid(iso.indexOf(":")) );
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
            setSaajaMaksaja(teksti);
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
            viesti_ = teksti;
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
    QRegularExpressionMatch mats = QRegularExpression("(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})[.](?<vvvv>\\d{4})").match(str);
    if( mats.hasMatch() ) {
        return QDate(mats.captured("vvvv").toInt(),
                     mats.captured("kk").toInt(),
                     mats.captured("pp").toInt());
    }

    mats = QRegularExpression("(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})[.](?<vv>\\d{4})").match(str);
    if( mats.hasMatch() ) {
        return QDate(2000 + mats.captured("vv").toInt(),
                     mats.captured("kk").toInt(),
                     mats.captured("pp").toInt());
    }

    QDate pvm;
    mats = QRegularExpression("(?<pp>\\d{1,2})[.](?<kk>\\d{1,2})").match(str);
    if( !mats.hasMatch()) mats = QRegularExpression("(?<pp>\\d{1,2})(?<kk>\\d{1,2})").match(str);
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
    ostopvm_ = QDate();
    arkistotunnusTyhjennyt_ = false;
}

QVariantMap OteRivi::map(const QDate &kirjauspvm) const
{
    if( !pvm_.isValid() && !kirjauspvm.isValid())
        return QVariantMap();

    QVariantMap map;
    map.insert("euro", euro_.toString());
    map.insert("arkistotunnus", arkistotunnus_);

    if(!saajamaksaja_.isEmpty())
        map.insert("saajamaksaja", saajamaksaja_);
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
        map.insert("selite", viesti_);
    if( ostopvm_.isValid())
        map.insert("ostopvm", ostopvm_);

    return map;

}

void OteRivi::lisaaOstoPvm(const QString &teksti)
{

    QRegularExpression pisteRe("OSTOPVM\\s*(?<pp>\\d{2})[.](?<kk>\\d{2})[.](?<vvvv>\\d{4})");
    QRegularExpressionMatch pisteMatch = pisteRe.match(teksti);
    QRegularExpression ilmanRe("OSTOPVM\\s*(?<vv>\\d{2})(?<kk>\\d{2})(?<pp>\\d{2})");
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
     "SWEDIFIHH", "VPAYFIH2", "MAKSUPÄIVÄ","****","SEPA-MAKSU","ARKISTOINTITUNNUS","IBAN","BIC"};

}
