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
#include "tositeviennit.h"

#include <QDate>
#include "db/tili.h"
#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"
#include "tosite.h"
#include "alv/alvilmoitustenmodel.h"

#include <QDebug>

TositeViennit::TositeViennit(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant TositeViennit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return tr("Pvm");
            case TILI:
                return tr("Tili");
            case DEBET :
                return tr("Debet");
            case KREDIT:
                return tr("Kredit");
            case KOHDENNUS :
                return tr("Kohdennus");
            case ALV:
                return tr("Alv");
            case SELITE:
                return tr("Selite");
        }
    }
    return QVariant( section + 1);
}


int TositeViennit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return viennit_.count();
}

int TositeViennit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant TositeViennit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TositeVienti rivi = vienti(index.row());

    switch (role) {
    case Qt::DisplayRole :
        switch ( index.column()) {
        case PVM:
            return rivi.value("pvm").toDate();
        case TILI:
        {
            Tili *tili = kp()->tilit()->tili( rivi.value("tili").toInt() );
            if( tili )
                return QString("%1 %2").arg(tili->numero()).arg(tili->nimi());
            return QVariant();
        }
        case DEBET:
        {
            double debet = rivi.value("debet").toDouble();
             if( debet > 1e-5 )
                return QVariant( QString("%L1 €").arg(debet,0,'f',2));
             return QVariant();
        }
        case KREDIT:
        {
            double kredit = rivi.value("kredit").toDouble();
            if( kredit > 1e-5)
                return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
             return QVariant();
        }
        case ALV:
        {
            int alvkoodi = rivi.value("alvkoodi").toInt();
            if( alvkoodi == AlvKoodi::EIALV )
                return QVariant();
            else
            {
                if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                    return tr("VERO");
                else if(alvkoodi == AlvKoodi::TILITYS)
                    return QString();
                else
                    return QVariant( QString("%1 %").arg( rivi.value("alvprosentti").toInt() ));
            }
        }
        case SELITE:
            return rivi.value("selite");
        case KOHDENNUS:
            QString txt;
            int kohdennus = rivi.value("kohdennus").toInt();
            if( kohdennus )
                txt.append( kp()->kohdennukset()->kohdennus(kohdennus).nimi() + " " );
            QVariantList merkkaukset = rivi.value("merkkaukset").toList();
            for( auto merkkaus : merkkaukset)
                txt.append( kp()->kohdennukset()->kohdennus( merkkaus.toInt() ).nimi() + " " );
            if( rivi.eraId() == -1 )
                txt.append(tr("Uusi erä"));
            else if( rivi.value("era").toMap().contains("tunniste")  )
            {
                if( rivi.value("era").toMap().value("tunniste") != rivi.value("tosite").toMap().value("tunniste") ||
                    rivi.value("era").toMap().value("pvm") != rivi.value("tosite").toMap().value("pvm")) {
                    txt.append( kp()->tositeTunnus( rivi.value("era").toMap().value("tunniste").toInt(),
                                                    rivi.value("era").toMap().value("pvm").toDate(),
                                                    rivi.value("era").toMap().value("sarja").toString()) );
                }
            }            
            return txt;

        }
        break;
    case Qt::EditRole :
        switch ( index.column())
        {
        case PVM:
            return rivi.value("pvm").toDate();
        case TILI:
            return rivi.value("tili").toInt();
        case DEBET:
            return rivi.value("debet").toDouble();
        case KREDIT:
            return rivi.value("kredit").toDouble();
        case KOHDENNUS:
            return rivi.kohdennus();
        case SELITE:
            return rivi.selite();
        }
        break;
    case Qt::TextAlignmentRole:
        if( index.column()==KREDIT || index.column() == DEBET || index.column() == ALV)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::DecorationRole:
        if( index.column() == ALV )
        {
            return kp()->alvTyypit()->kuvakeKoodilla( rivi.data(TositeVienti::ALVKOODI).toInt() );
        } else if( index.column() == KOHDENNUS ) {
            if( rivi.contains("era") && rivi.value("era").toMap().value("saldo") == 0 )
                return QIcon(":/pic/ok.png");
            Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( rivi.value("kohdennus").toInt() );
            if(kohdennus.tyyppi())
                return kp()->kohdennukset()->kohdennus( rivi.value("kohdennus").toInt()).tyyppiKuvake();
            else
                return QIcon(":/pic/tyhja.png");
        } else if( index.column() == PVM)
        {
            // Väärät päivät
            QDate pvm = rivi.pvm();
            if( rivi.id() && pvm <= kp()->tilitpaatetty())
                return QIcon(":/pic/lukittu.png");
            else if( !pvm.isValid() || pvm <= kp()->tilitpaatetty() || pvm > kp()->tilikaudet()->kirjanpitoLoppuu() )
                return QIcon(":/pic/varoitus.png");
            else if( kp()->alvIlmoitukset()->onkoIlmoitettu(pvm) && rivi.alvkoodi() )
                return QIcon(":/pic/vero.png");
        }
        break;
    case IdRooli:
        return rivi.id();
    case PvmRooli:
        return rivi.pvm();
    case TiliNumeroRooli:
        return rivi.tili();
    case DebetRooli:
        return rivi.debet();
    case KreditRooli:
        return rivi.kredit();
    case AlvKoodiRooli:
        return rivi.alvKoodi();
    case AlvProsenttiRooli:
        return rivi.alvProsentti();
    case KohdennusRooli:
        return rivi.kohdennus();
    case SeliteRooli:
        return rivi.selite();
    case EraIdRooli:
        return rivi.eraId();
    case TaseErittelyssaRooli:
    {
        Tili tili = kp()->tilit()->tiliNumerolla( rivi.tili() );
        return tili.eritellaankoTase();
    }
    case TagiIdListaRooli:
        return rivi.data(TositeVienti::MERKKAUKSET);


    }

    return QVariant();
}

