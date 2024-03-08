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
#include "tositerivit.h"

#include "db/verotyyppimodel.h"
#include "db/kohdennus.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"

#include "model/tositevienti.h"
#include "lasku.h"

#include <QMessageBox>

TositeRivit::TositeRivit(QObject *parent, const QVariantList& data)
    : QAbstractTableModel(parent)
{
    for(const auto& variant : data) {
        rivit_.append( TositeRivi( variant.toMap()) );
    }
}

void TositeRivit::lataa(const QVariantList &data)
{
    beginResetModel();
    rivit_.clear();

    for(const auto& variant : data) {
        rivit_.append( TositeRivi( variant.toMap()) );
    }
    endResetModel();
}

QVariant TositeRivit::headerData(int section, Qt::Orientation orientation, int role) const
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
                    return rivityyppi_ == Lasku::BRUTTORIVIT ? tr("á hinta") : tr("á netto");
                case ALE:
                    return  tr("Alennus");
                case ALV:
                    return tr("Alv");
                case TILI:
                    return tr("Tili");
                case KOHDENNUS:
                    return tr("Kohdennus");
                case YHTEENSA:
                    return rivityyppi_ == Lasku::NETTORIVIT ? tr("Yht netto") : tr("Yhteensä");
            }
        }
        else
            return QVariant( section + 1);
    }
    return QVariant();
}


int TositeRivit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int TositeRivit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 9;
}

QVariant TositeRivit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const TositeRivi& rivi = rivit_.at(index.row());


    if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==YHTEENSA || index.column() == MAARA || index.column() == ALV || index.column() == AHINTA || index.column() == ALE)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    } else if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NIMIKE:
            return rivi.nimike();
        case MAARA:
            return rivi.laskutetaanKpl().replace(".",",");
        case YKSIKKO:
            if( rivi.unKoodi().isEmpty())
                return rivi.yksikko();
            else
                return yksikkoModel_.nimi(rivi.unKoodi());
        case AHINTA: {
            const double ahinta = rivityyppi_ == Lasku::BRUTTORIVIT ? rivi.aBrutto() : rivi.aNetto();
            return QString("%L1 €").arg(ahinta, 0, 'f', kp()->asetukset()->luku("LaskuYksikkoDesimaalit", 2));
        }
        case ALE:
            if(rivi.aleProsentti())
                return QString("%1 %").arg(rivi.aleProsentti() );
            else if( rivi.euroAlennus())
                return Euro::fromDouble( rivityyppi_ == Lasku::BRUTTORIVIT ? rivi.bruttoEuroAlennus() : rivi.euroAlennus() ).display();
            else
                return QString();
        case ALV:
            switch ( rivi.alvkoodi() ) {
            case AlvKoodi::ALV0:
                return tr("0 %");
            case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
                return tr("AVL 8c §");
            case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
                return tr("AVL 65 §");
            case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
                return tr("AVL 72a §");
            case Lasku::KAYTETYT:
                return tr("Margin. tavarat","Marginaaliverotus käytetyt tavarat/ Lyhyt teksti laskunäkymässä");
            case Lasku::TAIDE:
                return tr("Margin. taide","Marginaaliverotus taide-esineet/ Lyhyt teksti laskunäkymässä");
            case Lasku::ANTIIKKI:
                return tr("Margin. keräily","Marginaaliverotus keräily- ja antiikkiesineet/ Lyhyt teksti laskunäkymässä");
            default:
                return QString("%1 %").arg( rivi.alvProsentti() );
            }
        case KOHDENNUS:
            {
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(  rivi.kohdennus() );
                if( kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    return kohdennus.nimi();
                else
                    return QVariant();
        }
        case TILI:
            if( ennakkolasku_ )
                return tr("Saadut ennakot");
            return kp()->tilit()->tiliNumerolla( rivi.tili() ).nimiNumero();
        case YHTEENSA:
           return rivityyppi_ == Lasku::NETTORIVIT ? Euro::fromDouble(rivi.nettoYhteensa()).display() : rivi.bruttoYhteensa().display();
        }
    } else if( role == Qt::EditRole) {
        switch (index.column()) {
        case NIMIKE:
            return rivi.nimike();
        case MAARA:
            return rivi.laskutetaanKpl();
        case YKSIKKO:
            return rivi.yksikko();
        case AHINTA:
            return rivityyppi_ == Lasku::BRUTTORIVIT ? rivi.aBrutto() : rivi.aNetto();
        case ALE:
            return rivi.laskettuAleProsentti();
        case KOHDENNUS:
            return rivi.kohdennus();
        case TILI:
            return rivi.tili();
        case YHTEENSA:
            return rivityyppi_ == Lasku::NETTORIVIT ? rivi.nettoYhteensa() :  rivi.bruttoYhteensa().toDouble();
        }
    } else if( role == AlvProsenttiRooli)
        return rivi.alvProsentti();
    else if( role == AlvKoodiRooli)
        return rivi.alvkoodi();
    else if( role == TiliNumeroRooli )
        return rivi.tili();
    else if( role == UNkoodiRooli)
        return rivi.unKoodi();
    else if( role == Qt::DecorationRole && index.column() == ALV) {
        return kp()->alvTyypit()->kuvakeKoodilla(rivi.alvkoodi());
    }

    // FIXME: Implement me!
    return QVariant();
}

