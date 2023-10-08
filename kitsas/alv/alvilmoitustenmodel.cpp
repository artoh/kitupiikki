/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QSqlQuery>

#include "alvilmoitustenmodel.h"
#include "db/kirjanpito.h"

#include "verovarmennetila.h"
#include "alvkaudet.h"

#include "pilvi/pilvimodel.h"

AlvIlmoitustenModel::AlvIlmoitustenModel(QObject *parent)
    : QAbstractTableModel(parent),
      varmenneTila_{new VeroVarmenneTila(this)},
      kaudet_{new AlvKaudet(this)}
{
    connect( varmenneTila_, &VeroVarmenneTila::kaytossa, kaudet_, &AlvKaudet::haeKaudet);
    connect( kaudet_, &AlvKaudet::haettu, this, &AlvIlmoitustenModel::kaudetPaivitetetty);
}

int AlvIlmoitustenModel::rowCount(const QModelIndex &/*parent*/) const
{
    return ilmoitukset_.count();
}

int AlvIlmoitustenModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 5;
}

QVariant AlvIlmoitustenModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
    {
            return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    }
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case ALKAA:
            return tr("Alkaa");
        case PAATTYY:
            return tr("Päättyy");
        case ERAPVM:
            return tr("Eräpäivä");
        case VERO:
            return tr("Maksettava vero");
        case TILA:
            return tr("Ilmoituksen tila");
        }
    }
    return QVariant();
}

QVariant AlvIlmoitustenModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
    {
        return QVariant();
    }

    const AlvIlmoitusTieto tieto = ilmoitukset_.at(index.row());


    if( role == Qt::DisplayRole )
    {
        switch (index.column()) {
        case ALKAA:
            return tieto.alkaa();
        case PAATTYY:
            return tieto.paattyy();
        case ERAPVM:
            return erapaiva( tieto.paattyy() );
        case VERO:
            return tieto.maksettava().display();
        case TILA: {
                AlvKausi kausi = kaudet_->kausi(tieto.paattyy());
                if( kausi.tila() == AlvKausi::PUUTTUVA && tieto.ilmoitettu().isValid()) {
                    return tr("Ilmoitettu %1").arg(tieto.ilmoitettu().toString("dd.MM.yyyy"));
                } else {
                    return kausi.tilaInfo();
                }
            }
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==VERO)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == TositeIdRooli )
        return tieto.tositeId();
    else if( role == PaattyyRooli)
        return  tieto.paattyy();
    else if( role == EraPvmRooli)
        return erapaiva( tieto.paattyy() );
    else if( role == AlkaaRooli)
        return tieto.alkaa();
    else if( role == MapRooli)
        return tieto.map();
    else if( role == Qt::DecorationRole && index.column() == TILA) {
        AlvKausi kausi = kaudet_->kausi(tieto.paattyy());

        if( kausi.tila() == AlvKausi::KASITELTY) return QIcon(":/pic/kaytossa.png");
        else if( kausi.tila() == AlvKausi::KASITTELYSSA) return QIcon(":/pic/keltainen.png");
        else if( kausi.tila() == AlvKausi::PUUTTUVA && tieto.ilmoitettu().isValid()) return QIcon(":/pic/keltainen.png");
        else if( kausi.tila() == AlvKausi::PUUTTUVA && kausi.erapvm() < QDate::currentDate() ) return QIcon(":/pic/punainen.png");
        else return QIcon(":/pic/tyhja.png");
    }

    return QVariant();

}

qlonglong AlvIlmoitustenModel::marginaalialijaama(const QDate &paiva, int kanta) const
{
    for (const auto& item : qAsConst(ilmoitukset_)) {
        if( item.paattyy() != paiva)
            continue;        
        const double aj = item.marginaaliAliJaama().value(QString::number(kanta/100.0,'f',2)).toDouble();
        return qRound64(aj * 100.0);
    }
    return 0;
}

bool AlvIlmoitustenModel::onkoIlmoitettu(const QDate &paiva) const
{
    for( const auto& item : qAsConst(ilmoitukset_)) {
        if( item.alkaa() <= paiva && item.paattyy() >= paiva)
            return true;
    }
    return false;
}

QDate AlvIlmoitustenModel::viimeinenIlmoitus() const
{
    QDate viimeinen = kp()->asetukset()->pvm("AlvAlkaa", kp()->tilitpaatetty()).addDays(-1);
    for(const auto& item : qAsConst(ilmoitukset_)) {
        if( item.paattyy() > viimeinen)
            viimeinen = item.paattyy();
    }
    return viimeinen;
}

QDate AlvIlmoitustenModel::erapaiva(const QDate &loppupaiva) const
{
    // Haetaan ensisijaisesti verohallinnon ilmoituksen mukaisesti ;)
    AlvKausi alvkausi = kaudet_->kausi(loppupaiva);
    if( alvkausi.erapvm().isValid())
        return alvkausi.erapvm();

    QDate erapvm = loppupaiva.addDays(1).addMonths(1).addDays(11);

    if( kp()->asetukset()->luku("AlvKausi") == 12 )
        erapvm = loppupaiva.addMonths(2);

    // Ei eräpäivää viikonloppuun
    while( erapvm.dayOfWeek() > 5)
        erapvm = erapvm.addDays(1);

    return erapvm;
}

void AlvIlmoitustenModel::lataa()
{
    KpKysely *kysely = kpk("/alv");
    if( !kysely)
        return;

    kaudet_->tyhjenna();
    connect( kysely, &KpKysely::vastaus, this, &AlvIlmoitustenModel::dataSaapuu);
    kysely->kysy();

    if( qobject_cast<PilviModel*>(kp()->yhteysModel()) &&
        kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET | YhteysModel::ALV_ILMOITUS)) {
        varmenneTila_->paivita();
    } else {
        varmenneTila_->tyhjenna();        
    }
}

void AlvIlmoitustenModel::dataSaapuu(QVariant *data)
{
    QVariantList list = data->toList();

    beginResetModel();
    ilmoitukset_.clear();

    for(const auto& item : qAsConst(list)) {
        ilmoitukset_.append(AlvIlmoitusTieto(item.toMap()));
    }

    endResetModel();
}

void AlvIlmoitustenModel::kaudetPaivitetetty()
{
    emit dataChanged( index(0,TILA), index(rowCount()-1, TILA));
}


AlvIlmoitustenModel::AlvIlmoitusTieto::AlvIlmoitusTieto()
{

}

AlvIlmoitustenModel::AlvIlmoitusTieto::AlvIlmoitusTieto(const QVariantMap &data)
{
    tositeId_ = data.value("id").toInt();
    alkaa_ = data.value("kausialkaa").toDate();
    paattyy_ = data.value("kausipaattyy").toDate();
    maksettava_ = Euro::fromString(data.value("maksettava").toString());
    marginaaliAliJaama_ = data.value("marginaalialijaama").toMap();
    ilmoitettu_ = data.value("ilmoitettu").toDateTime();
}

QVariantMap AlvIlmoitustenModel::AlvIlmoitusTieto::map() const
{
    QVariantMap map;
    map.insert("id", tositeId_);
    map.insert("kausialkaa", alkaa_);
    map.insert("kausipaattyy", paattyy_);
    map.insert("maksettava", maksettava_.toString());
    map.insert("marginaalialijaama", marginaaliAliJaama_);
    return map;
}