bool TositeViennit::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        if( role == Qt::EditRole) {

            TositeVienti rivi = vienti(index.row());

            switch (index.column()) {

            case PVM:
                rivi.setPvm( value.toDate() );
                break;
            case TILI:
                {
                    Tili uusitili;
                    if( value.toInt())
                        uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
                    else if(!value.toString().isEmpty() && value.toString() != " ")
                        uusitili = TilinValintaDialogi::valitseTili(value.toString());
                    else
                        uusitili = TilinValintaDialogi::valitseTili( QString());

                    rivi.setTili( uusitili.numero());
                    if( uusitili.eritellaankoTase()) {
                        rivi.setEra(-1);
                        emit dataChanged( index.sibling(index.row(), KOHDENNUS), index.sibling(index.row(), KOHDENNUS) );
                    } else
                        rivi.setEra( 0);
                    if( kp()->asetukset()->onko(AsetusModel::ALV)) {
                        int alvkoodi = uusitili.arvo("alvlaji").toInt();
                        if( alvkoodi == AlvKoodi::MYYNNIT_NETTO)
                            alvkoodi = AlvKoodi::MYYNNIT_BRUTTO;
                        else if( alvkoodi == AlvKoodi::OSTOT_NETTO)
                            alvkoodi = AlvKoodi::OSTOT_BRUTTO;
                        rivi.setAlvKoodi( alvkoodi );
                        rivi.setAlvProsentti( uusitili.arvo("alvprosentti").toDouble() );
                        emit dataChanged( index.sibling(index.row(), ALV), index.sibling(index.row(), ALV) );
                    }


                    break;
                }
            case SELITE:
                rivi.setSelite( value.toString());
                break;
            case DEBET:
                rivi.setDebet( value.toDouble() );
                break;
            case KREDIT:
                rivi.setKredit( value.toDouble() );
                break;
            case KOHDENNUS:
                rivi.setKohdennus( value.toInt() );
                break;
            default:
                return false;

            }

            viennit_[index.row()] = rivi;
        } else if( role == TositeViennit::EraMapRooli) {
            TositeVienti rivi = vienti(index.row());
            rivi.setEra( value.toMap() );
            viennit_[index.row()] = rivi;

            emit dataChanged(index, index, QVector<int>() << Qt::EditRole);
        } else if( role == TositeViennit::AlvKoodiRooli) {
            TositeVienti rivi = vienti(index.row());
            rivi.setAlvKoodi( value.toInt());
            viennit_[index.row()] = rivi;
            emit dataChanged(index, index, QVector<int>() << Qt::EditRole);
        } else if( role == TositeViennit::AlvProsenttiRooli) {
            TositeVienti rivi = vienti(index.row());
            rivi.setAlvProsentti( value.toDouble() );
            viennit_[index.row()] = rivi;
            emit dataChanged(index, index, QVector<int>() << Qt::EditRole);
        } else if( role == TositeViennit::TagiIdListaRooli) {
            TositeVienti rivi = vienti(index.row());
            rivi.setMerkkaukset(value.toList());
            viennit_[index.row()] = rivi;
            emit dataChanged(index, index, QVector<int>() << Qt::EditRole);
        }
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags TositeViennit::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if( index.column() == ALV)
        return Qt::ItemIsEnabled;
    else if( index.column() == KOHDENNUS )
    {
        TositeVienti rivi = vienti(index.row());
        Tili tili = kp()->tilit()->tiliNumerolla(rivi.tili());
        if( !tili.onko(TiliLaji::TULOS) && !tili.onko(TiliLaji::POISTETTAVA) &&  !tili.eritellaankoTase())
            return Qt::ItemIsEnabled;
    }


    if( muokattavissa_ )
        return Qt::ItemIsEditable | Qt::ItemIsEnabled; // FIXME: Implement me!

    return Qt::ItemIsEnabled;
}

