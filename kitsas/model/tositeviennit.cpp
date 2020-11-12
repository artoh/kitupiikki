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
            case KUMPPANI:
                return tr("Asiakas/Toimittaja");
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

    return 8;
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
                else if(kp()->alvTyypit()->nollaTyyppi(alvkoodi) )
                    return QString();
                else
                    return QVariant( QString("%1 %").arg( qRound64(rivi.value("alvprosentti").toDouble()) ));
            }
        }
        case KUMPPANI:
            return rivi.value("kumppani").toMap().value("nimi").toString();
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
            if( rivi.eraId() == -1 || (rivi.eraId() == rivi.id() && rivi.eraId() > 0))
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
    case TyyppiRooli:
        return rivi.tyyppi();
    case Qt::TextColorRole:
        if( rivi.tyyppi() == TositeVienti::ALVKIRJAUS)
            return QColor(Qt::darkGray);

    }

    return QVariant();
}

bool TositeViennit::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( !muokattavissa())
        return false;

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
                    rivi.setTili( uusitili.numero());
                    if( uusitili.eritellaankoTase()) {
                        rivi.setEra(-1);
                        emit dataChanged( index.sibling(index.row(), KOHDENNUS), index.sibling(index.row(), KOHDENNUS) );
                    } else
                        rivi.setEra( 0);
                    if( kp()->asetukset()->onko(AsetusModel::ALV)) {
                        int alvkoodi = uusitili.arvo("alvlaji").toInt();                     
                        if( !alvkoodi || !kp()->alvIlmoitukset()->onkoIlmoitettu(rivi.pvm())) {
                            rivi.setAlvKoodi( alvkoodi );
                            rivi.setAlvProsentti( uusitili.arvo("alvprosentti").toDouble() );
                            emit dataChanged( index.sibling(index.row(), ALV), index.sibling(index.row(), ALV) );
                            paivitaAalv(index.row());
                        }
                    }
                    if( uusitili.luku("kohdennus")) {
                        rivi.setKohdennus(uusitili.luku("kohdennus"));
                        emit dataChanged( index.sibling(index.row(), KOHDENNUS), index.sibling(index.row(), KOHDENNUS) );
                    }


                    break;
                }
            case SELITE:
                rivi.setSelite( value.toString());
                viennit_[index.row()] = rivi;
                emit dataChanged(index, index, QVector<int>() << role);
                paivitaAalv(index.row());
                return true;
            case DEBET:
                if( value.toDouble() < -1e-5)
                    rivi.setKredit( 0 - value.toDouble());
                else
                    rivi.setDebet( value.toDouble() );
                viennit_[index.row()] = rivi;
                emit dataChanged(index, index.sibling(index.row(), TositeViennit::KREDIT), QVector<int>() << Qt::EditRole);
                paivitaAalv(index.row());
                return true;
            case KREDIT:
                if( value.toDouble() < -1e-5)
                    rivi.setDebet( 0 - value.toDouble());
                else
                    rivi.setKredit( value.toDouble() );
                viennit_[index.row()] = rivi;
                emit dataChanged(index.sibling(index.row(), TositeViennit::DEBET), index, QVector<int>() << Qt::EditRole);
                paivitaAalv(index.row());
                return true;
            case KOHDENNUS:
                rivi.setKohdennus( value.toInt() );
                break;
            default:
                return false;

            }

            viennit_[index.row()] = rivi;
        } else if( role == Qt::DisplayRole && index.column() == KUMPPANI) {
            TositeVienti rivi = vienti(index.row());
            rivi.setKumppani(value.toString());
            viennit_[index.row()] = rivi;
        } else if( role == TositeViennit::EraMapRooli && index.column() != KUMPPANI) {
            TositeVienti rivi = vienti(index.row());
            rivi.setEra( value.toMap() );
            double avoin = value.toMap().value("avoin").toDouble();
            if( qAbs(avoin) > 1e-5 && qAbs(rivi.debet()) < 1e-5 && qAbs(rivi.kredit()) < 1e-5) {
                Tili* ptili = kp()->tilit()->tili( rivi.tili() );
                if( ptili && ptili->onko(TiliLaji::VASTAAVAA))
                    rivi.setKredit(avoin);
                else if(ptili && ptili->onko(TiliLaji::VASTATTAVAA))
                    rivi.setDebet(avoin);
            }

            viennit_[index.row()] = rivi;
            emit dataChanged(index.sibling(index.row(), TositeViennit::DEBET), index, QVector<int>() << Qt::EditRole);
            return true;
        } else if( role == TositeViennit::AlvKoodiRooli) {
            TositeVienti rivi = vienti(index.row());            
            rivi.setAlvKoodi( value.toInt());
            viennit_[index.row()] = rivi;
            paivitaAalv(index.row());
            emit dataChanged(index, index, QVector<int>() << Qt::EditRole);
        } else if( role == TositeViennit::AlvProsenttiRooli) {
            TositeVienti rivi = vienti(index.row());
            rivi.setAlvProsentti( value.toDouble() );
            viennit_[index.row()] = rivi;
            paivitaAalv(index.row());
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

    TositeVienti rivi = vienti(index.row());

    if( index.column() == ALV)
        return Qt::ItemIsEnabled;
    else if( index.column() == KOHDENNUS )
    {    
        Tili tili = kp()->tilit()->tiliNumerolla(rivi.tili());
        if( !tili.onko(TiliLaji::TULOS) && !tili.onko(TiliLaji::POISTETTAVA) &&  !tili.eritellaankoTase())
            return Qt::ItemIsEnabled;
    }

    if( rivi.tyyppi() == TositeVienti::ALVKIRJAUS)
        return Qt::ItemIsEnabled;

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
    while(row < rowCount()) {
        if(viennit_.at(row).toMap().value("tyyppi").toInt() == TositeVienti::ALVKIRJAUS) {
            beginRemoveRows(parent, row, row);
            viennit_.removeAt(row);
            endRemoveRows();
        } else {
            break;
        }
    }
    return true;
}

