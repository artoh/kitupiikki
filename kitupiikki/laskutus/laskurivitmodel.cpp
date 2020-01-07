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
#include "laskurivitmodel.h"

#include "db/verotyyppimodel.h"
#include "db/kohdennus.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"

#include "model/tositevienti.h"

LaskuRivitModel::LaskuRivitModel(QObject *parent, const QVariantList& data)
    : QAbstractTableModel(parent),
      rivit_(data)
{
    lisaaRivi();
}

QVariant LaskuRivitModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    if( role == Qt::DisplayRole)
    {
        if( orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case NIMIKE:
                    return tr("Nimike");
                case MAARA:
                    return tr("Määrä");
                case YKSIKKO:
                    return tr("Yksikkö");
                case AHINTA :
                    return tr("á netto");
                case ALE:
                    return  tr("Alennus");
                case ALV:
                    return tr("Alv");
                case TILI:
                    return tr("Tili");
                case KOHDENNUS:
                    return tr("Kohdennus");
                case BRUTTOSUMMA:
                    return tr("Yhteensä");
            }
        }
        else
            return QVariant( section + 1);
    }
    return QVariant();
}


int LaskuRivitModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int LaskuRivitModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 9;
}

QVariant LaskuRivitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = rivit_.at(index.row()).toMap();


    if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==BRUTTOSUMMA || index.column() == MAARA || index.column() == ALV || index.column() == AHINTA || index.column() == ALE)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    } else if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NIMIKE:
            return map.value("nimike");
        case MAARA:
            return map.value("myyntikpl");
        case YKSIKKO:
            return map.value("yksikko");
        case AHINTA:
            return QString("%L1 €").arg(map.value("ahinta").toDouble(),0,'f',2);
        case ALE:
            if(map.contains("aleprosentti"))
                return QString("%1 %").arg(map.value("aleprosentti").toDouble());
            else
                return QString();
        case ALV:
            switch ( map.value("alvkoodi").toInt()) {
            case AlvKoodi::ALV0:
                return tr("0 %");
            case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
                return tr("AVL 8c §");
            case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
                return tr("AVL 65 §");
            case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
                return tr("AVL 72a §");
            case AlvKoodi::MYYNNIT_MARGINAALI :
                return "Margin.";
            default:
                return QString("%1 %").arg( map.value("alvprosentti").toDouble() );
            }
        case KOHDENNUS:
            {
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(  map.value("kohdennus").toInt());
                if( kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    return kohdennus.nimi();
                else
                    return QVariant();
        }
        case TILI:
            if( ennakkolasku_ )
                return tr("Saadut ennakot");
            return kp()->tilit()->tiliNumerolla( map.value("tili").toInt() ).nimiNumero();
        case BRUTTOSUMMA:
           return QString("%L1 €").arg(  riviSumma(map) ,0,'f',2);
        }
    } else if( role == Qt::EditRole) {
        switch (index.column()) {
        case NIMIKE:
            return map.value("nimike").toString();
        case MAARA:
            return map.value("myyntikpl").toDouble();
        case YKSIKKO:
            return map.value("yksikko").toString();
        case AHINTA:
            return map.value("ahinta").toDouble();
        case ALE:
            return map.value("aleprosentti").toDouble();
        case KOHDENNUS:
            return map.value("kohdennus").toInt();
        case TILI:
            return map.value("tili").toInt();
        case BRUTTOSUMMA:
            return riviSumma(map);
        }
    } else if( role == AlvProsenttiRooli)
        return map.value("alvprosentti").toDouble();
    else if( role == AlvKoodiRooli)
        return map.value("alvkoodi").toInt();
    else if( role == TiliNumeroRooli )
        return map.value("tili").toInt();
    else if( role == Qt::DecorationRole && index.column() == ALV) {
        return kp()->alvTyypit()->kuvakeKoodilla(map.value("alvkoodi").toInt());
    }

    // FIXME: Implement me!
    return QVariant();
}