bool TositeViennit::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    for(int i=0; i < count; i++)
        lisaaVienti(row);
    return true;
}


bool TositeViennit::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit_.removeAt(row);
    endRemoveRows();
    return true;
}

QModelIndex TositeViennit::lisaaVienti(int indeksi)
{
    TositeVienti uusi;

    Tosite* tosite = qobject_cast<Tosite*>(parent());
    uusi.setPvm( tosite->pvm() );

    if( indeksi > 0){
        TositeVienti edellinen = viennit_.at(indeksi-1).toMap();
        Tili tili = kp()->tilit()->tiliNumerolla(edellinen.tili());
        if( tili.luku("vastatili"))
            uusi.setTili(tili.luku("vastatili"));
        uusi.setSelite(edellinen.selite());
    } else {
        uusi.setSelite( tosite->otsikko());
    }

    // Pyrkii tasaamaan tilit ;)

    qlonglong dsumma = 0;
    qlonglong ksumma = 0;
    for(int i=0; i < rowCount(); i++) {
        TositeVienti tvienti( viennit_.at(i).toMap());
        dsumma += qRound64( tvienti.debet() * 100 );
        ksumma += qRound64 ( tvienti.kredit() * 100);
    }
    if( dsumma > ksumma )
        uusi.setKredit( dsumma - ksumma);
    else
        uusi.setDebet( ksumma - dsumma );


    beginInsertRows( QModelIndex(), indeksi, indeksi);
    viennit_.insert(indeksi, uusi);
    endInsertRows();
    return index(indeksi, 0);
}

TositeVienti TositeViennit::vienti(int indeksi) const
{
    return TositeVienti( viennit_.at(indeksi).toMap() );
}

void TositeViennit::lisaa(const TositeVienti &vienti)
{
    beginInsertRows(QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append(vienti);
    endInsertRows();
}


void TositeViennit::asetaViennit(QVariantList viennit)
{
    beginResetModel();
    viennit_ = viennit;
    // Erätietojen siivoaminen ja sijoittaminen välimuistiin
    endResetModel();

}

void TositeViennit::tyhjenna()
{
    asetaViennit(QVariantList());
}

QVariant TositeViennit::tallennettavat() const
{
    QVariantList ulos;
    for( auto vienti : viennit_) {
        TositeVienti tv(vienti.toMap());
        ulos.append( tv.tallennettava());
    }
    return ulos;
}

void TositeViennit::asetaMuokattavissa(bool muokattavissa)
{
    muokattavissa_ = muokattavissa;
}

QString TositeViennit::alvTarkastus() const
{
   qlonglong alvPerusteella = 0;
   qlonglong alvKirjattuna = 0;
   qlonglong vahennysPerusteella = 0;
   qlonglong vahennysKirjattuna= 0;

   for( auto vienti : viennit_) {
       TositeVienti tv(vienti.toMap());
       switch (tv.alvKoodi()) {
       case AlvKoodi::MYYNNIT_NETTO:
           alvPerusteella += qRound64((tv.kredit() - tv.debet()) * tv.alvProsentti());
           break;
       case AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS:
            alvKirjattuna += qRound64((tv.kredit() - tv.debet()) * 100);
            break;
       case AlvKoodi::OSTOT_NETTO:
           vahennysPerusteella += qRound64((tv.debet() - tv.kredit()) * tv.alvProsentti());
           break;
        case AlvKoodi::OSTOT_NETTO + AlvKoodi::ALVVAHENNYS:
            vahennysKirjattuna += qRound64((tv.debet() - tv.kredit()) * 100);
       }

   }
   QString palaute;
   if( alvPerusteella != alvKirjattuna)
       palaute.append(tr("\nMyynneistä pitäisi tilittää arvonlisäveroa %1 €").arg(alvPerusteella / 100.0, 0, 'f', 2));
   if( vahennysKirjattuna != vahennysPerusteella)
       palaute.append(tr("\nOstoista pitäisi vähentää arvonlisäveroa %1 €").arg(vahennysPerusteella / 100.0, 0, 'f', 2));

   return palaute;
}
