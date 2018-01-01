/*
   Copyright (C) 2017,2018 Arto Hyvättinen

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
#include <QSqlError>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QIcon>

#include "tilimodel.h"
#include "tili.h"
#include "vientimodel.h"
#include "tilityyppimodel.h"
#include "kirjanpito.h"

TiliModel::TiliModel(QSqlDatabase *tietokanta, QObject *parent) :
    QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int TiliModel::rowCount(const QModelIndex & /* parent */) const
{
    return tilit_.count();
}

int TiliModel::columnCount(const QModelIndex & /* parent */) const
{
    return 5;
}

bool TiliModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

    if( role == TiliModel::TilaRooli)
    {
        tilit_[index.row()].asetaTila(value.toInt());

        emit dataChanged(index.sibling(index.row(), 0 ), index.sibling(index.row(), columnCount(QModelIndex())) );
    }
    else if( role == TiliModel::NroRooli)
    {
        tilit_[ index.row()].asetaNumero( value.toInt());
    }
    else if( role == TiliModel::NimiRooli)
    {
        tilit_[index.row()].asetaNimi( value.toString());
    }
    else if( role == TiliModel::TyyppiRooli)
    {
        tilit_[index.row()].asetaTyyppi( value.toString());
    }
    else
        return false;

    emit dataChanged( index.sibling(index.row(), 0), index.sibling(index.row(), 4)) ;

    return false;
}

QVariant TiliModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case NRONIMI :
            return QVariant("Numero ja nimi");
        case NUMERO:
            return QVariant("Numero");
        case NIMI :
            return QVariant("Nimi");
        case TYYPPI:
            return QVariant("Tilityyppi");
        case ALV:
            return QVariant("Alv");
        }
    }
    return QVariant();

}

QVariant TiliModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())     
        return QVariant();

    Tili tili = tilit_.value(index.row());

    if( role == IdRooli )
        return QVariant( tili.id());
    else if( role == NroRooli )
        return QVariant( tili.numero());
    else if( role == NimiRooli )
        return QVariant( tili.nimi());
    else if( role == NroNimiRooli)
        return QVariant( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));
    else if( role == OtsikkotasoRooli)
        return QVariant( tili.otsikkotaso());
    else if( role == TyyppiRooli )
        return QVariant( tili.tyyppiKoodi());
    else if( role == YsiRooli)
        return QVariant( tili.ysivertailuluku());
    else if( role == TilaRooli)
        return QVariant( tili.tila());

    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case NRONIMI :
            return QVariant( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));
        case NUMERO:
            return QVariant( tili.numero());
        case NIMI :
            return QVariant( tili.nimi());
        case TYYPPI:
            if( tili.otsikkotaso() )
            {
                // Otsikoiden tyyppitekstinä on tieto otsikon tasosta sisennettynä
                QString otsikkotxt;
                for(int i=0; i<tili.otsikkotaso() - 1; i++)
                    otsikkotxt += " ";
                otsikkotxt += tr("Otsikko %1").arg(tili.otsikkotaso());
                return QVariant( otsikkotxt );
            }
            return QVariant( kp()->tiliTyypit()->tyyppiKoodilla( tili.tyyppiKoodi() ).kuvaus() );
        case ALV:
            int vero = tili.json()->luku("AlvProsentti");
            if(vero)
                return QVariant( QString("%1 %").arg(vero));
            return QVariant();
        }
    }

    else if( role == Qt::TextColorRole)
    {
        if( tili.tila() == 0 )
            return QColor(Qt::darkGray);
        else
            return QColor(Qt::black);
    }
    else if( role == Qt::DecorationRole && index.column() == NIMI)
    {
        if( tili.tila() == 0)
            return QIcon(":/pic/eikaytossa.png");
        else if( tili.tila() == 2)
            return QIcon(":/pic/tahti.png");
    }
    else if( role == Qt::DecorationRole && index.column() == ALV)
    {
        return kp()->alvTyypit()->kuvakeKoodilla( tili.json()->luku("AlvLaji") );
    }
    else if( role == Qt::DecorationRole && index.column() == NUMERO )
    {
        if( !tili.json()->str("Kirjausohje").isEmpty())
            return QIcon(":/pic/info.png");     // Tiliin on olemassa kirjausohje
    }

    else if( role == Qt::FontRole)
    {
        QFont fontti;
        if( tili.otsikkotaso())
            fontti.setBold(true);
        return QVariant( fontti );
    }

    return QVariant();
}

void TiliModel::lisaaTili(Tili uusi)
{
    beginInsertRows( QModelIndex(), tilit_.count(), tilit_.count()  );
    tilit_.append(uusi);
    // TODO - lisätään oikeaan paikkaan kasiluvun mukaan
    endInsertRows();
}

void TiliModel::poistaRivi(int riviIndeksi)
{
    Tili tili = tilit_[riviIndeksi];
    if( tili.montakoVientia())
        return;         // Ei voi poistaa, jos kirjauksia

    beginRemoveRows( QModelIndex(), riviIndeksi, riviIndeksi);
    if( tili.id() )
        poistetutIdt_.append( tili.id());

    tilit_.removeAt(riviIndeksi);
    endRemoveRows();

}

