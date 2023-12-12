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
#include "tiliotemodel.h"

#include "tiliotekirjausrivi.h"
#include "tilioteharmaarivi.h"

#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "db/kpkysely.h"
#include "db/tositetyyppimodel.h"
#include "tilioterivi.h"

#include <QSortFilterProxyModel>

TilioteModel::TilioteModel(QObject *parent, KitsasInterface *kitsasInterface)
    : QAbstractTableModel(parent),
      kitsasInterface_(kitsasInterface)    
{
}

QVariant TilioteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case TilioteRivi::PVM:
            return tr("Pvm");
        case TilioteRivi::EURO:
            return tr("Euro");
        case TilioteRivi::TILI:
            return tr("Tili");
        case TilioteRivi::ALV:
            return tr("Alv");
        case TilioteRivi::KOHDENNUS:
            return tr("Kohdennus");
        case TilioteRivi::SAAJAMAKSAJA:
            return tr("Saaja/Maksaja");
        case TilioteRivi::SELITE:
            return tr("Selite");

        }
    }
    return QVariant();
}


int TilioteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return kirjausRivit_.count() + harmaatRivit_.count();
}

int TilioteModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 7;
}

QVariant TilioteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int rivi = index.row();
    int kirjausriveja = kirjausRivit_.count();

    if( rivi < kirjausriveja )
        return kirjausRivit_.at(rivi).riviData(index.column(), role);
    else {
        const bool alternate = role == Qt::BackgroundRole ? proxy_->mapFromSource(index).row() % 2 == 1 : false;
        return harmaatRivit_.at(rivi - kirjausriveja).riviData(index.column(), role, alternate);
    }
}

bool TilioteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( index.isValid() && index.row() < kirjausRivit_.count()) {
        if( kirjausRivit_[index.row()].setRiviData(index.column(), value)) {
            emit dataChanged(
                        index.sibling(index.row(), TilioteRivi::PVM),
                        index.sibling(index.row(), TilioteRivi::EURO),
                        QVector<int>() << role);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TilioteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !muokkausSallittu_)
        return Qt::NoItemFlags;    

    if( index.row() < kirjausRivit_.count())
        return kirjausRivit_.at(index.row()).riviFlags(index.column());

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool TilioteModel::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    beginInsertRows(QModelIndex(),row, row + count - 1);

    for(int i=0; i < count; i++) {
        if(kirjausRivit_.count() >= row && row) {
            kirjausRivit_.insert(row+i, TilioteKirjausRivi(kirjausRivit_.at(row-1).pvm(),this));
        } else {
            kirjausRivit_.append(TilioteKirjausRivi( QDate(),this));
        }
    }

    endInsertRows();
    return true;
}

int TilioteModel::lisaaRivi(const QDate &pvm)
{
    return lisaaRivi(TilioteKirjausRivi(pvm, this));
}

int TilioteModel::lisaaRivi(TilioteKirjausRivi rivi)
{
    if( !rivi.lisaysIndeksi())
        rivi.asetaLisaysIndeksi( lisaysIndeksi() );

    int indeksi = kirjausRivit_.count();
    beginInsertRows(QModelIndex(), indeksi, indeksi);
    kirjausRivit_.append( rivi );
    endInsertRows();
    return indeksi;
}

void TilioteModel::poistaRivi(const int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    kirjausRivit_.removeAt(indeksi);
    endRemoveRows();
}

void TilioteModel::asetaRivi(int indeksi, const TilioteKirjausRivi& rivi)
{
    kirjausRivit_[indeksi] = rivi;
    emit dataChanged( index(indeksi, TilioteRivi::PVM),
                      index(indeksi, TilioteRivi::EURO),
                      QVector<int>() << Qt::EditRole);
}

TilioteKirjausRivi TilioteModel::rivi(const int indeksi) const
{
    return kirjausRivit_.at(indeksi);
}

void TilioteModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    kirjausRivit_.clear();

    QVariantList rivinlista;
    for(auto const& listanrivi : lista) {
        const QVariantMap& rivimap = listanrivi.toMap();
        int rivintili = rivimap.value("tili").toInt();
        if( rivintili == tilinumero() && !rivinlista.isEmpty()) {
            kirjausRivit_.append(TilioteKirjausRivi(rivinlista, this));
            rivinlista.clear();
        }
        rivinlista.append(listanrivi);
    }
    if( !rivinlista.isEmpty()) {
        kirjausRivit_.append(TilioteKirjausRivi(rivinlista, this));
    }
    endResetModel();
}


void TilioteModel::asetaTilinumero(int tilinumero)
{
    tili_ = tilinumero;
}

int TilioteModel::lisaysIndeksi()
{
    lisaysIndeksi_++;
    return lisaysIndeksi_;
}

void TilioteModel::tilaaAlkuperaisTosite(int rivinLisaysIndeksi, int eraId)
{
    KpKysely *kysely = kitsas()->yhteysModel()->kysely("/tositteet");
    kysely->lisaaAttribuutti("vienti", eraId);
    connect( kysely, &KpKysely::vastaus, this,
             [this, rivinLisaysIndeksi, eraId] (QVariant* data) { this->alkuperaisTositeSaapuu(rivinLisaysIndeksi, data, eraId); } );
    kysely->kysy();
}

void TilioteModel::asetaTositeId(int id)
{
    tositeId_ = id;
}

int TilioteModel::tositeId() const
{
    return tositeId_;
}

QSortFilterProxyModel *TilioteModel::initProxy()
{
    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel(this);
    return proxy_;
}

void TilioteModel::lataaHarmaat(const QDate &mista, const QDate &mihin)
{
    KpKysely *kysely = kitsas()->yhteysModel()->kysely("/viennit");
    if( !kysely ) return;

    kysely->lisaaAttribuutti("tili", tilinumero());
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);
    kysely->lisaaAttribuutti("vastatilit");

    connect(kysely, &KpKysely::vastaus, this, &TilioteModel::harmaatSaapuu);
    harmaaLaskuri_++;

    kysely->kysy();
}

QVariantList TilioteModel::viennit() const
{
    QVariantList ulos;
    for(const auto& rivi : kirjausRivit_) {
        if( !rivi.peitetty() && rivi.summa()) {
           ulos << rivi.viennit(tili_);
        }
    }
    return ulos;
}

void TilioteModel::tuo(const QVariantList tuotavat)
{
    if( kirjausRivit_.count() == 1 && kirjausRivit_.first().summa() == Euro::Zero ) {
        beginResetModel();
        kirjausRivit_.clear();
        endResetModel();
    }

    for(const auto& tuotava : tuotavat) {
        kirjausRivit_.append(TilioteKirjausRivi(tuotava.toMap(), this));
    }
}

QPair<qlonglong, qlonglong> TilioteModel::summat() const
{
    Euro panot, otot;
    for(const auto& rivi : kirjausRivit_) {
        if( rivi.peitetty()) continue;
        const Euro summa = rivi.summa();
        if( summa > Euro::Zero)
            panot += summa;
        else
            otot += summa.abs();
    }
    for(const auto& rivi : harmaatRivit_) {
        panot += rivi.vienti().debetEuro();
        otot +=  rivi.vienti().kreditEuro();
    }
    return qMakePair(panot, otot);
}

void TilioteModel::salliMuokkaus(bool sallittu)
{
    muokkausSallittu_ = sallittu;
}

void TilioteModel::harmaatSaapuu(QVariant *data)
{
    harmaaLaskuri_--;
    if( harmaaLaskuri_)
        return;

    beginResetModel();
    harmaatRivit_.clear();

    const QList lista = data->toList();

    for (const auto& vienti : lista) {
        QVariantMap map = vienti.toMap();
        if(map.value("tosite").toMap().value("id").toInt() == tositeId())
            continue;
        harmaatRivit_.append(TilioteHarmaaRivi(map, this));
    }

    peitaHarmailla();

    endResetModel();
}

