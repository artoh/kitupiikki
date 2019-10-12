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

    emit dataChanged( index.sibling(index.row(), 0), index.sibling(index.row(), 4));

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
        qDebug() << " --- id --- "  << tili->nimiNumero();
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
    else if( role == YsiRooli)
        return QVariant( tili->ysivertailuluku());
    else if( role == TilaRooli)
        return tili->tila();
    else if( role == OhjeRooli) {
        return tili->ohje();
    } else if( role == MaksutapaRooli )
        return tili->str("maksutapa");
    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case NRONIMI :
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

void TiliModel::lisaaTili(const Tili& uusi)
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

Tili TiliModel::tiliIdllaVanha(int id) const
{
    qDebug() << " -------idhaku----- " << id;
    throw tr("Yritetään hakea tiliä %1").arg(id);

}

Tili *TiliModel::tili(const QString &tilinumero) const
{
    return tiliPNumerolla( tilinumero.toInt());
}

Tili *TiliModel::tiliPNumerolla(int numero) const
{
    return nroHash_.value(numero);
}

Tili TiliModel::tiliNumerolla(int numero, int otsikkotaso) const
{
    // Vertailu tehdään "ysiluvuilla" joten tilit 154 ja 15400 ovat samoja
    return tiliYsiluvulla( Tili::ysiluku(numero, otsikkotaso) );
}

Tili TiliModel::tiliYsiluvulla(int ysiluku) const
{
    foreach (Tili tili, tilit_)
    {
        if( tili.ysivertailuluku() == ysiluku )
            return tili;
    }
    return Tili();
}

