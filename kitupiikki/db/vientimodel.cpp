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

#include "db/vientimodel.h"
#include "db/tositemodel.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "db/tilinvalintadialogi.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

VientiModel::VientiModel(TositeModel *tositemodel) : tositeModel_(tositemodel), muokattu_(false)
{

}

int VientiModel::rowCount(const QModelIndex & /* parent */) const
{
    return viennit_.count();
}

int VientiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 7;
}

QVariant VientiModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                return QVariant("Pvm");
            case TILI:
                return QVariant("Tili");
            case DEBET :
                return QVariant("Debet");
            case KREDIT:
                return QVariant("Kredit");
            case ALV:
                return QVariant("Alv");
            case SELITE:
                return QVariant("Selite");
            case KOHDENNUS :
                return QVariant("Kohdennus");

        }

    }
    return QVariant( section + 1);
}

QVariant VientiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    VientiRivi rivi = viennit_.value( index.row() );

    if( role == IdRooli)
        return QVariant( rivi.vientiId );
    else if( role == PvmRooli)
        return QVariant( rivi.pvm );
    else if( role == TiliNumeroRooli)
        return QVariant( rivi.tili.numero());
    else if( role == DebetRooli)
        return QVariant( rivi.debetSnt );
    else if( role == KreditRooli)
        return QVariant( rivi.kreditSnt);
    else if( role == AlvKoodiRooli)
        return QVariant( rivi.alvkoodi );
    else if( role == AlvProsenttiRooli)
        return QVariant( rivi.alvprosentti);
    else if( role == KohdennusRooli)
        return QVariant( rivi.kohdennus.id());
    else if( role == LuotuRooli)
        return QVariant( rivi.luotu);
    else if( role == MuokattuRooli)
        return QVariant( rivi.muokattu);
    else if( role == RiviRooli)
        return QVariant( rivi.riviNro );
    else if( role == EraIdRooli)
        return QVariant( rivi.eraId );
    else if( role == PoistoKkRooli)
    {
        if( !rivi.tili.onko(TiliLaji::TASAERAPOISTO))
            return -1;
        return QVariant( rivi.json.luku("Tasaerapoisto") );
    }
    else if( role == TaseErittelyssaRooli)
        return QVariant( rivi.tili.taseErittelyTapa() == Tili::TASEERITTELY_TAYSI);


    else if( role==Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case PVM: return QVariant( rivi.pvm );

            case TILI:
                if( rivi.tili.numero())
                    return QVariant( QString("%1 %2").arg(rivi.tili.numero()).arg(rivi.tili.nimi()) );
                else
                    return QVariant();

            case DEBET:
                if( role == Qt::EditRole)
                    return QVariant( rivi.debetSnt);
                else if( rivi.debetSnt )
                    return QVariant( QString("%L1 €").arg(rivi.debetSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case KREDIT:
                if( role == Qt::EditRole)
                    return QVariant( rivi.kreditSnt);
                else if( rivi.kreditSnt )
                    return QVariant( QString("%L1 €").arg(rivi.kreditSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case ALV:
                if( rivi.alvkoodi == AlvKoodi::EIALV)
                    return QVariant();
                else
                    return QVariant( QString("%1 %").arg(rivi.alvprosentti));
                // TODO: Alv-lajit (esim. pieninä kuvakkeina)


            case SELITE: return QVariant( rivi.selite );
            case KOHDENNUS:
                if( role == Qt::DisplayRole)
                {
                    // Tase-erät näytetään samalla sarakkeella
                    if( rivi.eraId )
                    {
                        TaseEra era(rivi.eraId);
                        TositeTunniste tunniste = era.tositteenTunniste();
                        return QVariant( tr("%1/%2").arg(tunniste.tunnus).arg(era.pvm.toString(Qt::SystemLocaleShortDate)) );
                    }
                    else if( rivi.json.luku("Tasapoisto") )
                    {
                        // Samaan paikkaan tulee myös tieto tasapoistosta
                        int kk = rivi.json.luku("Tasapoisto");
                        if( kk % 12)
                            return QVariant( tr("Tasapoisto %1 v %2 kk").arg(kk / 12).arg(kk % 12) );
                        else
                            return QVariant( tr("Tasapoisto %1 v").arg(kk / 12) );
                    }
                    else
                    {
                        if( rivi.kohdennus.tyyppi() == Kohdennus::EIKOHDENNETA)
                            // Ei kohdennusta näyttää tyhjää
                            return QVariant();
                        else
                            return QVariant(rivi.kohdennus.nimi()  );
                    }
                }
                else if(role == Qt::EditRole)
                    return QVariant( rivi.kohdennus.id());
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET || index.column() == ALV)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::DecorationRole)
    {
        if( index.column() == KOHDENNUS)
        {
        if( rivi.eraId )
            return QIcon(":/pic/folder.png");
        return rivi.kohdennus.tyyppiKuvake();
        }
        else if( index.column() == ALV )
        {
            return kp()->alvTyypit()->kuvakeKoodilla( rivi.alvkoodi % 100 );
        }
    }
    else if( role == Qt::TextColorRole)
    {
        if( index.column() == ALV)
        {
            // Alv-kirjaukset harmajalla
            if( rivi.alvkoodi > 800)
                return QColor(Qt::darkGray);
        }
    }

    return QVariant();
}

bool VientiModel::setData(const QModelIndex &index, const QVariant &value, int  role )
{
    muokattu_ = true;

    int rivi = index.row();

    // EditRole käsittelee käyttäjän vientiruudukossa tekemät muutokset
    if( role == Qt::EditRole)
    {


        switch (index.column())
        {
        case PVM:
            viennit_[index.row()].pvm = value.toDate();
            emit siirryRuutuun( index.sibling(index.row(), TILI) );
            return true;
        case TILI:
        {
            // Tili asetetaan numerolla!
            Tili uusitili;
            if( value.toInt())
                uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
            else if(!value.toString().isEmpty() && value.toString() != " ")
                uusitili = TilinValintaDialogi::valitseTili(value.toString());
            else
                uusitili = TilinValintaDialogi::valitseTili( QString());

            viennit_[index.row()].tili = uusitili;
            // Jos kirjataan tulotilille, niin siirrytään syöttämään kredit-summaa
            if( uusitili.onko(TiliLaji::TULO) )
                emit siirryRuutuun(index.sibling(index.row(), KREDIT));
            else
                emit siirryRuutuun(index.sibling(index.row(), DEBET));
            return true;
        }
        case SELITE:
            viennit_[index.row()].selite = value.toString();
            return true;
        case DEBET:
            viennit_[index.row()].debetSnt = value.toInt();
            if(value.toInt())
            {
                viennit_[index.row()].kreditSnt = 0;
                emit siirryRuutuun(index.sibling(index.row(), KOHDENNUS));
            }
            emit muuttunut();
            return true;
        case KREDIT:
            viennit_[index.row()].kreditSnt = value.toInt();
            if( value.toInt())
            viennit_[index.row()].debetSnt = 0;
            emit siirryRuutuun(index.sibling(index.row(), KOHDENNUS));
            emit muuttunut();
            return true;
        case KOHDENNUS:
            viennit_[rivi].kohdennus = kp()->kohdennukset()->kohdennus(value.toInt());
            return true;
        default:
            return false;
        }
    }

    // Käytettäessä omia rooleja muokkaus tulee ohjelmallisesti eli
    // ei ole tarvetta siirtää käyttäjää ruudusta toiseen

    if( role == PvmRooli)
        viennit_[rivi].pvm = value.toDate();
    else if( role == TiliNumeroRooli)
        viennit_[rivi].tili = kp()->tilit()->tiliNumerolla( value.toInt() );
    else if( role == SeliteRooli)
        viennit_[rivi].selite = value.toString();
    else if( role == DebetRooli)
    {
        viennit_[rivi].debetSnt = value.toInt();
        emit muuttunut();
    }
    else if( role == KreditRooli)
    {
        viennit_[rivi].kreditSnt = value.toInt();
        emit muuttunut();
    }
    else if( role == KohdennusRooli)
        viennit_[rivi].kohdennus=kp()->kohdennukset()->kohdennus( value.toInt());
    else if( role == AlvKoodiRooli)
        viennit_[rivi].alvkoodi = value.toInt();
    else if( role == AlvProsenttiRooli)
        viennit_[rivi].alvprosentti = value.toInt();
    else if( role == EraIdRooli )
        viennit_[rivi].eraId = value.toInt();
    else if( role == PoistoKkRooli)
    {
        if( !value.toInt())
            viennit_[rivi].json.unset("Tasaerapoisto");
        else
            viennit_[rivi].json.set("Tasaerapoisto", value.toInt());
    }
    else
        return false;

    return true;
}

Qt::ItemFlags VientiModel::flags(const QModelIndex &index) const
{

    VientiRivi rivi = viennit_.value(index.row());

    // Vientien muokkaus: Jos model sallii

    if( tositeModel_->muokkausSallittu() )
    {
        // Alv-saraketta ei voi suoraan muokata, vaan siihen tarvitaan oma dialogi
        // Samoin kohdennusta voi muokata vain, jos tili ei ole tasetili
        if( index.column() == ALV )
        {
            return QAbstractTableModel::flags(index) | Qt::ItemIsEnabled;
        }
        else if( index.column() == KOHDENNUS)
        {
            if( !rivi.tili.numero() || rivi.tili.onko(TiliLaji::TASE))
                return QAbstractTableModel::flags(index);
        }

        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }
    else
        return QAbstractTableModel::flags(index);
}

bool VientiModel::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    beginInsertRows( QModelIndex(), row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit_.insert(row, uusiRivi() );
    endInsertRows();
    emit muuttunut();
    // emit vientejaOnTaiEi(true);
    return true;
}

void VientiModel::poistaRivi(int rivi)
{
    // Jos vienti on tietokannassa, pitää se poistaa myöskin sieltä
    if( viennit_[rivi].vientiId)
        poistetutVientiIdt_.append( viennit_[rivi].vientiId);

    beginRemoveColumns( QModelIndex(), rivi, rivi);
    viennit_.removeAt(rivi);
    endRemoveRows();
    emit muuttunut();       // Rivin poisto muuttaa debet/kredit täsmäystä

}


QModelIndex VientiModel::lisaaVienti()
{
    return lisaaVienti( VientiRivi() );
}

QModelIndex VientiModel::lisaaVienti(VientiRivi rivi)
{
    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());

    rivi.riviNro = seuraavaRiviNumero();
    viennit_.append( rivi );

    endInsertRows();
    emit muuttunut();   // Debet / kredit täsmäytykseen
    return index( viennit_.count() - 1, 0);
}

int VientiModel::debetSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.debetSnt;
    }
    return summa;
}

