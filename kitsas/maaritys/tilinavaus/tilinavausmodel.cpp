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

#include "tilinavausmodel.h"
#include "model/tositeviennit.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>
#include <QDebug>
#include <QPalette>

#include <QMessageBox>

#include "db/tositetyyppimodel.h"
#include "model/tositevienti.h"
#include "tuonti/tilimuuntomodel.h"

TilinavausModel::TilinavausModel(QObject* parent) :
    QAbstractTableModel{parent},
    tosite_(new Tosite(this)),
    muokattu_(false)
{
    setKuukausittain( kp()->asetukset()->onko(AsetusModel::AvausKuukausittain) );
    connect( tosite_, &Tosite::ladattu, this, &TilinavausModel::ladattu);
}

int TilinavausModel::rowCount(const QModelIndex & /* parent */ ) const
{
    return kp()->tilit()->rowCount( QModelIndex());
}

int TilinavausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 4;
}

QVariant TilinavausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case NRO :
            return QVariant("Numero");
        case NIMI:
            return QVariant("Tili");
        case SALDO :
            return QVariant("Alkusaldo");
        case ERITTELY:
            return QVariant("Erittely");
        }
    }
    return QVariant();
}

QVariant TilinavausModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    if( index.row() > kp()->tilit()->rowCount())
        return QVariant();

    Tili tili = kp()->tilit()->tiliIndeksilla( index.row());

    if( tili.numero() > 99999) {
        return QVariant();
    }

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case NRO:
                if( tili.otsikkotaso())
                    return QString();
                return QVariant( tili.numero() );

            case NIMI:
            {
                QString txt;
                for(int i=0; i < tili.otsikkotaso(); i++)
                    txt.append("  ");

                return txt + tili.nimi();
            }

            case SALDO:
            {
                if( tili.otsikkotaso())
                    return QVariant();
                if( tili.onko(TiliLaji::KAUDENTULOS))
                {
                    Euro tulos;
                    QMapIterator<int,QList<AvausEra>> iter(erat_);
                    while( iter.hasNext() )
                    {
                        iter.next();
                        Tili tili = Kirjanpito::db()->tilit()->tiliNumerolla( iter.key());
                        if( tili.onko(TiliLaji::TULOS) )
                            tulos += erasumma(iter.value());
                    }
                    return tulos.display(false);
                }

                QList<AvausEra> avaus = erat_.value(tili.numero());
                Euro saldo = Euro::Zero;
                for( auto &rivi : avaus)
                    saldo += rivi.saldo();

                if( role == Qt::EditRole)
                    return QVariant(saldo.toString());

                return saldo.display(false);
            }
            case ERITTELY:
            {
                QList<AvausEra> avaus = erat_.value(tili.numero());
                if( avaus.count() > 0 && (!avaus.first().eranimi().isEmpty() || avaus.last().kohdennus()) )
                    return avaus.count();
            }
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==SALDO)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::FontRole)
    {
        QFont fontti;
        if( tili.otsikkotaso() )
            fontti.setBold(true);
        return QVariant( fontti );
    }
    else if( role == Qt::ForegroundRole)
    {
        if( !tili.tila() )
            return QColor(Qt::darkGray);
        else if( tili.onko(TiliLaji::KAUDENTULOS))
            return QColor(Qt::gray);
        else
            return QColor(Qt::black);
    }
    else if( role == Qt::DecorationRole && index.column() == ERITTELY) {
        if( tili.onko(TiliLaji::OTSIKKO) ||  tili.onko(TiliLaji::KAUDENTULOS) || tili.onko(TiliLaji::EDELLISTENTULOS))            
            return QVariant();
        else if( kuukausittain() )
            return QIcon(":/pic/calendar.png");
        else if( tili.eritellaankoTase())
            return QIcon(":/pic/format-list-unordered.png");
        else if( kp()->kohdennukset()->kohdennuksia() && (
            tili.onko(TiliLaji::TULOS) || tili.luku("kohdennukset")  ))
                return QIcon(":/pic/kohdennus.png");
    } else if( role == ErittelyRooli ) {
        if( tili.onko(TiliLaji::OTSIKKO) || tili.onko(TiliLaji::KAUDENTULOS) || tili.onko(TiliLaji::EDELLISTENTULOS))
            return EI_ERITTELYA;
        else if( kuukausittain() )
            return KUUKAUDET;
        else if( tili.eritellaankoTase())
            return TASEERAT;
        else if( kp()->kohdennukset()->kohdennuksia() && (
            tili.onko(TiliLaji::TULOS) || tili.luku("kohdennukset")  ))
                return KOHDENNUKSET;
        return EI_ERITTELYA;
    }
    else if( role == KaytossaRooli)
    {
        if( tili.otsikkotaso())
            return "0";
        if( erat_.contains(tili.numero()) )
            return "012";
        else if( tili.tila())
            return "01";
        return "0";
    }
    else if( role == Qt::BackgroundRole) {
        if( tili.otsikkotaso())
            return QPalette().mid().color();
    } else if( role == NimiRooli )
        return tili.nimi();
    else if( role == NumeroRooli)
        return tili.numero();
    else if( role == LajitteluRooli) {
        if( tili.otsikkotaso())
            return QString("%1/%2").arg(tili.numero()).arg(tili.otsikkotaso(),2,10,QChar('0'));
        else
            return QString("%1/XX").arg(tili.numero());
    }


    return QVariant();
}