bool TositeRivit::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {        
        int r = index.row();

        if( role == Qt::EditRole) {
            switch (index.column()) {
            case NIMIKE:
                rivit_[r].setNimike( value.toString() );
                break;
            case MAARA:
                rivit_[r].setLaskutetaanKpl( value.toString());
                rivit_[r].setMyyntiKpl(value.toDouble());
                break;
            case YKSIKKO:
                rivit_[r].setYksikko( value.toString() );
                rivit_[r].setUNkoodi("ZZ");
                break;
            case AHINTA:
                if(rivityyppi_ == Lasku::BRUTTORIVIT)
                    rivit_[r].setABrutto(value.toDouble());
                else
                    rivit_[r].setANetto( value.toDouble());
                break;
            case ALE:
                rivit_[r].setEuroAlennus(0);
                rivit_[r].setAleProsentti(qAbs(value.toDouble()));
                break;
            case KOHDENNUS:
                rivit_[r].setKohdennus(value.toInt());
                break;
            case TILI:
            {
                // Tili asetetaan numerolla!
                Tili uusitili;
                if( value.toInt() )
                    uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
                else if(!value.toString().isEmpty() && value.toString() != " " && value.toString() != "0")
                    uusitili = TilinValintaDialogi::valitseTili(value.toString());
                else if( value.toString()==" " || !rivit_.at(r).tili() )
                    uusitili = TilinValintaDialogi::valitseTili( QString());

                if( uusitili.onkoValidi()) {
                    rivit_[r].setTili(uusitili.numero());
                    int alvlaji = uusitili.alvlaji();
                    if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen) && (alvlaji == AlvKoodi::EIALV || alvlaji == AlvKoodi::MYYNNIT_NETTO )) {
                        rivit_[r].setAlvKoodi( uusitili.alvlaji() );
                        rivit_[r].setAlvProsentti( uusitili.alvprosentti() );
                    }
                }
            }
                break;
            case YHTEENSA:
                if( rivityyppi_ != Lasku::NETTORIVIT)
                    rivit_[r].setBruttoYhteensa( value.toString() );
                else
                    rivit_[r].setNettoYhteensa( value.toDouble() );
            }
        } else if( role == AlvKoodiRooli) {
            rivit_[r].setAlvKoodi(value.toInt());            
        } else if( role == AlvProsenttiRooli) {
            rivit_[r].setAlvProsentti(value.toDouble());
        } else if( role == UNkoodiRooli) {
            rivit_[r].setUNkoodi(value.toString());
            rivit_[r].setYksikko(QString());    // TODO! Yksikön tulkattu teksti?
        }

        emit dataChanged(index.sibling(index.row(), MAARA),
                             index.sibling(index.row(), YHTEENSA),
                             QVector<int>() << Qt::DisplayRole << Qt::EditRole );

        if( index.row() == rowCount() - 1 && rivit_.at(r).bruttoYhteensa().cents())
            lisaaRivi();

        return true;
    }
    return false;
}

Qt::ItemFlags TositeRivit::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    if( ennakkolasku_ && index.row() == TILI)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QVariantList TositeRivit::rivit() const
{
    QVariantList ulos;
    for(auto rivi : rivit_)
    {
        if( rivi.bruttoYhteensa().cents() || !rivi.nimike().isEmpty()) {
            auto map = rivi.data();
            if( ennakkolasku_ ) rivi.unset("myyntikpl");    // Ennakkolaskutetut tuotteet eivät näy varastossa
            ulos.append( map );
        }
    }
    return ulos;
}