int VientiModel::kreditSumma() const
{
    int summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.kreditSnt;
    }
    return summa;
}

void VientiModel::tallenna()
{
    QSqlQuery query(*tositeModel_->tietokanta());
    for(int i=0; i < viennit_.count() ; i++)
    {
        VientiRivi rivi = viennit_[i];

        if(( rivi.kreditSnt == 0 && rivi.debetSnt == 0) || rivi.tili.id() == 0)
            continue;       // "Tyhjä" rivi, ei tallenneta

        if( rivi.vientiId )
        {
            query.prepare("UPDATE vienti SET pvm=:pvm, tili=:tili, debetsnt=:debetsnt, "
                          "kreditsnt=:kreditsnt, selite=:selite, alvkoodi=:alvkoodi,"
                          "kohdennus=:kohdennus, eraid=:eraid, alvprosentti=:alvprosentti, muokattu=:muokattu, json=:json"
                          " WHERE id=:id");
            query.bindValue(":id", rivi.vientiId);
        }
        else
        {
            query.prepare("INSERT INTO vienti(tosite,pvm,tili,debetsnt,kreditsnt,selite,"
                           "alvkoodi, alvprosentti, luotu, muokattu, json, kohdennus, eraid, vientirivi) "
                            "VALUES(:tosite,:pvm,:tili,:debetsnt,:kreditsnt,:selite,"
                            ":alvkoodi, :alvprosentti, :luotu, :muokattu, :json, :kohdennus, :eraid, :rivinro)");
            query.bindValue(":luotu",  QDateTime(kp()->paivamaara(), QTime::currentTime() ) );
            query.bindValue(":rivinro", rivi.riviNro);
        }


        query.bindValue(":tosite", tositeModel_->id() );
        query.bindValue(":pvm", rivi.pvm);
        query.bindValue(":tili", rivi.tili.id());

        if( rivi.debetSnt )
            query.bindValue(":debetsnt", rivi.debetSnt);
        else
            query.bindValue(":debetsnt", QVariant());

        if( rivi.kreditSnt)
            query.bindValue(":kreditsnt", rivi.kreditSnt);
        else
            query.bindValue(":kreditsnt", QVariant());

        if( rivi.eraId )
            query.bindValue(":eraid", rivi.eraId);
        else
            query.bindValue(":eraid", QVariant());

        query.bindValue(":selite", rivi.selite);
        query.bindValue(":alvkoodi", rivi.alvkoodi);
        query.bindValue(":alvprosentti", rivi.alvprosentti);
        query.bindValue(":muokattu", QVariant( QDateTime(kp()->paivamaara(), QTime::currentTime() ) ) );
        query.bindValue(":kohdennus", rivi.kohdennus.id());
        query.bindValue(":json", rivi.json.toSqlJson());
        query.exec();

        if( !rivi.vientiId )
            viennit_[i].vientiId = query.lastInsertId().toInt();

        if( rivi.maksaaLaskua)
        {
            // Tämä kirjaus maksaa laskua eli vähentää sen avointa summaa
            QSqlQuery laskunMaksuKysely;
            laskunMaksuKysely.exec( QString("UPDATE lasku SET avoinSnt = avoinSnt - %1 WHERE id=%2")
                                    .arg(rivi.debetSnt).arg(rivi.maksaaLaskua) );
        }
    }
    // Lopuksi pitäisi vielä poistaa ne rivit, jotka on poistettu...
    foreach (int id, poistetutVientiIdt_)
    {
        query.exec( QString("DELETE FROM vienti WHERE id=%1").arg(id));
    }

    muokattu_ = false;
}