Qt::ItemFlags TilinavausModel::flags(const QModelIndex &index) const
{
    Tili tili = kp()->tilit()->tiliIndeksilla( index.row());
    if( index.column() == SALDO && !tili.otsikkotaso() && !tili.onko(TiliLaji::KAUDENTULOS))
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index);
}

bool TilinavausModel::setData(const QModelIndex &index, const QVariant &value, int /* role */)
{
    int tilinro = kp()->tilit()->tiliIndeksilla( index.row()).numero() ;

    if( qAbs(value.toDouble()) > 1e-5) {
        AvausEra rivi( qRound64( value.toDouble() * 100) );
        QList<AvausEra> lista;
        lista.append(rivi);
        erat_.insert(tilinro, lista);
    } else
        erat_.remove(tilinro);          // Ei jätetä nollia kirjauksiin

    muokattu_ = true;
    paivitaInfo();

    if( kp()->tilit()->tiliIndeksilla( index.row() ).onko(TiliLaji::TULOS))
    {
        // Päivitetään kauden tulosta
        emit dataChanged( index.sibling(kaudenTulosIndeksi_, SALDO),
                     index.sibling(kaudenTulosIndeksi_, SALDO));
    }

    return true;
}

void TilinavausModel::asetaErat(int tili, QList<AvausEra> erat)
{
    erat_.insert(tili, erat);
    for(int i=0; i < rowCount(); i++) {
        if( kp()->tilit()->tiliIndeksilla(i).numero() == tili ) {
            emit dataChanged( index(i, SALDO), index(i, ERITTELY) );
            break;
        }
    }
    muokattu_ = true;
    paivitaInfo();
}

QList<AvausEra> TilinavausModel::erat(int tili) const
{
    return erat_.value(tili);
}

void TilinavausModel::setKuukausittain(bool onko)
{
    if(onko != avausKuukausittain_) {
        beginResetModel();
        avausKuukausittain_ = onko;
        endResetModel();
        kp()->asetukset()->aseta(AsetusModel::AvausKuukausittain, onko);
        emit kuukausittainenVaihtui(onko);
    }
}