bool LaskuRivitModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        QVariantMap map = rivit_.at(index.row()).toMap();

        if( role == Qt::EditRole) {
            switch (index.column()) {
            case NIMIKE:
                map.insert("nimike", value.toString());
                break;
            case MAARA:
                map.insert("myyntikpl", value.toDouble());
                break;
            case YKSIKKO:
                map.insert("yksikko", value.toString());
                break;
            case AHINTA:
                map.insert("ahinta", value.toDouble());
                break;
            case ALE:
                map.insert("aleprosentti", value.toDouble());
                break;
            case KOHDENNUS:
                map.insert("kohdennus", value.toInt());
                break;
            case TILI:
            {
                // Tili asetetaan numerolla!
                Tili uusitili;
                if( value.toInt() )
                    uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
                else if(!value.toString().isEmpty() && value.toString() != " " && value.toString() != "0")
                    uusitili = TilinValintaDialogi::valitseTili(value.toString());
                else if( value.toString()==" " || map.value("rivi").toInt())
                    uusitili = TilinValintaDialogi::valitseTili( QString());

                if( uusitili.onkoValidi()) {
                    map.insert("tili", uusitili.numero());
                    if( kp()->asetukset()->onko(AsetusModel::ALV)) {
                        map.insert("alvkoodi", uusitili.arvo("alvlaji").toInt());
                        map.insert("alvprosentti",uusitili.arvo("alvprosentti").toDouble());
                        rivit_[index.row()] = map;
                        emit dataChanged(index.sibling(index.row(), ALV), index.sibling(index.row(), BRUTTOSUMMA), QVector<int>() << role);
                        return true;
                    }
                }
            }
                break;
            case BRUTTOSUMMA:
                if( map.value("myyntikpl").toDouble() < 1e-5)
                    return false;
                // Tässä ei huomioitu marginaalimenettelyä
                double ahinta = 10000 * value.toDouble() / map.value("myyntikpl").toDouble() /
                        (100.0 + map.value("alvprosentti").toDouble()) /
                         (100.0 - map.value("aleprosentti").toDouble());
                map.insert("ahinta", ahinta);
            }
        } else if( role == AlvKoodiRooli) {
            map.insert("alvkoodi", value.toDouble());
        } else if( role == AlvProsenttiRooli) {
            map.insert("alvprosentti", value.toDouble());
        }

        rivit_[index.row()] = map;

        if( index.column() == AHINTA || index.column() == MAARA || index.column() == ALE
                || index.column() == BRUTTOSUMMA )
            // Summa muuttui
            emit dataChanged(index.sibling(index.row(), MAARA),
                             index.sibling(index.row(), BRUTTOSUMMA),
                             QVector<int>() << role);
        else
            emit dataChanged(index, index, QVector<int>() << role);

        if( index.row() == rowCount() - 1 && map.value("ahinta").toDouble() > 1e-5)
            lisaaRivi();

        return true;
    }
    return false;
}

Qt::ItemFlags LaskuRivitModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    if( ennakkolasku_ && index.row() == TILI)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QVariantList LaskuRivitModel::rivit() const
{
    QVariantList ulos;
    for(auto rivi : rivit_)
    {
        QVariantMap map = rivi.toMap();
        if( qAbs(map.value("ahinta").toDouble()) > 1e-5 && qAbs(map.value("myyntikpl").toDouble()) > 1e-5 )
            ulos.append(rivi);
    }
    return ulos;
}

double LaskuRivitModel::yhteensa() const
{
    double summa = 0;
    for(auto rivi : rivit_)
        summa += qRound64(riviSumma( rivi.toMap() ) * 100.0 );
    return summa / 100.0;
}