void VientiModel::tyhjaa()
{
    beginResetModel();
    viennit_.clear();
    endResetModel();
    muokattu_ = false;
    emit muuttunut();
}

void VientiModel::lataa()
{
    beginResetModel();
    viennit_.clear();

    QSqlQuery query( *tositeModel_->tietokanta() );
    query.exec(QString("SELECT id, pvm, tili, debetsnt, kreditsnt, selite, "
                       "alvkoodi, alvprosentti, luotu, muokattu, json, "
                       "kohdennus, eraid, vientirivi "
                       "FROM vienti WHERE tosite=%1 "
                       "ORDER BY id").arg( tositeModel_->id() ));
    while( query.next())
    {
        VientiRivi rivi;
        rivi.vientiId = query.value("id").toInt();
        rivi.pvm = query.value("pvm").toDate();
        rivi.tili = Kirjanpito::db()->tilit()->tiliIdlla( query.value("tili").toInt());
        rivi.debetSnt = query.value("debetsnt").toInt();
        rivi.kreditSnt = query.value("kreditsnt").toInt();
        rivi.selite = query.value("selite").toString();
        rivi.alvkoodi = query.value("alvkoodi").toInt();
        rivi.alvprosentti = query.value("alvprosentti").toInt();
        rivi.luotu = query.value("luotu").toDateTime();
        rivi.muokattu = query.value("muokattu").toDateTime();
        rivi.kohdennus = kp()->kohdennukset()->kohdennus( query.value("kohdennus").toInt());
        rivi.eraId = query.value("eraid").toInt();
        rivi.riviNro = query.value("vientirivi").toInt();
        rivi.json.fromJson( query.value("json").toByteArray() );
        viennit_.append(rivi);
    }

    qDebug() << query.lastQuery() << " - " << query.lastError().text();

    endResetModel();
    muokattu_ = false;
    emit muuttunut();
}



