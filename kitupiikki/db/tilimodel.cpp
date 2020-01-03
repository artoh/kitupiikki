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

#include <QMessageBox>

#include "tilimodel.h"
#include "tili.h"
#include "tilityyppimodel.h"
#include "kirjanpito.h"
#include "kielikentta.h"

#include "laskutus/myyntilaskuntulostaja.h"

TiliModel::TiliModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int TiliModel::rowCount(const QModelIndex & /* parent */) const
{
    return tiliLista_.count();
}

int TiliModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
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
            return tr("Numero ja nimi");
        case NUMERO:
            return tr("Numero");
        case NIMI :
            return tr("Nimi");
        case TYYPPI:
            return tr("Tilityyppi");
        case ALV:
            return tr("Alv");
        case SALDO:
            return tr("Saldo");
        }
    }
    return QVariant();

}

QVariant TiliModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())     
        return QVariant();

    Tili* tili = tiliLista_.value(index.row());

    if( role == IdRooli ) {
        return QVariant( tili->id());
    } else if( role == NroRooli )
        return QVariant( tili->numero());
    else if( role == NimiRooli )
        return QVariant( tili->nimi());
    else if( role == NroNimiRooli)
        return tili->nimiNumero();
    else if( role == OtsikkotasoRooli)
        return tili->otsikkotaso();
    else if( role == TyyppiRooli )
        return QVariant( tili->tyyppiKoodi());
    else if( role == TilaRooli)
        return tili->tila();
    else if( role == OhjeRooli)
        return tili->ohje();
    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case NRONIMI :
            if( tili->onko(TiliLaji::PANKKITILI))
                return tili->nimiNumero() + " " + MyyntiLaskunTulostaja::valeilla( tili->str("IBAN") );
            return tili->nimiNumero();
        case NUMERO:
            if( tili->otsikkotaso())
                return QString();
            return QVariant( tili->numero());
        case NIMI : {
            QString sisennys;
            for(int i=0; i < tili->otsikkotaso() - 1; i++)
                sisennys += "  ";
            return sisennys + tili->nimi();
        }
        case TYYPPI:
            if( tili->otsikkotaso() )
                return QString();
            return QVariant( kp()->tiliTyypit()->tyyppiKoodilla( tili->tyyppiKoodi() ).kuvaus() );
        case ALV: {
            int vero = tili->luku("alvprosentti");
            if(vero)
                return QVariant( QString("%1 %").arg(vero));
            return QVariant();
        }
        case SALDO:
            if( saldot_.contains( tili->numero() ))
                return QString("%L1").arg( saldot_.value( tili->numero() ), 0, 'f', 2 );
            return QString();
        }
    }

    else if( role == Qt::DecorationRole && index.column() == NIMI)
    {
        if( tili->tila() == 0)
            return QIcon(":/pic/eikaytossa.png");
        else if( tili->tila() == 2)
            return QIcon(":/pic/tahti.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == Qt::DecorationRole && index.column() == ALV)
    {
        return kp()->alvTyypit()->kuvakeKoodilla( tili->luku("alvlaji") );
    }
    else if( role == Qt::DecorationRole && index.column() == NUMERO )
    {
        if( !tili->ohje().isEmpty())
            return QIcon(":/pic/info.png");     // Tiliin on olemassa kirjausohje
        else
            return QIcon(":/pic/tyhja.png");
    }

    else if( role == Qt::FontRole)
    {
        QFont fontti;
        if( tili->otsikkotaso())
            fontti.setBold(true);
        return QVariant( fontti );
    }

    return QVariant();
}

Tili *TiliModel::lisaaTili(int numero, int otsikkotaso)
{
    // Etsitään tilille paikka
    int i;
    QString nrostr = QString::number(numero);
    Tili *edellinenotsikko = nullptr;
    for(i=0; i<rowCount()-1; i++) {
        Tili* tili = tiliPIndeksilla(i);
        if( QString::number(tili->numero()) > nrostr || ( tili->numero() == numero && tili->otsikkotaso() > otsikkotaso)  )
            break;
        if( tili->otsikkotaso() && ( !otsikkotaso || tili->otsikkotaso() < otsikkotaso))
            edellinenotsikko = tili;
    }
    // Nyt indeksi kertoo, minne lisätään
    Tili* tili = new Tili();
    tili->asetaNumero(numero);
    tili->asetaOtsikko(edellinenotsikko);
    if( otsikkotaso)
        tili->asetaTyyppi(QString("H%1").arg(otsikkotaso));

    beginInsertRows(QModelIndex(), i, i);
    tiliLista_.insert(i, tili);

    if( !otsikkotaso)
        nroHash_.insert(numero, tili);

    endInsertRows();
    return tili;
}

void TiliModel::tallenna(Tili* tili)
{
    int indeksi = tiliLista_.indexOf(tili);

    KpKysely* kysely = kpk( tili->otsikkotaso() ?
                                QString("/tilit/%1/%2").arg(tili->numero()).arg(tili->otsikkotaso())
                              : QString("/tilit/%1").arg(tili->numero()),
                            KpKysely::PUT);
    connect( kysely, &KpKysely::vastaus,
             [this,indeksi] () { emit this->dataChanged(this->index(indeksi,0), this->index(indeksi,SALDO)); });
    connect( kysely, &KpKysely::virhe,
             [](int, const QString& selitys) { QMessageBox::critical(nullptr,tr("Virhe tallentamisessa"), tr("Tilin tallentaminen epäonnistui.\n%1").arg(selitys)); } );

    kysely->kysy( tili->data() );

}


void TiliModel::poistaRivi(int riviIndeksi)
{
    beginRemoveRows( QModelIndex(), riviIndeksi, riviIndeksi);
    Tili* tili = tiliPIndeksilla(riviIndeksi);
    if( !tili->otsikkotaso())
        nroHash_.remove( tili->numero() );
    tiliLista_.removeAt(riviIndeksi);

    KpKysely* kysely = kpk( tili->otsikkotaso() ?
                                QString("/tilit/%1/%2").arg(tili->numero()).arg(tili->otsikkotaso())
                              : QString("/tilit/%1").arg(tili->numero()),
                            KpKysely::DELETE);
    kysely->kysy();
    delete tili;

    endRemoveRows();
}


Tili *TiliModel::tili(const QString &tilinumero) const
{
    return tili( tilinumero.toInt());
}

Tili *TiliModel::tili(int numero) const
{
    return nroHash_.value(numero);
}

Tili TiliModel::tiliNumerolla(int numero, int otsikkotaso) const
{
    for(auto tili : tiliLista_)
        if( tili->numero()==numero && tili->otsikkotaso() == otsikkotaso)
            return *tili;
    return Tili();
}

Tili TiliModel::tiliIbanilla(const QString &iban) const
{
    for(Tili* tili: tiliLista_)
    {
        if( tili->str("IBAN") == iban)
            return *tili;
    }
    return Tili();
}


Tili TiliModel::tiliTyypilla(TiliLaji::TiliLuonne tyyppi) const
{
    foreach (Tili* tili, tiliLista_) {
        if( tili->tyyppi().luonne() == tyyppi)
            return *tili;
    }
    return Tili();
}

Tili TiliModel::tiliTyypilla(const QString &tyyppikoodi) const
{
    for( Tili* tili : tiliLista_) {
       if( tili->tyyppiKoodi() == tyyppikoodi)
           return *tili;
    }
    return Tili();
}

QStringList TiliModel::laskuTilit() const
{
    // Tämä siirtyy toisaalle
    return QStringList();
}


void TiliModel::lataa(QVariantList lista)
{
    beginResetModel();
    tyhjenna();


    QVector<Tili*> otsikot(10);
    otsikot.fill(nullptr);
    int ylinotsikkotaso = 0;

    for( QVariant variant : lista)
    {
        QVariantMap map = variant.toMap();

        int otsikkotaso = 0;
        Tili *tili = new Tili( map );
        int nro = map.value("numero").toInt();

        QString tyyppikoodi = map.value("tyyppi").toString();
        if( tyyppikoodi.startsWith('H')) {   // Tyyppikoodi H1 tarkoittaa 1-tason otsikkoa jne.
            otsikkotaso = tyyppikoodi.midRef(1).toInt();
            otsikot[otsikkotaso] = tili;
            tili->asetaOtsikko( otsikot.at(otsikkotaso - 1) );
            ylinotsikkotaso = otsikkotaso;
        } else {
            tili->asetaOtsikko( otsikot.at(ylinotsikkotaso) );
            nroHash_.insert( nro, tili );
        }

        tiliLista_.append( tili );

    }

    piilotetut_.clear();
    suosikit_.clear();

    // Samalla ladataan piilotukset ja suosikit
    for(auto piilo: kp()->asetus("piilotilit").split(","))
        piilotetut_.insert(piilo.toInt());
    for(auto suosikki: kp()->asetus("suosikkitilit").split(","))
        suosikit_.insert(suosikki.toInt());


    paivitaTilat();

    endResetModel();
}


void TiliModel::asetaSuosio(int tili, Tili::TiliTila tila)
{

    if( tila == Tili::TILI_PIILOSSA)
        piilotetut_.insert(tili);
    else
        piilotetut_.remove(tili);
    if( tila == Tili::TILI_SUOSIKKI)
        suosikit_.insert(tili);
    else
        suosikit_.remove(tili);

    // Tallennetaan suosiot
    QStringList piilolista;
    for( int piilossa : piilotetut_) {
        piilolista.append( QString::number(piilossa) );
    }
    kp()->asetukset()->aseta("piilotilit", piilolista.join(","));

    QStringList suosikkilista;
    for( int suosiossa : suosikit_) {
        suosikkilista.append( QString::number(suosiossa) );
    }
    kp()->asetukset()->aseta("suosikkitilit", suosikkilista.join(","));


    paivitaTilat();
    emit dataChanged( index(0,0), index(rowCount()-1, SALDO), QVector<int>() << TilaRooli );
}

void TiliModel::haeSaldot()
{
    KpKysely *saldokysely = kpk("/saldot");
    saldokysely->lisaaAttribuutti("pvm", kp()->paivamaara());
    connect( saldokysely, &KpKysely::vastaus, this, &TiliModel::saldotSaapuu);
    saldokysely->kysy();
}

void TiliModel::saldotSaapuu(QVariant *saldot)
{
    QVariantMap map = saldot->toMap();
    QMapIterator<QString,QVariant> iter(map);
    saldot_.clear();

    while( iter.hasNext()) {
        iter.next();
        saldot_.insert( iter.key().toInt(), iter.value().toDouble() );
    }
    emit dataChanged( index(0, SALDO), index( rowCount()-1, SALDO )  );
}

void TiliModel::tyhjenna()
{
    for(Tili *tili : tiliLista_)
        delete tili;

    tiliLista_.clear();
    nroHash_.clear();
}

void TiliModel::paivitaTilat()
{

    int laajuus = kp()->asetukset()->luku("laajuus",3);
    QString muoto = kp()->asetukset()->asetus("muoto");

    for(Tili* tili : tiliLista_) {
        if( tili->otsikkotaso()) {
            tili->asetaTila(Tili::TILI_PIILOSSA);
            continue;
        }
        int numero = tili->numero();

        if( suosikit_.contains(numero)  )
            tili->asetaTila(Tili::TILI_SUOSIKKI);
        else if( piilotetut_.contains(numero))
            tili->asetaTila(Tili::TILI_PIILOSSA);
        else {
            QVariantList muodot = tili->arvo("muodot").toList();
            if( laajuus >= tili->laajuus() && ( muodot.isEmpty() || muodot.contains(muoto) ))
                tili->asetaTila(Tili::TILI_KAYTOSSA);
            else
                tili->asetaTila(Tili::TILI_PIILOSSA);
        }


        // Asetetaan kaikki otsikkotasot käyttöön
        if( tili->tila() != Tili::TILI_PIILOSSA) {
            Tili *otsikko = tili->tamanOtsikko();
            while( otsikko ) {
                if( otsikko->tila() == Tili::TILI_KAYTOSSA)
                    break;
                otsikko->asetaTila(Tili::TILI_KAYTOSSA);

                otsikko = otsikko->tamanOtsikko();

            }
        }

    }
    emit dataChanged( index(0,0), index(rowCount()-1,0), QVector<int>() << TilaRooli );
}