Euro TositeRivit::yhteensa() const
{
    Euro summa;
    for(const auto& rivi : qAsConst( rivit_ ))
        summa += rivi.bruttoYhteensa();
    return summa;
}

bool TositeRivit::onkoTyhja() const
{
    for( const auto& rivi : rivit_)
    {
        if( rivi.bruttoYhteensa().cents() || !rivi.nimike().isEmpty() )
            return false;
    }
    return true;
}

TositeRivi TositeRivit::rivi(int indeksi) const
{
    return rivit_.at(indeksi);
}

void TositeRivit::asetaRivi(int indeksi, const TositeRivi &rivi)
{
    rivit_[indeksi] = rivi;
    emit dataChanged( index(indeksi, NIMIKE),
                      index(indeksi, YHTEENSA));
}

void TositeRivit::lisaaRivi()
{
    TositeRivi rivi;

    int uusitili = kp()->asetukset()->luku("OletusMyyntitili",3000);
    rivi.setTili(uusitili);
    Tili *tili = kp()->tilit()->tili(uusitili);
    if(tili && kp()->asetukset()->onko(AsetusModel::AlvVelvollinen)) {
        const int alvlaji = tili->alvlaji();
        rivi.setAlvKoodi( alvlaji == AlvKoodi::EIALV || alvlaji == AlvKoodi::ALV0 || alvlaji == AlvKoodi::RAKENNUSPALVELU_MYYNTI || alvlaji == AlvKoodi::YHTEISOMYYNTI_PALVELUT || alvlaji == AlvKoodi::YHTEISOMYYNTI_TAVARAT
                             ? alvlaji : AlvKoodi::MYYNNIT_NETTO );
        rivi.setAlvProsentti( tili->alvprosentti());
    }

    rivi.setMyyntiKpl(1.0);
    rivi.setLaskutetaanKpl("1");

    lisaaRivi(rivi);
}

void TositeRivit::lisaaRivi(TositeRivi rivi)
{
    int indeksi = rivit_.count();
    if(indeksi > 0 && !rivit_.at(indeksi - 1).bruttoYhteensa() )
        indeksi = indeksi - 1;

    beginInsertRows( QModelIndex(), indeksi, indeksi );
    rivit_.insert(indeksi, rivi);
    endInsertRows();
}

void TositeRivit::lisaaTuote(const Tuote &tuote, const QString &lkm, const QString kieli)
{
    TositeRivi rivi;
    rivi.setTuote(tuote.id());
    rivi.setNimike(tuote.nimike(kieli));

    if( !tuote.unKoodi().isEmpty())
        rivi.setUNkoodi( tuote.unKoodi());
    else if( !tuote.yksikko().isEmpty()) {
        rivi.setUNkoodi("");
        rivi.setYksikko( tuote.yksikko() );
    }

    rivi.setLaskutetaanKpl( lkm );
    rivi.setMyyntiKpl( lkm.toDouble());

    rivi.setANetto( tuote.ahinta() );
    rivi.setTili( tuote.tili() );
    rivi.setKohdennus( tuote.kohdennus() );

    if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen)) {
        rivi.setAlvKoodi( tuote.alvkoodi() );
        rivi.setAlvProsentti( tuote.alvprosentti() );
    };

    rivi.setTuoteKoodi( tuote.koodi() );

    int indeksi = rivit_.count();
    if(indeksi > 0 && !rivit_.at(indeksi - 1).bruttoYhteensa() )
        indeksi = indeksi - 1;

    beginInsertRows( QModelIndex(), indeksi, indeksi );
    rivit_.insert(indeksi, rivi);
    endInsertRows();
}


void TositeRivit::poistaRivi(int rivi)
{
    beginRemoveRows(QModelIndex(), rivi, rivi);
    rivit_.removeAt(rivi);
    endRemoveRows();
}

void TositeRivit::asetaEnnakkolasku(bool ennakkoa)
{
    beginResetModel();
    ennakkolasku_ = ennakkoa;
    endResetModel();
}

void TositeRivit::asetaRivityyppi(Lasku::Rivityyppi rivityyppi)
{
    beginResetModel();
    rivityyppi_ = rivityyppi;
    endResetModel();
}

void TositeRivit::clear()
{
    beginResetModel();
    rivit_.clear();
    ennakkolasku_ = false;
    rivityyppi_ = Lasku::NETTORIVIT;
    endResetModel();
}



