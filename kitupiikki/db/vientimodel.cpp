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
                    return QVariant("-");
                else
                    return QVariant( QString("%1 %").arg(rivi.alvprosentti));
                // TODO: Alv-lajit (esim. pieninä kuvakkeina)


            case SELITE: return QVariant( rivi.selite );
            case KOHDENNUS: return QVariant(rivi.kohdennus.nimi()  );
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

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
            else if(!value.toString().isEmpty())
                uusitili = TilinValintaDialogi::valitseTili(value.toString());

            viennit_[index.row()].tili = uusitili;
            // Jos kirjataan tulotilille, niin siirrytään syöttämään kredit-summaa
            if( uusitili.onkoTulotili())
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
                emit siirryRuutuun(index.sibling(index.row(), SELITE));
            }
            emit muuttunut();
            return true;
        case KREDIT:
            viennit_[index.row()].kreditSnt = value.toInt();
            if( value.toInt())
            viennit_[index.row()].debetSnt = 0;
            emit siirryRuutuun(index.sibling(index.row(), SELITE));
            emit muuttunut();
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
        viennit_[rivi].debetSnt = value.toInt();
    else if( role == KreditRooli)
        viennit_[rivi].kreditSnt = value.toInt();
    else if( role == KohdennusRooli)
        viennit_[rivi].kohdennus=kp()->kohdennukset()->kohdennus( value.toInt());
    else
        return false;

    return true;
}

Qt::ItemFlags VientiModel::flags(const QModelIndex &index) const
{

    VientiRivi rivi = viennit_.value(index.row());

    // Vientien muokkaus: Jos model sallii (ei ole järjestelmätosite eikä
    // päätetyllä tilikaudella) eikä ole automaattinen alv-nettokirjaus

    if( tositeModel_->muokkausSallittu() && rivi.riviNro < 10000 )
    {
        // Alv-saraketta ei voi suoraan muokata, vaan siihen tarvitaan oma dialogi
        // Samoin kohdennusta voi muokata vain, jos tili ei ole tasetili
        if( index.column() == ALV )
        {
            return QAbstractTableModel::flags(index);
        }
        else if( index.column() == KOHDENNUS)
        {
            if( !rivi.tili.numero() || rivi.tili.onkoTasetili())
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

bool VientiModel::lisaaRivi()
{
    return insertRows( rowCount(QModelIndex()), 1, QModelIndex() );

}

bool VientiModel::lisaaVienti(const QDate &pvm, int tilinumero, const QString &selite, int debetSnt, int kreditSnt, int rivinro)
{
    VientiRivi uusi;
    if( rivinro)
        uusi.riviNro = rivinro;
    else
        uusi.riviNro = seuraavaRiviNumero();

    uusi.pvm = pvm;
    uusi.tili = kp()->tilit()->tiliNumerolla(tilinumero);
    uusi.selite = selite;
    uusi.debetSnt = debetSnt;
    uusi.kreditSnt = kreditSnt;

    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append( uusi );
    endInsertRows();

    return true;
}

QModelIndex VientiModel::lisaaVienti()
{
    beginInsertRows( QModelIndex(), viennit_.count(), viennit_.count());

    viennit_.append( VientiRivi());

    endInsertRows();
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
        if( rivi.vientiId )
        {
            query.prepare("UPDATE vienti SET pvm=:pvm, tili=:tili, debetsnt=:debetsnt, "
                          "kreditsnt=:kreditsnt, selite=:selite, alvkoodi=:alvkoodi,"
                          "kohdennus=:kohdennus, alvprosentti=:alvprosentti, muokattu=:muokattu, json=:json"
                          " WHERE id=:id");
            query.bindValue(":id", rivi.vientiId);
        }
        else
        {
            query.prepare("INSERT INTO vienti(tosite,pvm,tili,debetsnt,kreditsnt,selite,"
                           "alvkoodi, luotu, muokattu, json, kohdennus, vientirivi) "
                            "VALUES(:tosite,:pvm,:tili,:debetsnt,:kreditsnt,:selite,"
                            ":alvkoodi, :luotu, :muokattu, :json, :kohdennus, :rivinro)");
            query.bindValue(":luotu",  QDateTime(kp()->paivamaara(), QTime::currentTime() ) );
            query.bindValue(":rivinro", rivi.riviNro);
        }


        query.bindValue(":tosite", tositeModel_->id() );
        query.bindValue(":pvm", rivi.pvm);
        query.bindValue(":tili", rivi.tili.id());
        query.bindValue(":debetsnt", rivi.debetSnt);
        query.bindValue(":kreditsnt", rivi.kreditSnt);
        query.bindValue(":selite", rivi.selite);
        query.bindValue(":alvkoodi", rivi.alvkoodi);
        query.bindValue(":alvprosentti", rivi.alvprosentti);
        query.bindValue(":muokattu", QVariant( QDateTime(kp()->paivamaara(), QTime::currentTime() ) ) );
        query.bindValue(":kohdennus", rivi.kohdennus.id());
        query.bindValue(":json", rivi.json.toSqlJson());
        query.exec();

        if( !rivi.vientiId )
            viennit_[i].vientiId = query.lastInsertId().toInt();
    }
    // Lopuksi pitäisi vielä poistaa ne rivit, jotka on poistettu...
    // Tätä varten voisi ylläpitää poistettujen vientien Id-listaa

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
                       "kohdennus, vientirivi "
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

    uusirivi.pvm = tositeModel_->pvm();
    uusirivi.riviNro = seuraavaRiviNumero();

    int debetit = debetSumma();
    int kreditit = kreditSumma();

    // Ensimmäiseen vientiin kopioidaan tositteen otsikko
    if( !viennit_.count() )
        uusirivi.selite = tositeModel_->otsikko();
    // Täytetään automaattisesti, elleivät tilit vielä täsmää
    else if( kreditit >= 0 && debetit >= 0 && kreditit != debetit)
    {
        if( kreditit > debetit)
            uusirivi.debetSnt = kreditit - debetit;
        else
            uusirivi.kreditSnt = debetit - kreditit;

        uusirivi.selite = viennit_.last().selite;
    }

    // TODO: Tilin "arvaaminen"


    return uusirivi;
}

int VientiModel::seuraavaRiviNumero()
{
    int seuraava = 1;
    foreach (VientiRivi rivi, viennit_)
    {
        // 10000 suuremmat rivinumerot ovat alv-vientejä
        if( rivi.riviNro >= seuraava && rivi.riviNro < 10000)
            seuraava = rivi.riviNro + 1;
    }
    return seuraava;
}



