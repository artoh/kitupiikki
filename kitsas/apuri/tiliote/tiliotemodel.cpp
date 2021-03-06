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

TilioteModel::TilioteModel(QObject *parent, KitsasInterface *kitsasInterface)
    : VanhaTilioteModel(parent),
      kitsasInterface_(kitsasInterface)
    // : QAbstractTableModel(parent)
{
}

QVariant TilioteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case PVM:
            return tr("Pvm");
        case EURO:
            return tr("Euro");
        case TILI:
            return tr("Tili");
        case KOHDENNUS:
            return tr("Kohdennus");
        case SAAJAMAKSAJA:
            return tr("Saaja/Maksaja");
        case SELITE:
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
    return 6;
}

QVariant TilioteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int rivi = index.row();
    int kirjausriveja = kirjausRivit_.count();

    if( rivi < kirjausriveja )
        return kirjausRivit_.at(rivi).riviData(index.column(), role);
    else
        return harmaatRivit_.at(rivi - kirjausriveja).riviData(index.column(), role);
}

bool TilioteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( index.row() < kirjausRivit_.count()) {
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

int TilioteModel::lisaaRivi(const QDate &pvm)
{
    return lisaaRivi(TilioteKirjausRivi(pvm, this));
}

int TilioteModel::lisaaRivi(const TilioteKirjausRivi &rivi)
{
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

void TilioteModel::asetaRivi(int indeksi, const QList<TositeVienti> &viennit)
{
    kirjausRivit_[indeksi].asetaViennit(viennit);
    emit dataChanged( index(indeksi, TilioteRivi::PVM),
                      index(indeksi, TilioteRivi::EURO),
                      QVector<int>() << Qt::EditRole);
}

TilioteKirjausRivi TilioteModel::rivi(const int indeksi) const
{
    return kirjausRivit_.value(indeksi);
}

void TilioteModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    kirjausRivit_.clear();

    QVariantList rivinlista;
    for(auto const& listanrivi : lista) {
        int rivintili = listanrivi.toMap().value("tili").toInt();
        if( rivintili == tilinumero() && rivinlista.count() > 1) {
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
    for(int i=0; i < kirjausRivit_.count(); i++)
        kirjausRivit_[i].asetaPankkitili(tilinumero);
}

int TilioteModel::lisaysIndeksi()
{
    return ++lisaysIndeksi_;
}

void TilioteModel::lataaHarmaat(const QDate &mista, const QDate &mihin)
{
    harmaaLaskuri_++;
    KpKysely *kysely = kitsas()->yhteysModel()->kysely("/viennit");
    kysely->lisaaAttribuutti("tili", tilinumero());
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);
    kysely->lisaaAttribuutti("vastatilit");

    connect(kysely, &KpKysely::vastaus, this, &TilioteModel::harmaatSaapuu);
    kysely->kysy();
}

QVariantList TilioteModel::viennit() const
{
    QVariantList ulos;
    for(const auto& rivi : kirjausRivit_) {
        if( !rivi.peitetty()) {
            ulos << rivi.tallennettavat();
        }
    }
    return ulos;
}

void TilioteModel::tuo(const QVariantList tuotavat)
{
    for(const auto& tuotava : tuotavat) {
        kirjausRivit_.append(TilioteKirjausRivi(tuotava.toMap(), this));
    }
}

QPair<qlonglong, qlonglong> TilioteModel::summat() const
{
    qlonglong panot = 0l, otot = 0l;
    for(const auto& rivi : kirjausRivit_) {
        if( rivi.peitetty()) continue;
        panot += qRound64( rivi.pankkivienti().debet() * 100.0 );
        otot += qRound64( rivi.pankkivienti().kredit() * 100.0 );
    }
    for(const auto& rivi : harmaatRivit_) {
        panot += qRound64( rivi.vienti().debet() * 100.0 );
        otot += qRound64( rivi.vienti().kredit() * 100.0 );
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

    for (const auto& vienti : data->toList()) {
        QVariantMap map = vienti.toMap();
        if(map.value("tosite").toMap().value("tyyppi").toInt() == TositeTyyppi::TILIOTE)
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
    const QString& arkistotunnus = harmaa.arkistotunnus();

    // Ensin etsitään arkistotunnuksella
    if( !arkistotunnus.isEmpty()) {
        for(int ki=0; ki < kirjausRivit_.count(); ki++) {
            const TilioteKirjausRivi& rivi = kirjausRivit_.at(ki);
            if( rivi.peitetty()) continue;
            if( rivi.pankkivienti().arkistotunnus() == arkistotunnus) {
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
        const TositeVienti& kirjaus = rivi.pankkivienti();
        if( kirjaus.pvm() == harmaa.pvm() &&
            qAbs(kirjaus.debet() - harmaa.debet()) < 1e-5 &&
            qAbs(kirjaus.kredit() - harmaa.kredit()) < 1e-5) {
            sopivat.append(ki);
        }
    }

    if( sopivat.isEmpty()) return;
    if( sopivat.count() == 1 ) {
        peita(harmaaIndeksi, sopivat.first());
        return;
    }

    // Sitten sopivalla kumppanilla
    QList<int> kumppaniSopivat;

    for(int sopiva : sopivat) {
        const TositeVienti& kirjaus = kirjausRivit_.at(sopiva).pankkivienti();
        if(kirjaus.kumppaniId() == harmaa.kumppaniId())
            kumppaniSopivat.append(sopiva);
    }

    if( kumppaniSopivat.count() == 1) {
        peita(harmaaIndeksi, kumppaniSopivat.first());
        return;
    }

    // Ja viimeiseksi selitteellä
    QList<int> seliteSopivat;

    for(int sopiva : sopivat) {
        const TositeVienti& kirjaus = kirjausRivit_.at(sopiva).pankkivienti();
        if(kirjaus.selite().toLower() == harmaa.selite().toLower())
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