QVariantList LaskuRivitModel::viennit(const QDate& pvm, const QDate &jaksoalkaa, const QDate &jaksopaattyy, const QString &otsikko, bool ennakkolasku) const
{
    QVariantList lista;

    for(auto rivi : rivit()) {
        QVariantMap map = rivi.toMap();
        int alvkoodi = map.value("alvkoodi").toInt();

        if( alvkoodi == AlvKoodi::MYYNNIT_NETTO && ennakkolasku)
            alvkoodi = AlvKoodi::ENNAKKOLASKU_MYYNTI;
        else if( alvkoodi == AlvKoodi::MYYNNIT_NETTO && kp()->onkoMaksuperusteinenAlv(pvm))
            alvkoodi = AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;

        if( ennakkolasku )
            map.insert("tili", kp()->asetukset()->luku("LaskuEnnakkotili"));

        bool vanhaan = false;

        if( !map.contains("ennakkohyvitys")) {
            for(int i=0; i < lista.count(); i++) {
                QVariantMap vmap = lista.at(i).toMap();

                if( map.value("tili") == vmap.value("tili") &&
                    map.value("kohdennus",0) == vmap.value("kohdennus",0) &&
                    map.value("alvprosentti",0) == vmap.value("alvprosentti",0) &&
                    alvkoodi == vmap.value("alvkoodi",0))
                {
                    double brutto = riviSumma( map);
                    double vanhasumma = vmap.value("kredit",0).toDouble() - vmap.value("debet",0).toDouble();
                    double uusisumma = vanhasumma + brutto;

                    if( uusisumma > 0.0) {
                        vmap.insert("kredit", uusisumma);
                        vmap.remove("debet");
                    } else {
                        vmap.insert("debet", 0 - uusisumma);
                        vmap.remove("kredit");
                    }
                    vmap.insert("tyyppi", TositeVienti::MYYNTI + TositeVienti::KIRJAUS);

                    lista[i] = vmap;
                    vanhaan = true;
                    break;
                }
            }
        }
        if( !vanhaan ) {
            TositeVienti vienti;
            vienti.setTili( map.value("tili").toInt() );
            vienti.setKohdennus( map.value("kohdennus").toInt());
            vienti.setAlvKoodi( alvkoodi );
            vienti.setAlvProsentti( map.value("alvprosentti").toInt());
            vienti.setPvm(pvm);
            if( jaksoalkaa.isValid())
                vienti.setJaksoalkaa(jaksoalkaa);
            if( jaksopaattyy.isValid())
                vienti.setJaksoloppuu(jaksopaattyy);

            double summa = riviSumma(map);
            if( summa > 0)
                vienti.setKredit( summa );
            else
                vienti.setDebet( 0 - summa);

            vienti.setSelite( otsikko );
            if( alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI)
                vienti.setEra(-1);
            else if( map.contains("ennakkohyvitys"))
                vienti.setEra( map.value("ennakkohyvitys").toInt());

            lista.append(vienti);
        }
    }

    // Nyt vienteihin pitäisi vielä täydentään kotimaan alv:n alv-velkarivit

    QVariantList ulos;
    for(auto rivi: lista) {
        QVariantMap map = rivi.toMap();
        int alvkoodi = map.value("alvkoodi").toInt();
        if( alvkoodi == AlvKoodi::MYYNNIT_NETTO ||
            alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI ||
            alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
            double brutto = map.value("kredit",0).toDouble() - map.value("debet",0).toDouble();
            double netto = qRound64( brutto * 10000 / ( 100 + map.value("alvprosentti").toDouble() ) ) / 100.0;
            double vero = brutto - netto;

            if( netto > 0)
                map.insert("kredit", netto);
            else
                map.insert("debet", 0 - netto);

            ulos.append(map);

            TositeVienti verorivi;
            verorivi.setPvm(pvm);

            // TODO: Maksuperusteinen ALV ja Ennakkolaskutus
            if( alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI ) {
                verorivi.setTili( kp()->asetukset()->luku("LaskuEnnakkoALV") );
                verorivi.setAlvKoodi( AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON );
                verorivi.setEra(-1);
            } else if( alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
                verorivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).numero());
                verorivi.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON);
                verorivi.setEra(-1);
            } else {
                verorivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
                verorivi.setAlvKoodi( AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS);
            }
            if( vero > 0)
                verorivi.setKredit(vero);
            else
                verorivi.setDebet(vero);
            verorivi.setAlvProsentti( map.value("alvprosentti").toInt() );
            verorivi.setTyyppi( TositeVienti::ALVKIRJAUS + TositeVienti::MYYNTI );
            verorivi.setSelite( otsikko + " ALV " + QString::number(map.value("alvprosentti").toInt()) );
            ulos.append(verorivi);
        } else {
            ulos.append(map);
        }
    }
    return ulos;
}

bool LaskuRivitModel::onkoTyhja() const
{
    for( auto rivi : rivit_)
    {
        QVariantMap map = rivi.toMap();
        if( qAbs(map.value("ahinta").toDouble()) > 1e-5 &&
            qAbs(map.value("myyntikpl").toDouble()) > 1e-5)
            return false;
    }
    return true;
}

void LaskuRivitModel::lisaaRivi(QVariantMap rivi)
{
    if( !rivi.contains("myyntikpl"))
        rivi.insert("myyntikpl", 1.0);
    if( !rivi.contains("tili")) {
        int uusitili = kp()->asetukset()->luku("OletusMyyntitili",3000);
        rivi.insert("tili", uusitili);
        Tili *tili = kp()->tilit()->tili(uusitili);
        if(tili && kp()->asetukset()->onko(AsetusModel::ALV)) {
            rivi.insert("alvkoodi", tili->arvo("alvlaji").toInt());
            rivi.insert("alvprosentti", tili->arvo("alvprosentti").toDouble());
        }
    }

    int rivia = rivit_.count();
    if( rivia && qAbs( rivit_.last().toMap().value("ahinta").toDouble()) < 1e-5 )
    {
        rivit_[rivia-1] = rivi;
        emit dataChanged( index(rivia-1, 0), index(rivia-1, columnCount()) );
            lisaaRivi();
        return;
    }

    beginInsertRows( QModelIndex(), rivit_.count(), rivit_.count() );
    rivit_.append(rivi);
    endInsertRows();
}

void LaskuRivitModel::asetaEnnakkolasku(bool ennakkoa)
{
    beginResetModel();
    ennakkolasku_ = ennakkoa;
    endResetModel();
}




double LaskuRivitModel::riviSumma(QVariantMap map)
{
    double maara = map.value("myyntikpl",1).toDouble();
    double ahinta = map.value("ahinta").toDouble();
    double alennus = map.value("aleprosentti",0).toDouble();
    double alvprossa = map.value("alvprosentti").toDouble();

    double brutto = map.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_MARGINAALI ?
                maara * ahinta * ( 100 - alennus) / 100 :
                maara * ahinta * ( 100 - alennus) * (100 + alvprossa) / 10000;

    return qRound64( brutto * 100.0 ) / 100.0;

}
