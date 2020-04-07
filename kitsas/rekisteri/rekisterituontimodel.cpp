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
#include "rekisterituontimodel.h"
#include "tuonti/csvtuonti.h"
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include "rekisteri/postinumerot.h"

RekisteriTuontiModel::RekisteriTuontiModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant RekisteriTuontiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if( section == MALLI)
            return tr("Sarake");
        else
            return tr("Tuonti");
    }
    return QVariant();
}


int RekisteriTuontiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return csv_.value(0).count();
}

int RekisteriTuontiModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant RekisteriTuontiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        if( index.column() == MALLI ) {
            return csv_.value(0).value(index.row());
        } else {
            return otsikkoTeksti(sarakkeet_.value(index.row()));
        }
    } else if( role == Qt::EditRole) {
        return sarakkeet_.value(index.row());
    }

    return QVariant();
}

bool RekisteriTuontiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        if( role == Qt::EditRole) {
            sarakkeet_[index.row()] = value.toInt();
            emit dataChanged(index, index, QVector<int>() << role);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags RekisteriTuontiModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if(index.column() == MUOTO)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled;
}

QString RekisteriTuontiModel::otsikkoTeksti(int sarake)
{
    switch (sarake) {
        case NIMI: return tr("Nimi");
        case LAHIOSOITE: return tr("Lähiosoite");
        case POSTINUMERO: return tr("Postinumero");
        case KAUPUNKI: return tr("Kaupunki");
        case POSTIOSOITE: return tr("Postiosoite");
        case SAHKOPOSTI: return tr("Sähköposti");
        case PUHELIN: return tr("Puhelin");
        case YTUNNUS: return tr("Y-tunnus");
        case VERKKOLASKUOSOITE: return tr("Verkkolaskuosoite");
        case VERKKOLASKUVALITTAJA: return tr("Verkkolaskuvälittäjä");
        case MAA: return tr("Maa");
        case KIELI: return tr("Kieli");
        case LISATIETO: return tr("Lisätieto");
    default: return QString();
    }
}

int RekisteriTuontiModel::lataaCsv(const QString &tiedostonnimi)
{
     QFile tiedosto(tiedostonnimi);
     if( !tiedosto.open(QIODevice::ReadOnly))
         return 0;

     beginResetModel();
     csv_ = Tuonti::CsvTuonti::csvListana(tiedosto.readAll());
     if( !csv_.isEmpty()) {
         sarakkeet_.resize(csv_.value(0).count());
         arvaaSarakkeet();
     }

     endResetModel();
     return csv_.count();
}

QVariantList RekisteriTuontiModel::lista() const
{
    QVariantList lista;
    for(int r=otsikkorivi_ ? 1 : 0 ; r < csv_.count(); r++) {
        QVariantMap map;
        QStringList rivi = csv_.value(r);

        for(int i=0; i < rivi.count(); i++) {
            QString txt = rivi.value(i);
            int tyyppi = sarakkeet_.value(i);

            switch (tyyppi) {
            case NIMI:
                if( map.contains("nimi"))
                    map.insert("nimi", map.value("nimi").toString() + " " + txt);
                else
                    map.insert("nimi", txt);
                break;
            case LAHIOSOITE:
                map.insert("osoite", txt);
                break;
            case POSTINUMERO:
                map.insert("postinumero", txt);
                break;
            case KAUPUNKI:
                map.insert("kaupunki", txt);
                break;
            case SAHKOPOSTI:
                map.insert("email", txt);
                break;
            case PUHELIN:
                map.insert("puhelin", txt);
                break;
            case KIELI:
                map.insert("kieli", txt);
                break;
            case MAA:
                map.insert("maa", txt);
                break;
            case LISATIETO:
                map.insert("lisatiedot", txt);
                break;
            case VERKKOLASKUOSOITE:
                map.insert("ovt", txt);
                break;
            case VERKKOLASKUVALITTAJA:
                map.insert("operaattori", txt);
                break;
            case POSTIOSOITE:
                map.insert("postinumero", txt.left(5));
                map.insert("kaupunki", txt.mid(6));
                break;
            }

        }
        if( map.contains("postinumero") && !map.contains("kaupunki")) {
            map.insert("kaupunki", Postinumerot::toimipaikka(map.value("postinumero").toString()));
        }

        lista.append(map);
    }

    return lista;
}

void RekisteriTuontiModel::asetaOtsikkorivi(bool otsikkorivi)
{
    if( otsikkorivi != otsikkorivi_) {
        beginResetModel();
        otsikkorivi_ = otsikkorivi;
        endResetModel();
        emit otsikkorivit(otsikkorivi);
    }
}


void RekisteriTuontiModel::arvaaSarakkeet()
{
    // Yritetään ensin löytää otsikkoja
    bool otsikkoja = false;
    QStringList rivi = csv_.value(0);

    for(int i=0; i < rivi.count(); i++) {
        QString txt = rivi.value(i);
        if( txt.contains("nimi", Qt::CaseInsensitive)) {
            sarakkeet_[i] = NIMI;
            otsikkoja = true;
        } else if( txt.contains("postiosoite", Qt::CaseInsensitive)) {
            sarakkeet_[i] = POSTIOSOITE;
            otsikkoja = true;
        } else if( txt.contains("osoite", Qt::CaseInsensitive)) {
            sarakkeet_[i] = LAHIOSOITE;
            otsikkoja = true;
        } else if( txt.contains("postinumero", Qt::CaseInsensitive)) {
            sarakkeet_[i] = POSTINUMERO;
            otsikkoja = true;
        } else if( txt.contains("toimipaikka", Qt::CaseInsensitive)) {
            sarakkeet_[i] = KAUPUNKI;
            otsikkoja = true;
        } else if( txt.contains("puhelin", Qt::CaseInsensitive)) {
            sarakkeet_[i] = PUHELIN;
            otsikkoja = true;
        } else if( txt.contains("email", Qt::CaseInsensitive) ||
                   txt.contains("sähköposti", Qt::CaseInsensitive)) {
            sarakkeet_[i] = SAHKOPOSTI;
            otsikkoja = true;
        }
    }
    if( otsikkoja ) {
        asetaOtsikkorivi(true);
        return;
    }
    QRegularExpression emailRe(R"(\S+@\S+[.]\w+)");
    QRegularExpression nroRe(R"(^\d{5}$)");
    QRegularExpression postiRe(R"(^\d{5}\s\S+)");
    QRegularExpression ytunnusRe(R"(^\d{7}-\d$)");

    for( int i=0; i < rivi.count(); i++) {
        QString txt = rivi.value(i);

        if(emailRe.match(txt).hasMatch())
            sarakkeet_[i] = SAHKOPOSTI;
        else if( nroRe.match(txt).hasMatch())
            sarakkeet_[i] = POSTINUMERO;
        else if( ytunnusRe.match(txt).hasMatch())
            sarakkeet_[i] = YTUNNUS;
        else if( postiRe.match(txt).hasMatch())
            sarakkeet_[i] = POSTIOSOITE;
    }
}