VientiRivi VientiModel::uusiRivi()
{
    VientiRivi uusirivi;

    uusirivi.riviNro = seuraavaRiviNumero();

    int debetit = debetSumma();
    int kreditit = kreditSumma();

    // Ensimmäiseen vientiin kopioidaan tositteen otsikko ja päivämäärä
    if( !viennit_.count() )
    {
        // Uuden rivin pvm tositteen päivämäärästä
        // Jos tositetyypille on määrätty oletustili, otetaan se käyttöön
        uusirivi.pvm = tositeModel_->pvm();
        uusirivi.selite = tositeModel_->otsikko();

        if( tositeModel_->tositelaji().json()->luku("Oletustili") )
            uusirivi.tili = kp()->tilit()->tiliNumerolla(  tositeModel_->tositelaji().json()->luku("Oletustili") );
    }
    else
    {
        // Päivämäärä edellisestä kirjauksesta
        uusirivi.pvm = viennit_.last().pvm;

        // Jos kirjaukset eivät täsmää, tarvitaan vastatiliä jolle lasketaan jo vastatilisummaa
        if( kreditit >= 0 && debetit >= 0 && kreditit != debetit && viennit_.count())
        {
            if( kreditit > debetit)
                uusirivi.debetSnt = kreditit - debetit;
            else
                uusirivi.kreditSnt = debetit - kreditit;

            uusirivi.selite = viennit_.last().selite;

            // Arvataan edellisen tilin vastatiliä tai tositelajin vastatiliä
            Tili edellinen = viennit_.last().tili;
            if( edellinen.json()->luku("Vastatili"))
            {
                uusirivi.tili = kp()->tilit()->tiliNumerolla( edellinen.json()->luku("Vastatili") );
            }
            else if( tositeModel_->tositelaji().json()->luku("Vastatili"))
            {
                uusirivi.tili = kp()->tilit()->tiliNumerolla(  tositeModel_->tositelaji().json()->luku("Vastatili") );
            }
        }
    }

    return uusirivi;
}

int VientiModel::seuraavaRiviNumero()
{
    int seuraava = 1;
    foreach (VientiRivi rivi, viennit_)
    {
        if( rivi.riviNro >= seuraava )
            seuraava = rivi.riviNro + 1;
    }
    return seuraava;
}



