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
#include "tesseracttuonti.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "validator/ytunnusvalidator.h"
#include "validator/ibanvalidator.h"
#include "validator/viitevalidator.h"

#include "tuontiapu.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"
#include "db/tositetyyppimodel.h"

#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

#include <QDebug>

Tuonti::TesserActTuonti::TesserActTuonti(QObject *parent)
    : QObject(parent)
{

}


void Tuonti::TesserActTuonti::tuo(const QByteArray &data)
{
    if( kp()->pilvi() &&
        !kp()->pilvi()->service("ocr").isEmpty())
    {
        QString osoite = kp()->pilvi()->service("ocr");

        PilviKysely *ocr = new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
        connect( ocr, &KpKysely::vastaus, this, &TesserActTuonti::kasittele);
        ocr->lahetaTiedosto(data);
    }
}

void Tuonti::TesserActTuonti::kasittele(QVariant *data)
{
    QString teksti = QString::fromUtf8(data->toByteArray());
    emit tuotu( analysoi(teksti));
    this->deleteLater();
}

QVariantMap Tuonti::TesserActTuonti::analysoi(const QString &teksti)
{
    // Hyvin Alkeellinen Analysaattori
    // Pitäisi tutkia: Toimitus- ja eräpäivä, maksutapa jne.

    QDate laskupvm;
    QDate erapvm;
    QVariantList ibanit;

    QVariantMap tulos;
    qlonglong senttia = 0;

    QRegularExpression ytunnusre( R"(\b\d{7}-\d\b)");
    QRegularExpression pvmre( R"(\b([0123]?\d)[.-]([01]?\d)[.-](20[012]\d)\b)");
    QRegularExpression varmaeurore(R"((yht|total|sum).*?[^.](\d{1,5}[.,]\d{2})\D*?$)",
                                   QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption
                                   | QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpression eurore(R"(\b(\d{1,5}[.,]\d{2})\W)");

    QRegularExpression viitere(R"((viite|ref).*?\b((RF\d\d)?(\d|\s)*\d\b))",
                               QRegularExpression::CaseInsensitiveOption);
    QRegularExpression ibanre(R"(FI\d{2}\s\d{4}\s\d{4}\s\d{4}\s\d{2})");


    QRegularExpressionMatchIterator ytunnusiter = ytunnusre.globalMatch(teksti);
    while( ytunnusiter.hasNext()) {
        QRegularExpressionMatch match = ytunnusiter.next();
        QString ytunnari = match.captured();

        if( ytunnari != kp()->asetukset()->ytunnus() &&
            YTunnusValidator::kelpaako(ytunnari))
        {
            tulos.insert("kumppaniytunnus", ytunnari);
        }
    }

    // Tässä pitäisi hakea erilaisia päivämääriä
    QRegularExpressionMatchIterator pvmiter = pvmre.globalMatch(teksti);
    while( pvmiter.hasNext()) {
        QRegularExpressionMatch match = pvmiter.next();
        QDate pvm( match.captured(3).toInt(),
                   match.captured(2).toInt(),
                   match.captured(1).toInt());

        if( pvm.isValid() ) {
            if( !laskupvm.isValid()) laskupvm = pvm;
            else if( pvm > erapvm ) erapvm = pvm;
            else if( pvm < laskupvm) {
                if( !erapvm.isValid() && pvm.daysTo(erapvm) > 7 ) erapvm = laskupvm;
                laskupvm = pvm;
            }
        }
    }

    QRegularExpressionMatchIterator varmaeuroiter = varmaeurore.globalMatch(teksti);
    while( varmaeuroiter.hasNext()) {
        QRegularExpressionMatch match = varmaeuroiter.next();
        qlonglong snt = TuontiApu::sentteina( match.captured(2) );
        if( snt > senttia)
            senttia = snt;
    }

    QRegularExpressionMatchIterator euroiter = eurore.globalMatch(teksti);
    if(!senttia) {
        while( euroiter.hasNext()) {
            QRegularExpressionMatch match = euroiter.next();
            qlonglong snt = TuontiApu::sentteina( match.captured(1) );
            if(snt && snt > senttia)
                senttia = snt;
        }
    }

    QRegularExpressionMatchIterator ibaniter = ibanre.globalMatch(teksti);
    while( ibaniter.hasNext()) {
        QRegularExpressionMatch match = ibaniter.next();
        QString iban = match.captured().remove(QRegularExpression("\\s"));

        if( IbanValidator::kelpaako(iban))
            ibanit.append(iban);
    }

    QRegularExpressionMatchIterator viiteiter = viitere.globalMatch(teksti);
    while( viiteiter.hasNext()) {
        QRegularExpressionMatch match = viiteiter.next();
        QString viite = match.captured(2).remove(QRegularExpression("\\s"));

        if( ViiteValidator::kelpaako(viite))
            tulos.insert("viite", viite);
    }

    if( laskupvm.isValid())
        tulos.insert("tositepvm", laskupvm);
    if( erapvm.isValid())
        tulos.insert("erapvm", erapvm);
    if( ibanit.count())
        tulos.insert("iban", ibanit);

    if( senttia )
        tulos.insert("summa", senttia / 100.0);

    if( teksti.contains("käteinen", Qt::CaseInsensitive))
        tulos.insert("maksutapa","kateinen");

    if( !tulos.isEmpty())
        tulos.insert("tyyppi", TositeTyyppi::MENO);

    return tulos;

}