Tili TiliModel::tiliIdlla(int id) const
{
    foreach (Tili tili, tilit_)
    {
        if( tili.id() == id)
            return tili;
    }
    return Tili();
}

Tili TiliModel::tiliNumerolla(int numero) const
{

    foreach (Tili tili, tilit_)
    {
        // Vertailu tehdään "ysiluvuilla" joten tilit 154 ja 15400 ovat samoja
        if( tili.ysivertailuluku() == Tili::ysiluku(numero, 0) )
            return tili;
    }
    return Tili();
}

Tili TiliModel::edellistenYlijaamaTili() const
{
    foreach (Tili tili, tilit_)
    {
        if( tili.onko(TiliLaji::EDELLISTENTULOS) )
            return tili;
    }
    return Tili();
}


Tili TiliModel::tiliTyypilla(TiliLaji::TiliLuonne luonne) const
{
    foreach (Tili tili, tilit_) {
        if( tili.onko(luonne))
            return tili;
    }
    return Tili();
}

JsonKentta *TiliModel::jsonIndeksilla(int i)
{
    return tilit_[i].json();
}

bool TiliModel::onkoMuokattu() const
{
    if( poistetutIdt_.count())  // Tallennettuja rivejä poistettu
        return true;

    foreach (Tili tili, tilit_)
    {
        if( tili.muokattu())
            return true;        // Tosi, jos yhtäkin tiliä muokattu
    }
    return false;
}

void TiliModel::lataa()
{
    beginResetModel();
    tilit_.clear();

    QSqlQuery kysely( *tietokanta_ );
    kysely.exec("SELECT id, nro, nimi, tyyppi, tila, json, ysiluku "
                " FROM tili ORDER BY ysiluku");

    QVector<int> otsikkoId(10);

    QVector<Tili> otsikot(10);

    while(kysely.next())
    {
        int otsikkotaso = 0;
        QString tyyppikoodi = kysely.value(3).toString();
        if( tyyppikoodi.startsWith('H'))    // Tyyppikoodi H1 tarkoittaa 1-tason otsikkoa jne.
            otsikkotaso = tyyppikoodi.mid(1).toInt();

        int id = kysely.value(0).toInt();
        int otsikkoIdTalle = 0; // Nykytilille merkittävä otsikkotaso
        int ysiluku = kysely.value(6).toInt();

        // Etsitään otsikkotasoa tasojen lopusta alkaen
        for(int i=9; i >= 0; i--)
        {
            if( otsikot.at(i).onkoValidi() && otsikot.at(i).ysivertailuluku() <= ysiluku && otsikot[i].json()->luku("Asti") >= ysiluku )
            {
                otsikkoIdTalle = otsikot.at(i).id();
                break;
            }
        }

        Tili uusi( id,     // id
                   kysely.value(1).toInt(),     // nro
                   kysely.value(2).toString(),  // nimi
                   tyyppikoodi,  // tyyppi
                   kysely.value(4).toInt(),     // tila
                   otsikkoIdTalle    // Tätä tiliä/otsikkoa ylemmän otsikon id
                   );
        uusi.json()->fromJson( kysely.value(5).toByteArray());  // Luetaan json-kentät
        uusi.nollaaMuokattu();
        tilit_.append(uusi);

        if( otsikkotaso )
            otsikot[otsikkotaso] = uusi;

    }
    endResetModel();
}

void TiliModel::tallenna()
{
    tietokanta_->transaction();
    QSqlQuery kysely(*tietokanta_);
    for( int i=0; i < tilit_.count() ; i++)
    {
        Tili tili = tilit_[i];
        if( tili.onkoValidi() && tili.muokattu() )
        {
            if( tili.id())
            {
                // Muokkaus
                kysely.prepare("UPDATE tili SET nro=:nro, nimi=:nimi, tyyppi=:tyyppi, "
                               "tila=:tila, ysiluku=:ysiluku, json=:json "
                               "WHERE id=:id");
                kysely.bindValue(":id", tili.id());
            }
            else
            {
                // Tallennus
                kysely.prepare("INSERT INTO tili(nro, nimi, tyyppi, tila, ysiluku, json) "
                               "VALUES(:nro, :nimi, :tyyppi, :tila, :ysiluku, :json) ");

            }
            kysely.bindValue(":nro", tili.numero());
            kysely.bindValue(":nimi", tili.nimi());
            kysely.bindValue(":tyyppi", tili.tyyppiKoodi());
            kysely.bindValue(":tila", tili.tila());
            kysely.bindValue(":ysiluku", tili.ysivertailuluku());
            kysely.bindValue(":json", tilit_[i].json()->toSqlJson());

            if( kysely.exec() )
                tilit_[i].nollaaMuokattu();

            if( !tili.id())
                tilit_[i].asetaId( kysely.lastInsertId().toInt() );

        }
    }

    foreach (int id, poistetutIdt_)
    {
        kysely.exec( QString("DELETE FROM tili WHERE id=%1").arg(id) );
    }
    poistetutIdt_.clear();

    tietokanta_->commit();
}

