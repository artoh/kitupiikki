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
#include "ilmoitintuottaja.h"
#include <QDateTime>
#include <QApplication>
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QRegularExpression>

IlmoitinTuottaja::IlmoitinTuottaja(QObject *parent) : QObject(parent)
{

}

void IlmoitinTuottaja::tallennaAineisto(int ilmoitusId)
{
    KpKysely* kysely = kpk(QString("/tositteet/%1").arg(ilmoitusId));
    connect(kysely, &KpKysely::vastaus, this, &IlmoitinTuottaja::tositeSaapuu);
    kysely->kysy();
}

bool IlmoitinTuottaja::voikoMuodostaa(const QVariantMap &map)
{
    // Ilmoittimen voi muodostaa vain, jos on tilaaja sekä
    // kausi on normaali
    return !kausiTieto(map).isEmpty() &&
            kp()->pilvi() &&
            !kp()->pilvi()->ilmoitinTunnus().isEmpty() &&
            !kp()->asetukset()->asetus(AsetusModel::Ytunnus).isEmpty() &&
            ( kp()->pilvi()->tilausvoimassa() ||
              qobject_cast<PilviModel*>(kp()->yhteysModel()));
}

QVariantList IlmoitinTuottaja::kausiTieto(const QVariantMap &map)
{
    QDate alkaa = map.value("kausialkaa").toDate();
    QDate loppuu = map.value("kausipaattyy").toDate();

    if( alkaa.year() != loppuu.year() ||
        alkaa.day() != 1 ||
        loppuu.day() != loppuu.daysInMonth())
        return QVariantList();

    // Vuosi
    if(alkaa.month() == 1 && loppuu.month() == 12) {
        return QVariantList() << "V" << alkaa.year();
    }
    // Kuukausi
    else if( alkaa.month() == loppuu.month()  ) {
        return QVariantList() << "K" << alkaa.year() << alkaa.month();
    }
    // Mahdolliset vuosineljännekset
    //  1,2,3 4,5,6 7,8,9 10,11,12
    else if( alkaa.month()%3==1 and loppuu.month()%3==0)
        return QVariantList() << "Q" << alkaa.year() << loppuu.month() / 3;

    return QVariantList();
}

void IlmoitinTuottaja::tositeSaapuu(QVariant *data)
{
    QVariantMap alv = data->toMap().value("alv").toMap();
    if( muodosta(alv)) {
        QSettings settings;
        QString hakemisto = settings.value("IlmoitinHakemisto", QDir::homePath()).toString();
        QDir dir(hakemisto);

        QDate loppupvm = alv.value("kausipaattyy").toDate();
        QString tnimi = dir.absoluteFilePath(QString("ALV_%1_%2.txt")
                                             .arg(loppupvm.toString("yyyyMM"))
                                             .arg(kp()->asetukset()->nimi().remove(QRegularExpression("\\W"))));

        QString filename = QFileDialog::getSaveFileName(nullptr, tr("Tallenna Ilmoitin-aineisto"),
                                                        tnimi, tr("Tekstitiedostot (*.txt);;Kaikki tiedostot (*.*)"));
        if( filename.isEmpty())
            return;
        QFileInfo info(filename);
        settings.setValue("IlmoitinHakemisto", info.absoluteDir().dirName());

        QFile file(filename);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(nullptr, tr("Ilmoitinaineiston muodostaminen"),
                                  tr("Ilmoitinaineiston tallentaminen tiedostoon epäonnistui"));
            return;
        }
        QTextStream out(&file);
        out << txt_;
        file.close();
        QMessageBox::information(nullptr , tr("Ilmoitinaineiston muodostaminen"),
                                 tr("Ilmoitinainesto tallennettu tiedostoon %1.\n"
                                    "Voit lähettää sen verottajalle ilmoitin.fi-palvelulla.")
                                 .arg(filename));
    }
}

bool IlmoitinTuottaja::muodosta(const QVariantMap &data)
{
    txt_.clear();

    QVariantList kausitieto = kausiTieto(data);
    if( kausitieto.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Ilmoitinaineiston muodostaminen"),
                              tr("Ilmoitinaineiston muodostaminen epäonnistui"));
        return false;
    }

    lisaa(0, "VSRALVKV");
    lisaa(198, QDateTime::currentDateTime().toString("ddMMyyyyhhmmss"));
    lisaa(14, kp()->pilvi()->ilmoitinTunnus());
    lisaa(48, qApp->applicationName());
    lisaa(10, kp()->asetukset()->asetus(AsetusModel::Ytunnus));

    lisaa(50, kausitieto.value(0).toString());
    if( kausitieto.count() > 2)
        lisaa(52, kausitieto.value(2).toString());
    lisaa(53, kausitieto.value(1).toString());

    QVariantMap kooditMap = data.value("koodit").toMap();

    bool tyhja = true;

    QMapIterator<QString,QVariant> iter(kooditMap);
    while( iter.hasNext()) {
        iter.next();
        int koodi = iter.key().toInt();

        // Jos ei haeta alijäämähuojennusta, niin ei myöskään
        // ilmoitetan niihin oikeuttavia määriä

        if( koodi >= 301 && koodi <= 320)
            tyhja = false;

        if( (koodi >= 315 && !kooditMap.contains("317")) || koodi > 317)
            break;
        lisaa(koodi, iter.value().toLongLong());
    }

    if( kooditMap.contains("317")) {
        if( kausitieto.value(0).toString() == "Q")
            lisaa(336,"2");
        else if( kausitieto.value(0).toString() == "K")
            lisaa(336,"1");
    }
    if( tyhja )
        lisaa(56, "1");

    if( kooditMap.contains("337"))
        lisaa(337,"1");

    iter.toFront();
    while( iter.hasNext()) {
        iter.next();
        int koodi = iter.key().toInt();
        if( koodi < 318 || koodi > 320)
            continue;
        lisaa(koodi, iter.value().toLongLong());
    }
    if( kp()->asetukset()->onko(AsetusModel::VeroYhteysPuhelin) )
        lisaa(42, kp()->asetukset()->asetus(AsetusModel::VeroYhteysPuhelin));
    txt_.append("999:1");

    return true;
}



void IlmoitinTuottaja::lisaa(int koodi, const QString &arvo)
{
    txt_.append(QString("%1:%2\n")
            .arg(koodi, 3, 10, QChar('0'))
                .arg(arvo));
}

void IlmoitinTuottaja::lisaa(int koodi, qlonglong sentit)
{
    QString luku = QString::number( sentit / 100.0  ,'f',2);
    lisaa(koodi, luku.replace(QChar('.'),QChar(',')));
}