void TilioteModel::peita(int harmaaIndeksi, int kirjausIndeksi)
{
    harmaatRivit_[harmaaIndeksi].asetaLisaysIndeksi(kirjausRivit_.at(kirjausIndeksi).lisaysIndeksi());
    kirjausRivit_[kirjausIndeksi].peita(true);
}

void TilioteModel::peitaHarmailla(int harmaaIndeksi)
{
    const TositeVienti& harmaa = harmaatRivit_.at(harmaaIndeksi).vienti();
    Euro harmaaSumma = harmaa.debetEuro() - harmaa.kreditEuro();
    const QString& arkistotunnus = harmaa.arkistotunnus();

    // Ensin etsitään arkistotunnuksella
    if( !arkistotunnus.isEmpty()) {
        for(int ki=0; ki < kirjausRivit_.count(); ki++) {
            const TilioteKirjausRivi& rivi = kirjausRivit_.at(ki);
            if( rivi.peitetty()) continue;
            if( rivi.arkistotunnus() == arkistotunnus) {
                peita(harmaaIndeksi, ki);
                return;
            }
        }
    }
    // Sitten päiväyksen ja määrän yhdistelmällä
    QList<int> sopivat;

    for(int ki=0; ki < kirjausRivit_.count(); ki++) {
        const TilioteKirjausRivi& rivi = kirjausRivit_.at(ki);
        if( rivi.peitetty()  || !rivi.tuotu() ) continue;        
        if( rivi.pvm() == harmaa.pvm() &&
            rivi.summa() == harmaaSumma ){
            sopivat.append(ki);
        }
    }

    if( sopivat.isEmpty()) {
        // Kokeillaan ostopäivää
        if( !harmaa.ostopvm().isValid()) {
            return;
        }
        for(int ki=0; ki < kirjausRivit_.count(); ki++) {
            const TilioteKirjausRivi& rivi = kirjausRivit_.at(ki);
            if( rivi.peitetty()  || !rivi.tuotu() ) continue;            
            if( rivi.pvm() == harmaa.ostopvm() &&
                rivi.summa() == harmaaSumma ) {
                sopivat.append(ki);
            }
        }
    }
    if( sopivat.isEmpty()) {
        return;
    }

    if( sopivat.count() == 1 ) {
        peita(harmaaIndeksi, sopivat.first());
        return;
    }

    // Sitten sopivalla kumppanilla
    QList<int> kumppaniSopivat;

    for(int sopiva : sopivat) {
        const TilioteKirjausRivi& rivi = kirjausRivit_.at(sopiva);
        if(rivi.kumppani().value("id").toInt() == harmaa.kumppaniId())
            kumppaniSopivat.append(sopiva);
    }

    if( kumppaniSopivat.count() == 1) {
        peita(harmaaIndeksi, kumppaniSopivat.first());
        return;
    }

    // Ja viimeiseksi selitteellä
    QList<int> seliteSopivat;

    for(int sopiva : sopivat) {
        const TilioteKirjausRivi& rivi = kirjausRivit_.at(sopiva);
        if(rivi.otsikko().toLower() == harmaa.selite().toLower())
            seliteSopivat.append(sopiva);
    }

    if( seliteSopivat.count() == 1)
        peita(harmaaIndeksi, seliteSopivat.first());
}

void TilioteModel::peitaHarmailla()
{
    for(int ki=0; ki < kirjausRivit_.count(); ki++) {
        kirjausRivit_[ki].peita(false);
    }
    for(int hi=0; hi < harmaatRivit_.count(); hi++) {
        peitaHarmailla(hi);
    }
}

void TilioteModel::alkuperaisTositeSaapuu(int lisaysIndeksi, QVariant *data, int eraId)
{
    for(int i=0; i < kirjausRivit_.count(); i++) {
        const int indeksi = kirjausRivit_.at(i).lisaysIndeksi();
        if( indeksi == lisaysIndeksi) {
            kirjausRivit_[i].alkuperaistositeSaapuu(data, eraId);
            break;
        }
    }
}