bool TilinavausModel::tallenna(int tila)
{
    tosite_->viennit()->tyhjenna();

    tosite_->asetaTyyppi(TositeTyyppi::TILINAVAUS);
    tosite_->asetaPvm(kp()->asetukset()->pvm("TilinavausPvm"));
    tosite_->asetaOtsikko( tulkkaa("Tilinavaus") );
    tosite_->asetaSarja( kp()->tositeTyypit()->sarja( TositeTyyppi::TILINAVAUS ) ) ;

    Q_ASSERT(tosite_->pvm().isValid());

    QMapIterator<int, QList<AvausEra>> iter(erat_);

    while( iter.hasNext()) {

        iter.next();
        int tili = iter.key();

        for(auto &era : iter.value()) {

            TositeVienti vienti;
            vienti.setPvm( era.pvm().isValid() ? era.pvm() : tosite_->pvm() );
            vienti.setTili(tili);
            vienti.setSelite( tr("Tilinavaus") );

            if( era.vienti() )
                vienti.set(TositeVienti::ID, era.vienti());

            Tili tilio = kp()->tilit()->tiliNumerolla(tili);

            if( era.saldo() ) {
                if( !tilio.eritellaankoTase()) {
                    // Jos tilillä ei tasetta eritellä, niin ei tule erää
                    vienti.setEra(0);
                }
                else if( era.vienti()) {
                    vienti.setEra( era.vienti() );
                } else if( !era.eranimi().isEmpty() || era.kumppaniId() ){
                    vienti.setEra(-1);
                }

                // Ostosaamiset ja velat -kirjataan niin,
                // että ne voidaan kirjata maksetuiksi Maksettu lasku -valinnalla
                if(  tilio.onko(TiliLaji::MYYNTISAATAVA) )
                    vienti.setTyyppi( TositeTyyppi::TULO + TositeVienti::VASTAKIRJAUS );
                else if( tilio.onko(TiliLaji::OSTOVELKA) )
                    vienti.setTyyppi( TositeTyyppi::MENO + TositeVienti::VASTAKIRJAUS );

                vienti.setSelite( era.eranimi() );
                vienti.setTasaerapoisto( era.tasapoisto());
            }
            vienti.setKohdennus( era.kohdennus() );
            if( era.kumppaniId())
                vienti.setKumppani(era.kumppaniId());
            else if( !era.kumppaniNimi().isEmpty() )
                vienti.setKumppani( era.kumppaniNimi());

            if( tilio.onko( (TiliLaji::VASTAAVAA))
                ^ ( era.saldo() < Euro::Zero ) )
                vienti.setDebet( era.saldo().abs() );
            else
                vienti.setKredit( era.saldo().abs());

            tosite_->viennit()->lisaa(vienti);
        }
    }
    // Tallennuksen jälkeen ladataan välittömästi, jotta kumppanirekisteri ajan tasalla
    connect( tosite_, &Tosite::talletettu, this, &TilinavausModel::lataa);    

    tosite_->tallenna(tila);

    kp()->asetukset()->aseta("Tilinavaus",1);   // Tilit merkitään avatuiksi

    return true;
}

void TilinavausModel::lataa()
{
    KpKysely* kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("tyyppi", TositeTyyppi::TILINAVAUS);
    if(kysely) {
        connect(kysely, &KpKysely::vastaus, this, &TilinavausModel::idTietoSaapuu);
        kysely->kysy();
    }
}

void TilinavausModel::paivitaInfo()
{
    Euro tasevastaavaa;
    Euro tasevastattavaa = 0;
    Euro tulos = 0;

    QMapIterator<int,QList<AvausEra>> iter(erat_);
    while( iter.hasNext() )
    {
        iter.next();
        Tili tili = Kirjanpito::db()->tilit()->tiliNumerolla( iter.key());
        if( tili.onko(TiliLaji::VASTAAVAA)  )
            tasevastaavaa += erasumma( iter.value() );
        else if( tili.onko(TiliLaji::VASTATTAVAA) )
            tasevastattavaa += erasumma(iter.value());
        else if( tili.onko(TiliLaji::TULOS) )
            tulos += erasumma(iter.value());        
    }

    tasevastattavaa += tulos;
    emit tilasto(tasevastaavaa, tasevastattavaa, tulos);
}