Tili TiliModel::tiliIbanilla(const QString &iban) const
{
    for(Tili tili: tilit_)
    {
        if( tili.str("iban") == iban)
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


Tili TiliModel::tiliTyypilla(TiliLaji::TiliLuonne tyyppi) const
{
    foreach (Tili tili, tilit_) {
        if( tili.tyyppi().luonne() == tyyppi)
            return tili;
    }
    return Tili();
}

QStringList TiliModel::laskuTilit() const
{
    QStringList tilit;
    for(Tili tili: tilit_) {
        if( tili.luku("laskulle") == 1)
            tilit.insert(0, tili.str("iban"));
        else if( tili.luku("laskulle") == 2)
            tilit.append( tili.str("iban"));
    }
    return tilit;
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
    /*

    beginResetModel();
    tilit_.clear();

    QSqlQuery kysely( *tietokanta_ );
    kysely.exec("SELECT id, nro, nimi, tyyppi, tila, json, ysiluku, muokattu "
                " FROM tili ORDER BY ysiluku");


    QVector<Tili> otsikot(10);

    while(kysely.next())
    {
        int otsikkotaso = 0;
        QString tyyppikoodi = kysely.value(3).toString();
        if( tyyppikoodi.startsWith('H'))    // Tyyppikoodi H1 tarkoittaa 1-tason otsikkoa jne.
            otsikkotaso = tyyppikoodi.midRef(1).toInt();

        int id = kysely.value(0).toInt();
        int otsikkoIdTalle = 0; // Nykytilille merkittävä otsikkotaso
        int ysiluku = kysely.value(6).toInt();
        int nro = kysely.value(1).toInt();


        // Etsitään otsikkotasoa tasojen lopusta alkaen
        for(int i=9; i >= 0; i--)
        {
            int asti = otsikot[i].json()->luku("Asti") ? Tili::ysiluku( otsikot[i].json()->luku("Asti"),true) : Tili::ysiluku( otsikot[i].numero(), true);
            if( otsikot.at(i).onkoValidi() && otsikot.at(i).ysivertailuluku() <= ysiluku && asti >= ysiluku )
            {
                otsikkoIdTalle = otsikot.at(i).id();
                break;
            }
        }

        Tili uusi( id,     // id
                   nro,     // nro
                   kysely.value(2).toString(),  // nimi
                   tyyppikoodi,  // tyyppi
                   kysely.value(4).toInt(),     // tila
                   otsikkoIdTalle,    // Tätä tiliä/otsikkoa ylemmän otsikon id
                   kysely.value(7).toDateTime()     // Muokattu viimeksi
                   );
        uusi.json()->fromJson( kysely.value(5).toByteArray());  // Luetaan json-kentät
        uusi.nollaaMuokattu();
        tilit_.append(uusi);

        if( otsikkotaso )
            otsikot[otsikkotaso] = uusi;

    }

    endResetModel();
    */
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

        Tili uusi( map );
        tilit_.append(uusi);

        tiliLista_.append( tili );

        if( !tili->str("palkkatili").isEmpty() )
            palkkatilit_.insert( tili->str("palkkatili"), tili->numero() );

    }
    laajuus_ = kp()->asetukset()->luku("laajuus",3);

    paivitaTilat();

    qDebug() << " Listalla " << tiliLista_.count() << " tiliä ";
    for(Tili* pointteri : tiliLista_)
        qDebug() <<  pointteri->tila() << "   " << pointteri->tyyppiKoodi() << "   " << pointteri->nimiNumero();

    endResetModel();
}

bool TiliModel::tallenna(bool tietokantaaLuodaan)
{
    return false;   // Poissa käytöstä

    /*

    tietokanta_->transaction();
    QDateTime nykyaika = QDateTime::currentDateTime();

    for( int i=0; i < tilit_.count() ; i++)
    {
        Tili tili = tilit_[i];
        if( tili.onkoValidi() && tili.muokattu() )
        {
            QSqlQuery kysely(*tietokanta_);
            if( tili.id())
            {
                // Muokkaus
                kysely.prepare("UPDATE tili SET nro=:nro, nimi=:nimi, tyyppi=:tyyppi, "
                               "tila=:tila, ysiluku=:ysiluku, json=:json, muokattu=:aika "
                               "WHERE id=:id");
                kysely.bindValue(":id", tili.id());
            }
            else
            {
                // Tallennus
                kysely.prepare("INSERT INTO tili(nro, nimi, tyyppi, tila, ysiluku, json, muokattu) "
                               "VALUES(:nro, :nimi, :tyyppi, :tila, :ysiluku, :json, :aika) ");

            }
            kysely.bindValue(":nro", tili.numero());
            kysely.bindValue(":nimi", tili.nimi());
            kysely.bindValue(":tyyppi", tili.tyyppiKoodi());
            kysely.bindValue(":tila", tili.tila());
            kysely.bindValue(":ysiluku", tili.ysivertailuluku());
            kysely.bindValue(":json", tilit_[i].json()->toSqlJson());

            if( !tietokantaaLuodaan && tili.muokattuMuutakinKuinTilaa() )
                // Pelkkä tilan vaihtaminen ei merkitse tietokantaan muokatuksi.
                kysely.bindValue(":aika", nykyaika );
            else
                kysely.bindValue(":aika", tili.muokkausaika() );
            if( kysely.exec() )
                tilit_[i].nollaaMuokattu();
            else
                kp()->lokiin(kysely);

            if( !tili.id())
                tilit_[i].asetaId( kysely.lastInsertId().toInt() );

        }
    }

    foreach (int id, poistetutIdt_)
    {
        QSqlQuery kysely(*tietokanta_);
        kysely.exec( QString("DELETE FROM tili WHERE id=%1").arg(id) );
    }

    tietokanta_->commit();

    if( tietokanta_->lastError().isValid() )
    {
        QMessageBox::critical(nullptr, tr("Tietokantavirhe"),
                          tr("Tallentaminen epäonnistui seuraavan virheen takia: %1")
                          .arg( tietokanta_->lastError().text() ));
        return false;
    }

    return true;
    */
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

    tilit_.clear();
    tiliLista_.clear();
    nroHash_.clear();
}

void TiliModel::paivitaTilat()
{
    qDebug() << " Laajuus " << laajuus_;

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
        else if( tili->laajuus() > laajuus_)
            tili->asetaTila(Tili::TILI_PIILOSSA);
        else
            tili->asetaTila(Tili::TILI_KAYTOSSA);

        qDebug() << tili->nimiNumero() << " tilassa " << tili->tila()
                 << " l " << tili->laajuus();

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
}

