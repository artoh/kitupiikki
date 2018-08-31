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
#include "ryhmantuontimodel.h"
#include "tuonti/csvtuonti.h"
#include "laskuryhmamodel.h"

#include <QFile>

RyhmanTuontiModel::RyhmanTuontiModel(QObject *parent)
    : QAbstractTableModel (parent)
{

}

int RyhmanTuontiModel::rowCount(const QModelIndex & /* parent */) const
{
    return csv_.count();
}

int RyhmanTuontiModel::columnCount(const QModelIndex & /* parent */) const
{
    if( csv_.isEmpty())
        return 0;
    return csv_.first().count();
}

QVariant RyhmanTuontiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return otsikkoTeksti( sarakkeet_.at(section) );
    }

    return {};
}

QVariant RyhmanTuontiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return {};

    if( role == Qt::DisplayRole)
    {
        return csv_.at(index.row()).at(index.column());
    }

    else if( role == Qt::TextColorRole)
    {
        if( index.row() == 0 && otsikkorivi_)
            return QColor(Qt::gray);
    }

    return {};
}

QString RyhmanTuontiModel::otsikkoTeksti(int sarakeEnum)
{
    switch (sarakeEnum) {
        case NIMI: return tr("Nimi");
        case LAHIOSOITE: return tr("Lähiosoite");
        case POSTIOSOITE: return tr("Postiosoite");
        case POSTINUMERO: return tr("Postinumero");
        case OSOITE: return tr("Osoite");
        case SAHKOPOSTI: return tr("Sähköposti");
        case YTUNNUS: return tr("Y-tunnus");
    default:
        return {};
    }
}

void RyhmanTuontiModel::lataaCsv(const QString &tiedostonNimi)
{
    QFile tiedosto(tiedostonNimi);
    if( !tiedosto.open(QIODevice::ReadOnly))
        return;

    beginResetModel();
    csv_ = CsvTuonti::csvListana( tiedosto.readAll() );

    if( !csv_.isEmpty())
        arvaaSarakkeet();

    endResetModel();
}

void RyhmanTuontiModel::asetaOtsikkoRivi(bool onko)
{
    otsikkorivi_ = onko;
    if( !csv_.isEmpty())
        emit dataChanged(index(0,0), index(0, csv_.first().size()));
}

void RyhmanTuontiModel::asetaMuoto(int sarake, int muoto)
{
    beginResetModel();
    sarakkeet_[sarake] = muoto;
    endResetModel();
}

void RyhmanTuontiModel::lisaaLaskuun(LaskuRyhmaModel *model)
{
    for(int i = onkoOtsikkoRivi() ? 1 : 0; i < csv_.count(); i++)
    {
        QString nimi;
        QString osoite;
        QString lahiosoite;
        QString postinumero;
        QString postiosoite;
        QString sahkoposti;
        QString ytunnus;

        for(int c = 0; c < csv_.at(i).count(); c++)
        {
            if( sarakkeet_.count() <= c)
                break;

            const QString& txt = csv_.at(i).at(c);

            switch (sarakkeet_.at(c)) {
                case NIMI :
                    if(!nimi.isEmpty())
                        nimi.append(" ");
                    nimi.append(txt);
                    break;
                case LAHIOSOITE:
                    lahiosoite = txt;
                    break;
                case POSTINUMERO:
                    postinumero = txt;
                    break;
                case POSTIOSOITE:
                    postiosoite = txt;
                    break;
                case SAHKOPOSTI:
                    sahkoposti = txt;
                    break;
                case YTUNNUS:
                    ytunnus = txt;
                    break;
                case OSOITE:
                    osoite = txt;
                    osoite.replace(QRegularExpression(R"(,\s*)"),"\n");
            }
        }
        if( osoite.isEmpty())
        {
            osoite = lahiosoite + "\n";
            if( !postinumero.isEmpty())
                osoite.append(postinumero + " ");
            osoite.append(postiosoite);
        }

        osoite = nimi + "\n" + osoite;

        if( !nimi.isEmpty())
            model->lisaa(nimi, osoite, sahkoposti, ytunnus);
    }
}

void RyhmanTuontiModel::arvaaSarakkeet()
{
    // Ensiksi kokeillaan, josko ensimmäisellä rivillä olisi otsikkoja
    QVector<bool> loydetty(YTUNNUS+1);

    for(QString otsikko : csv_.first())
    {
        int tyyppi = EITUODA;
        for(int i=0; i<=YTUNNUS; i++)
        {
            if( (!otsikko.compare( otsikkoTeksti(i), Qt::CaseInsensitive ) && !loydetty[i]) || (otsikko.contains("nimi", Qt::CaseInsensitive) && i == NIMI))
            {
                tyyppi = i;
                otsikkorivi_ = true;
                loydetty[i] = true;
                break;
            }
        }
        sarakkeet_.append(tyyppi);

    }
    // Sitten pitäisi varmaan vielä yrittää muodolla

    QRegularExpression emailRe(R"(\S+@\S+[.]\w+)");
    QRegularExpression nroRe(R"(^\d{5}$)");
    QRegularExpression lahiRe(R"(^\S{3,}\s\d.*)");
    QRegularExpression postiRe(R"(^\d{5}\s\S+.*$)");
    QRegularExpression osoiteRe(R"(^\S.*,\s?\d{5}\s\S+.*$)");
    QRegularExpression ytunnusRe(R"(^\d{7}-\d$)");

    for(int c = 0; c < sarakkeet_.count(); c++)
    {
        if( sarakkeet_.at(c) > EITUODA)
            continue;

        QVector<bool> eisovi(YTUNNUS + 1);

        for(int r = onkoOtsikkoRivi() ? 1 : 0; r < csv_.count(); r++)
        {
            if( csv_.at(r).count() <= c)
                continue;
            eisovi[EITUODA] = true;

            const QString &txt = csv_.at(r).at(c);

            if( txt.isEmpty())
                continue;

            if(  emailRe.match(txt).hasMatch() )
                eisovi[NIMI] = true;
            else
                eisovi[SAHKOPOSTI] = true;

            if( !nroRe.match(txt).hasMatch())
                eisovi[POSTINUMERO] = true;
            if( !lahiRe.match(txt).hasMatch())
                eisovi[LAHIOSOITE] = true;
            if( !postiRe.match(txt).hasMatch())
                eisovi[POSTIOSOITE] = true;
            if( !osoiteRe.match(txt).hasMatch())
                eisovi[OSOITE] = true;
            if( !ytunnusRe.match(txt).hasMatch())
                eisovi[YTUNNUS] = true;
        }
        QVector<bool> kaytetty(YTUNNUS + 1);

        if( eisovi[EITUODA])
        {
            for(int v=LAHIOSOITE; v<YTUNNUS+1; v++)
            {
                if( !eisovi.at(v))
                {
                    if( kaytetty.at(v)  )
                        continue;

                    sarakkeet_[c] = v;
                    kaytetty[v] = true;
                    break;
                }
            }
            if( sarakkeet_.at(c) == EITUODA && !eisovi.at(NIMI) && !kaytetty.at(NIMI))
            {
                sarakkeet_[c] = NIMI;
                kaytetty[NIMI] = true;
            }
        }
    }
}