void TilinavausModel::ladattu()
{
    beginResetModel();
    erat_.clear();
    muokattu_ = false;
    TositeViennit* viennit = tosite_->viennit();
    for(int i=0; i < viennit->rowCount(); i++ ) {
        TositeVienti vienti = viennit->vienti(i);

        int tilinro = vienti.tili();
        QString era;

        if( vienti.eraId() )
            era = vienti.selite();
        int kohdennus = vienti.kohdennus();
        int kumppani = vienti.kumppaniId();
        QString kumppaninimi = vienti.value("kumppani").toMap().value("nimi").toString();
        int poistoaika = vienti.tasaerapoisto();
        QDate pvm = vienti.pvm();

        Tili tili = kp()->tilit()->tiliNumerolla(tilinro);
        qlonglong saldo = tili.onko(TiliLaji::VASTAAVAA)  ?
                    qRound64(vienti.debet() * 100) - qRound64(vienti.kredit() * 100) :
                    qRound64(vienti.kredit() * 100) - qRound64(vienti.debet() * 100);

        QList<AvausEra>& erat = erat_[tilinro];
        erat.append( AvausEra(saldo, pvm, era, kohdennus, vienti.id(), kumppani, kumppaninimi, poistoaika));

    }
    endResetModel();

    // Haetaan kauden tuloksen indeksi, jotta voidaan päivittää tulosta
    for(int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
        if( kp()->tilit()->tiliIndeksilla(i).onko(TiliLaji::KAUDENTULOS))
        {
            kaudenTulosIndeksi_ = i;
            break;
        }
    paivitaInfo();
}

void TilinavausModel::tuo(TiliMuuntoModel *model)
{
    if( model->saldopaivat().count() > 1)
        setKuukausittain(true);

    for(int i=0; i < model->rowCount(); i++) {
        const int tiliNro = model->tilinumeroIndeksilla(i);
        Tili* tili = kp()->tilit()->tili(tiliNro);
        if(tili && !tili->onko(TiliLaji::KAUDENTULOS)) {

            QList<AvausEra> uudet = model->eraIndeksilla(i);
            QList<AvausEra> vanhat = erat_.value(tiliNro);

            if( uudet.length() == vanhat.length()) {
                for(int i=0; i < uudet.length(); i++) {
                    vanhat[i].asetaSaldo( vanhat.at(i).saldo() + uudet.at(i).saldo() );
                }
                erat_.insert(tiliNro, vanhat);
            } else {
                erat_.insert(tiliNro, uudet);
            }
        }
    }
    emit dataChanged( createIndex(0, SALDO), createIndex(rowCount(), ERITTELY) );
    muokattu_ = true;
    paivitaInfo();
}

void TilinavausModel::idTietoSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    if( lista.isEmpty() ) {
        KpKysely* kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("tyyppi", TositeTyyppi::TILINAVAUS);
        kysely->lisaaAttribuutti("luonnos");
        if(kysely) {
            connect(kysely, &KpKysely::vastaus, this, &TilinavausModel::luonnosIdSaapuu);
            kysely->kysy();
        }
    } else {
        QVariantMap map = lista.first().toMap();
        tosite_->lataa(map.value("id").toInt());
    }
}

void TilinavausModel::luonnosIdSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    if( !lista.isEmpty()) {
        QVariantMap map = lista.first().toMap();
        tosite_->lataa(map.value("id").toInt());
    }
}

Euro TilinavausModel::erasumma(const QList<AvausEra> &erat)
{
    Euro summa = Euro::Zero;
    for( auto &era : erat)
        summa += era.saldo();
    return summa;
}


AvausEra::AvausEra(Euro saldo, const QDate &pvm, const QString &eranimi, int kohdennus, int vienti, int kumppaniId, QString kumppaniNimi, int tasapoisto) :
    eranimi_(eranimi), kohdennus_(kohdennus), saldo_(saldo), vienti_(vienti), kumppaniId_(kumppaniId), kumppaniNimi_(kumppaniNimi), tasapoisto_(tasapoisto),
    pvm_{pvm}
{

}

void AvausEra::asetaKumppani(const QVariantMap &map)
{
    kumppaniId_ = map.value("id").toInt();
    kumppaniNimi_ = map.value("nimi").toString();
}
