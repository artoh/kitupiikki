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

TilinavausModel::TilinavausModel() :    
    tosite_(new Tosite(this)),
    muokattu_(false)
{
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

    Tili tili = kp()->tilit()->tiliIndeksilla( index.row());
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
                if( tili.onko(TiliLaji::KAUDENTULOS))
                {
                    qlonglong tulos = 0;
                    QMapIterator<int,QList<AvausEra>> iter(erat_);
                    while( iter.hasNext() )
                    {
                        iter.next();
                        Tili tili = Kirjanpito::db()->tilit()->tiliNumerolla( iter.key());
                        if( tili.onko(TiliLaji::TULO) )
                            tulos += erasumma(iter.value());
                        else if( tili.onko(TiliLaji::MENO) )
                            tulos -= erasumma(iter.value());
                    }
                    return QVariant( QString("%L1 €").arg( ( tulos / 100.0 ), 10,'f',2));
                }

                QList<AvausEra> avaus = erat_.value(tili.numero());
                qlonglong saldo = 0;
                for( auto rivi : avaus)
                    saldo += rivi.saldo();

                if( role == Qt::EditRole)
                    return QVariant(saldo / 100.0);

                double saldod = saldo / 100.0;
                if( saldo )
                    return QVariant( QString("%L1 €").arg( saldod, 10,'f',2));
                else
                    return QVariant();
            }
            case ERITTELY:
            {
                QList<AvausEra> avaus = erat_.value(tili.numero());
                if( avaus.count() > 0 && (!avaus.first().eranimi().isEmpty() || avaus.first().kohdennus()) )
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
    else if( role == Qt::TextColorRole)
    {
        if( !tili.tila() )
            return QColor(Qt::darkGray);
        else if( tili.onko(TiliLaji::KAUDENTULOS))
            return QColor(Qt::gray);
        else
            return QColor(Qt::black);
    }
    else if( role == Qt::DecorationRole && index.column() == ERITTELY) {
        if( tili.eritellaankoTase())
            return QIcon(":/pic/format-list-unordered.png");
        if( kp()->kohdennukset()->kohdennuksia() && (
            tili.onko(TiliLaji::TULOS) || tili.luku("kohdennukset")  ))
                return QIcon(":/pic/kohdennus.png");
    } else if( role == ErittelyRooli ) {
        if( tili.eritellaankoTase())
            return TASEERAT;
        if( kp()->kohdennukset()->kohdennuksia() && (
            tili.onko(TiliLaji::TULOS) || tili.luku("kohdennukset")  ))
                return KOHDENNUKSET;
        return EI_ERITTELYA;
    }
    else if( role == KaytossaRooli)
    {
        if( erat_.contains(tili.numero()) )
            return "012";
        else if( tili.tila())
            return "01";
        return "0";
    }
    else if( role == Qt::BackgroundColorRole) {
        if( tili.otsikkotaso())
            return QPalette().mid().color();
    } else if( role == NimiRooli )
        return tili.nimi();
    else if( role == NumeroRooli)
        return tili.numero();


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

    if(kp()->tilit()->tiliIndeksilla( index.row() ).onko(TiliLaji::ALVSAATAVA ) && !alvSaatavaVaroitus)
    {
        QMessageBox::critical(nullptr, tr("ALV-saatavien tili"),
           tr("ALV-saatavien tili on tarkoitettu ainoastaan saataville, joista ei ole vielä "
              "annettu alv-ilmoitusta. Jo ilmoitetun saatavan tulisi olla Verosaatavat-tilillä niin, "
              "että tämä tili on tilikauden vaihtuessa ilman saldoa.\n"
              "Lue Kitupiikin ohjeet arvonlisäverosta."));
            alvSaatavaVaroitus = true;
            return false;
    }
    if(kp()->tilit()->tiliIndeksilla( index.row() ).onko(TiliLaji::ALVVELKA ) && !alvVelkaVaroitus)
    {
        QMessageBox::critical(nullptr, tr("ALV-velkojen tili"),
           tr("ALV-velkojen tili on tarkoitettu ainoastaan saataville, joista ei ole vielä "
              "annettu alv-ilmoitusta. Jo ilmoitetun velan tulisi olla Verovelat-tilillä niin, "
              "että tämä tili on tilikauden vaihtuessa ilman saldoa.\n"
              "Lue Kitupiikin ohjeet arvonlisäverosta."));
            alvVelkaVaroitus = true;
            return false;
    }


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
        dataChanged( index.sibling(kaudenTulosIndeksi_, SALDO),
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


bool TilinavausModel::tallenna()
{
    tosite_->viennit()->tyhjenna();

    QMapIterator<int, QList<AvausEra>> iter(erat_);

    while( iter.hasNext()) {

        iter.next();
        int tili = iter.key();

        for(auto era : iter.value()) {

            TositeVienti vienti;
            vienti.setPvm( tosite_->pvm() );
            vienti.setTili(tili);

            if( era.vienti() )
                vienti.set(TositeVienti::ID, era.vienti());

            if( !era.eranimi().isEmpty()) {
                if( era.vienti())
                    vienti.setEra( era.vienti() );
                else
                    vienti.setEra(-1);
                vienti.setSelite( era.eranimi() );
            }
            vienti.setKohdennus( era.kohdennus() );

            Tili tilio = kp()->tilit()->tiliNumerolla(tili);
            if( ( tilio.onko( (TiliLaji::VASTAAVAA)) || tilio.onko(TiliLaji::MENO))
                ^ ( era.saldo() < 0 ) )
                vienti.setDebet( qAbs(era.saldo()) );
            else
                vienti.setKredit( qAbs(era.saldo()));

            tosite_->viennit()->lisaa(vienti);
        }
    }
    tosite_->tallenna();

    kp()->asetukset()->aseta("Tilinavaus",1);   // Tilit merkitään avatuiksi

    muokattu_ = false;
    return true;
}

void TilinavausModel::lataa()
{
    tosite_->lataa(1);
}

void TilinavausModel::paivitaInfo()
{
    qlonglong tasevastaavaa = 0;
    qlonglong tasevastattavaa = 0;
    qlonglong tulos = 0;

    QMapIterator<int,QList<AvausEra>> iter(erat_);
    while( iter.hasNext() )
    {
        iter.next();
        Tili tili = Kirjanpito::db()->tilit()->tiliNumerolla( iter.key());
        if( tili.onko(TiliLaji::VASTAAVAA)  )
            tasevastaavaa += erasumma( iter.value() );
        else if( tili.onko(TiliLaji::VASTATTAVAA) )
            tasevastattavaa += erasumma(iter.value());
        else if( tili.onko(TiliLaji::TULO) )
            tulos += erasumma(iter.value());
        else if( tili.onko(TiliLaji::MENO) )
            tulos -= erasumma(iter.value());
    }

    tasevastattavaa += tulos;
    emit tilasto(tasevastaavaa, tasevastattavaa, tulos);
}

void TilinavausModel::ladattu()
{
    beginResetModel();
    TositeViennit* viennit = tosite_->viennit();
    for(int i=0; i < viennit->rowCount(); i++ ) {
        TositeVienti vienti = viennit->vienti(i);

        int tilinro = vienti.tili();
        QString era;

        if( vienti.eraId() )
            era = vienti.selite();
        int kohdennus = vienti.kohdennus();

        Tili tili = kp()->tilit()->tiliNumerolla(tilinro);
        qlonglong saldo = tili.onko(TiliLaji::VASTAAVAA) || tili.onko(TiliLaji::MENO) ?
                    qRound64(vienti.debet() * 100) - qRound64(vienti.kredit() * 100) :
                    qRound64(vienti.kredit() * 100) - qRound64(vienti.debet() * 100);

        QList<AvausEra>& erat = erat_[tilinro];
        erat.append( AvausEra(saldo, era, kohdennus, vienti.id()) );

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

qlonglong TilinavausModel::erasumma(const QList<AvausEra> &erat)
{
    qlonglong summa = 0l;
    for( auto era : erat)
        summa += era.saldo();
    return summa;
}


AvausEra::AvausEra(qlonglong saldo, const QString &eranimi, int kohdennus, int vienti) :
    eranimi_(eranimi), kohdennus_(kohdennus), saldo_(saldo), vienti_(vienti)
{

}
