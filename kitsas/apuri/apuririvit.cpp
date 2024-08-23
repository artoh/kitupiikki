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
#include "apuririvit.h"

#include "db/kirjanpito.h"
#include "model/tositevienti.h"
#include "model/tosite.h"

ApuriRivit::ApuriRivit(QObject *parent)
    : QAbstractTableModel(parent)
{

}

QVariant ApuriRivit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {

            case TILI:
                return tr("Tili");
            case ALV:
                return tr("Alv");
            case EUROA:
                return tr("€");
        }
    }
    return QVariant();
}

int ApuriRivit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int ApuriRivit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant ApuriRivit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        if( index.column() == TILI) {
            Tili* tilini = kp()->tilit()->tili( rivit_.at(index.row()).tilinumero() );
            if( tilini )
                return  tilini->nimi() ;
        } else if( index.column() == ALV) {
            double alv = rivit_.at(index.row()).alvprosentti();
            int alvtyyppi = rivit_.at(index.row()).alvkoodi();
            if( alv < 1e-3 || kp()->alvTyypit()->nollaTyyppi(alvtyyppi))
                return QVariant();
            else
                return QString("%L1 %").arg(alv,0,'f',1);
        }
        else if( index.column() == EUROA)
        {
            return rivit_.at( index.row() ).naytettava().display(false);
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==EUROA)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::DecorationRole && index.column() == ALV) {
        return kp()->alvTyypit()->kuvakeKoodilla( rivit_.at(index.row()).alvkoodi() );
    }
    return QVariant();
}

void ApuriRivit::lisaa(const QVariantMap &map)
{
    TositeVienti vienti(map);

    if( vienti.tyyppi() % 100 == TositeVienti::KIRJAUS) {
        rivit_.append( ApuriRivi( vienti, plusOnKredit()) );
    } else if( !rivit_.count()) {
        ;
    } else if( vienti.alvKoodi() / 100 == AlvKoodi::ALVKIRJAUS / 100) {
        Euro vero = vienti.kreditEuro() - vienti.debetEuro();
        rivit_[ rivit_.count()-1].setNetonVero(vero, vienti.id());
    } else if( vienti.alvKoodi() / 100 == AlvKoodi::ALVVAHENNYS / 100) {
        Euro vahennys = vienti.debetEuro() - vienti.kreditEuro();
        if( !rivit_[rivit_.count()-1].bruttoSyotetty())
            rivit_[ rivit_.count() - 1].setNetonVero(vahennys, 0);
        rivit_[ rivit_.count() - 1 ].setAlvvahennys(true, vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::VAHENNYSKELVOTON) {
        rivit_[ rivit_.count() - 1].setVahentamaton( vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::MAAHANTUONTI_VERO) {
        rivit_[ rivit_.count() - 1 ].setMaahantuonninAlv(vienti.id());
    }
}

int ApuriRivit::lisaaRivi(int tili, const QDate &pvm)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    rivit_.append(ApuriRivi(tili, pvm));
    endInsertRows();
    return rowCount() - 1;
}

void ApuriRivit::lisaaRivi(const ApuriRivi &rivi)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    rivit_.append(rivi);
    endInsertRows();
}

void ApuriRivit::poistaRivi(int rivi)
{
    if(rivi < rowCount() && rowCount() > 1) {
        beginRemoveRows(QModelIndex(), rivi, rivi);
        rivit_.removeAt(rivi);
        endRemoveRows();
    }
}

ApuriRivi *ApuriRivit::rivi(int indeksi)
{
    if( indeksi == rivit_.count())
        rivit_.append(ApuriRivi());
    return &rivit_[indeksi];
}

ApuriRivi ApuriRivit::at(int indeksi) const
{
    return rivit_.at(indeksi);
}

ApuriRivi ApuriRivit::eka() const
{
    if( rivit_.isEmpty())
        return ApuriRivi();
    return rivit_.at(0);
}

void ApuriRivit::clear()
{
    beginResetModel();
    rivit_.clear();
    endResetModel();
}

QVariantList ApuriRivit::viennit(Tosite* tosite)
{
    QVariantList lista;

    for(int i=0; i < rowCount(); i++) {
        const ApuriRivi& rivi = rivit_.at(i);
        if( tosite->data(Tosite::TILA).toInt() == Tosite::MALLIPOHJA || (rivi.brutto() && rivi.tilinumero() )  )
            lista.append( rivit_.at(i).viennit( tyyppi(), plusOnKredit(),
                                              tosite->otsikko(), tosite->kumppanimap(), tosite->pvm()) );
    }

    return lista;
}

void ApuriRivit::asetaTyyppi(TositeVienti::VientiTyyppi tyyppi, bool plusOnKredit)
{
    tyyppi_ = tyyppi;    
    plusOnKredit_ = plusOnKredit;
}

void ApuriRivit::asetaRivit(const QList<ApuriRivi> &rivit)
{
    beginResetModel();
    rivit_ = rivit;
    endResetModel();
}

Euro ApuriRivit::summa() const
{
    Euro yhteensa;
    for(int i=0; i < rowCount(); i++)
        yhteensa += rivit_.at(i).naytettava();
    if( plusOnKredit() )
        return yhteensa;
    else
        return Euro::Zero-yhteensa;
}

QList<ApuriRivi> ApuriRivit::rivit() const
{
    return rivit_;
}