TositeVienti TositeViennit::uusi(int indeksi) const
{
    TositeVienti uusi;

    Tosite* tosite = qobject_cast<Tosite*>(parent());
    uusi.setPvm( tosite->pvm() );

    if( indeksi > 0){
        indeksi--;
        TositeVienti edellinen = viennit_.value(indeksi).toMap();
        while( edellinen.tyyppi() == TositeVienti::ALVKIRJAUS) {
            // Jotta saataisiin selite ja vastatili varsinaisesta kirjauksesta
            // eikä siihen liittyvistä alv-riveistä.
            indeksi--;
            edellinen = viennit_.value(indeksi).toMap();
        }
        Tili tili = kp()->tilit()->tiliNumerolla(edellinen.tili());
        if( tili.luku("vastatili"))
            uusi.setTili(tili.luku("vastatili"));
        uusi.setSelite(edellinen.selite());
        uusi.setPvm(edellinen.pvm());
        uusi.setKumppani(edellinen.kumppaniMap());
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

    uusi.set(TositeVienti::AALV, "+-");
    return uusi;
}

QModelIndex TositeViennit::lisaaVienti(int indeksi)
{
    while( viennit_.value(indeksi).toMap().value("tyyppi").toInt() == TositeVienti::ALVKIRJAUS)
        indeksi++;

    beginInsertRows( QModelIndex(), indeksi, indeksi);
    viennit_.insert(indeksi, uusi(indeksi));
    endInsertRows();
    paivitaAalv(indeksi);
    return index(indeksi, 0);
}

TositeVienti TositeViennit::vienti(int indeksi) const
{
    return TositeVienti( viennit_.value(indeksi).toMap() );
}

void TositeViennit::asetaVienti(int indeksi, const TositeVienti &vienti)
{
    viennit_[indeksi] = vienti;
    if( !vienti.tyyppi() )
        paivitaAalv(indeksi);

    emit dataChanged( index(indeksi, PVM), index(indeksi,SELITE) );
}

void TositeViennit::lisaa(const TositeVienti &vienti)
{
    beginInsertRows(QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append(vienti);
    endInsertRows();
    paivitaAalv(viennit_.count() - 1);
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

void TositeViennit::pohjaksi(const QDate &pvm, const QString &vanhaOtsikko, const QString &uusiOtsikko, bool sailytaErat)
{
    beginResetModel();
    for(int i=0; i < viennit_.count(); i++) {
        TositeVienti vienti = viennit_.value(i).toMap();
        if( !sailytaErat && vienti.eraId() == vienti.id())
            vienti.setEra(-1);
        vienti.remove("id");
        int siirto = vienti.pvm().daysTo(pvm);
        vienti.setPvm( pvm );
        if( vienti.jaksoalkaa().isValid())
            vienti.setJaksoalkaa( vienti.jaksoalkaa().addDays(siirto) );
        if( vienti.jaksoloppuu().isValid())
            vienti.setJaksoloppuu( vienti.jaksoloppuu().addDays(siirto));
        if( vienti.selite() == vanhaOtsikko)
            vienti.setSelite( uusiOtsikko);        
        viennit_[i] = vienti;
    }
    endResetModel();
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
   int alvMaara = 0;

   qlonglong vahennysPerusteella = 0;
   qlonglong vahennysKirjattuna= 0;
   int vahennysMaara = 0;

   qlonglong eupalveluPerusteella = 0;
   qlonglong eupalveluVero = 0;
   int eupalveluMaara=0;

   qlonglong eutavaraPerusteella = 0;
   qlonglong eutavaraVero = 0;
   int eutavaraMaara = 0;

   qlonglong rakennusPerusteella = 0;
   qlonglong rakennusVero = 0;
   int rakennusMaara = 0;

   for( auto vienti : viennit_) {
       TositeVienti tv(vienti.toMap());
       switch (tv.alvKoodi()) {
       case AlvKoodi::MYYNNIT_NETTO:
           alvPerusteella += qRound64((tv.kredit() - tv.debet()) * tv.alvProsentti());
           alvMaara++;
           break;
       case AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS:
            alvKirjattuna += qRound64((tv.kredit() - tv.debet()) * 100);            
            break;
       case AlvKoodi::OSTOT_NETTO:
           vahennysPerusteella += qRound64((tv.debet() - tv.kredit()) * tv.alvProsentti());
           vahennysMaara++;
           break;
        case AlvKoodi::OSTOT_NETTO + AlvKoodi::ALVVAHENNYS:
            vahennysKirjattuna += qRound64((tv.debet() - tv.kredit()) * 100);
           break;
       case AlvKoodi::YHTEISOHANKINNAT_PALVELUT:
           eupalveluPerusteella += qRound64((tv.debet() - tv.kredit()) * tv.alvProsentti());
           eupalveluMaara++;
           break;
       case AlvKoodi::YHTEISOHANKINNAT_PALVELUT + AlvKoodi::ALVKIRJAUS:
            eupalveluVero += qRound64((tv.kredit() - tv.debet()) * 100);
            break;
       case AlvKoodi::YHTEISOHANKINNAT_TAVARAT:
           eutavaraPerusteella += qRound64((tv.debet() - tv.kredit()) * tv.alvProsentti());
           eutavaraMaara++;
           break;
       case AlvKoodi::YHTEISOHANKINNAT_TAVARAT + AlvKoodi::ALVKIRJAUS:
            eutavaraVero += qRound64((tv.kredit() - tv.debet()) * 100);
            break;
       case AlvKoodi::RAKENNUSPALVELU_OSTO:
           rakennusPerusteella += qRound64((tv.debet() - tv.kredit()) * tv.alvProsentti());
           rakennusMaara++;
           break;
       case AlvKoodi::RAKENNUSPALVELU_OSTO + AlvKoodi::ALVKIRJAUS:
            rakennusVero += qRound64((tv.kredit() - tv.debet()) * 100);
            break;

       }

   }
   QString palaute;
   if( qAbs(alvPerusteella-alvKirjattuna) > alvMaara)
       palaute.append(tr("\nMyynneistä pitäisi tilittää arvonlisäveroa %L1 €").arg(alvPerusteella / 100.0, 0, 'f', 2));
   if( qAbs(vahennysKirjattuna-vahennysPerusteella) > vahennysMaara)
       palaute.append(tr("\nOstoista pitäisi vähentää arvonlisäveroa %L1 €").arg(vahennysPerusteella / 100.0, 0, 'f', 2));
   if( qAbs(eupalveluPerusteella-eupalveluVero) > eupalveluMaara)
       palaute.append(tr("\nPalveluiden yhteisöhankinnoista pitäisi tilittää arvonlisäveroa %L1 €").arg(eupalveluPerusteella / 100.0, 0, 'f', 2));
   if( qAbs(eutavaraPerusteella-eutavaraVero) > eutavaraMaara)
       palaute.append(tr("\nTavaroiden yhteisöhankinnoista pitäisi tilittää arvonlisäveroa %L1 €").arg(eutavaraPerusteella / 100.0, 0, 'f', 2));
   if( qAbs(rakennusPerusteella-rakennusVero) > rakennusMaara)
       palaute.append(tr("\nRakennuspalveluiden ostoista pitäisi tilittää arvonlisäveroa %L1 €").arg(rakennusPerusteella / 100.0, 0, 'f', 2));


   return palaute;
}

void TositeViennit::paivitaAalv(int rivi)
{
    TositeVienti lahde = viennit_.value(rivi).toMap();
    int alvkoodi = lahde.alvKoodi();
    double prosentti = lahde.alvProsentti();

    QString aalvtila = lahde.data(TositeVienti::AALV).toString();
    bool verorivi = aalvtila.contains("+") &&
            (alvkoodi == AlvKoodi::MYYNNIT_NETTO ||
             alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
             alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI ||
             alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
             alvkoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
             alvkoodi == AlvKoodi::MAAHANTUONTI ||
             alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO) &&
            prosentti > 1e-5;

    bool vahennysrivi = aalvtila.contains("-") &&
            (alvkoodi == AlvKoodi::OSTOT_NETTO ||
             alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
             alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
             alvkoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
             alvkoodi == AlvKoodi::MAAHANTUONTI ||
             alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO) ;

    qlonglong dsentit = qRound64( prosentti * lahde.debet() );
    qlonglong ksentit = qRound64( prosentti * lahde.kredit() );

    rivi++;
    TositeVienti seuraava = viennit_.value(rivi).toMap();
    bool onjoVerorivi = seuraava.tyyppi() == TositeVienti::ALVKIRJAUS &&
            seuraava.alvKoodi() > AlvKoodi::ALVKIRJAUS &&
            seuraava.alvKoodi() < AlvKoodi::ALVVAHENNYS;

    if( verorivi) {
        TositeVienti vero;
        vero.setPvm( lahde.pvm() );         
        vero.setAlvProsentti(prosentti);
        vero.setDebet(dsentit);
        vero.setKredit(ksentit);
        vero.setKumppani(lahde.kumppaniMap());
        vero.setSelite( QString("%1 ALV %2 %").arg( lahde.selite() ).arg( prosentti,0,'f',2 ) );
        vero.setTyyppi(TositeVienti::ALVKIRJAUS);

        if( alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
            vero.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::KOHDENTAMATONALVVELKA ).numero() );
            vero.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON );
            vero.setEra(-1);

        } else if (alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI) {
            vero.setTili( kp()->asetukset()->luku("LaskuEnnakkoALV") );
            vero.setAlvKoodi( AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON );
            vero.setEra(-1);
        } else if (alvkoodi == AlvKoodi::MYYNNIT_NETTO) {
            vero.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
            vero.setAlvKoodi( AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS);
        } else {
            vero.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
            vero.setAlvKoodi( alvkoodi + AlvKoodi::ALVKIRJAUS);
            vero.setDebet( ksentit );
            vero.setKredit( dsentit );
        }

        if(!onjoVerorivi)
            insertRow(rivi);
        asetaVienti(rivi, vero);
        rivi++;     // Mahdollinen vähennysrivi tulee tämän jälkeen
    } else {
        if( onjoVerorivi)
            removeRows(rivi, 1);
    }

    seuraava = viennit_.value(rivi).toMap();
    bool onjoVahennysrivi = seuraava.tyyppi() == TositeVienti::ALVKIRJAUS &&
            seuraava.alvKoodi() > AlvKoodi::ALVVAHENNYS &&
            seuraava.alvKoodi() < AlvKoodi::MAKSETTAVAALV;

    if( vahennysrivi ) {
        TositeVienti vahennys;
        vahennys.setPvm(lahde.pvm());
        vahennys.setAlvProsentti(prosentti);
        vahennys.setDebet(dsentit);
        vahennys.setKumppani(lahde.kumppaniMap());
        vahennys.setKredit(ksentit);
        vahennys.setSelite(QString("%1 ALV-VÄHENNYS %2 %").arg(lahde.selite()).arg(prosentti,0,'f',2));
        vahennys.setTyyppi(TositeVienti::ALVKIRJAUS);

        if( alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
            vahennys.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::KOHDENTAMATONALVSAATAVA ).numero());
            vahennys.setAlvKoodi(AlvKoodi::MAKSUPERUSTEINEN_OSTO + AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON);
            vahennys.setEra(-1);
        } else if( alvkoodi == AlvKoodi::OSTOT_NETTO) {
            vahennys.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVSAATAVA ).numero());
            vahennys.setAlvKoodi( AlvKoodi::OSTOT_NETTO + AlvKoodi::ALVVAHENNYS);
        } else {
            vahennys.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVSAATAVA ).numero());
            vahennys.setAlvKoodi( alvkoodi + AlvKoodi::ALVVAHENNYS);
        }

        if (!onjoVahennysrivi)
            insertRow(rivi);
        asetaVienti(rivi, vahennys);

    } else {
        if( onjoVahennysrivi)
            removeRows(rivi,1);
    }


}
