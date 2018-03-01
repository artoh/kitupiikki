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
        return QVariant( rivi.tili.eritellaankoTase());
    else if( role == ViiteRooli )
        return QVariant( rivi.viite );
    else if( role == SaajanTiliRooli )
        return QVariant( rivi.saajanTili );
    else if( role == SaajanNimiRooli)
        return rivi.json.str("SaajanNimi");
    else if( role == EraPvmRooli )
        return rivi.erapvm;
    else if( role == ArkistoTunnusRooli )
        return rivi.arkistotunnus;


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
                        return QVariant( tr("%1/%2").arg( era.tositteenTunniste() ).arg(era.pvm.toString(Qt::SystemLocaleShortDate)) );
                    }
                    else if( rivi.json.luku("Tasaerapoisto") )
                    {
                        // Samaan paikkaan tulee myös tieto tasapoistosta
                        int kk = rivi.json.luku("Tasaerapoisto");
                        if( kk % 12)
                            return QVariant( tr("Tasaerapoisto %1 v %2 kk").arg(kk / 12).arg(kk % 12) );
                        else
                            return QVariant( tr("Tasaerapoisto %1 v").arg(kk / 12) );
                    }
                    else if( !rivi.viite.isEmpty())
                    {
                        return tr("VIITE");
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
        if( rivi.maksaaLaskua )
            return QIcon(":/pic/lasku.png");
        return rivi.kohdennus.tyyppiKuvake();
        }
        else if( index.column() == ALV )
        {
            return kp()->alvTyypit()->kuvakeKoodilla( rivi.alvkoodi % 100 );
        }
        else if( index.column() == PVM)
        {
            // Väärät päivät punaisella
            if( rivi.pvm <= kp()->tilitpaatetty() || rivi.pvm > kp()->tilikaudet()->kirjanpitoLoppuu() )
                return QIcon(":/pic/lukittu.png");
            else if( kp()->asetukset()->pvm("AlvIlmoitus") >= rivi.pvm && rivi.alvkoodi )
                return QIcon(":/pic/vero.png");
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
            if( value.toDate().isValid())
            {
                viennit_[index.row()].pvm = value.toDate();
                emit siirryRuutuun( index.sibling(index.row(), TILI) );
            }
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
            // Tällä tilivalinnalla tulee myös oletukset veroille
            int alvlaji = uusitili.json()->luku("AlvLaji");
            // #37 Käsinkirjauksessa oletuksena netto
            if( alvlaji % 10 == 1)
                alvlaji++;
            // #40 Jos muokataan tilinavausta, ei siinä ole alveja
            if( tositeModel_ && tositeModel_->tunniste() == 0)
                alvlaji = 0;

            viennit_[index.row()].alvkoodi = alvlaji;

            if( alvlaji)
                viennit_[index.row()].alvprosentti = uusitili.json()->luku("AlvProsentti");

            emit dataChanged(index, index.sibling(index.row(), ALV));

            if( uusitili.onkoValidi())
            {
                // Jos kirjataan tulotilille, niin siirrytään syöttämään kredit-summaa
                if( uusitili.onko(TiliLaji::TULO) )
                    emit siirryRuutuun(index.sibling(index.row(), KREDIT));
                else
                    emit siirryRuutuun(index.sibling(index.row(), DEBET));
            }
            emit muuttunut();
            return true;
        }
        case SELITE:
            viennit_[index.row()].selite = value.toString();
            return true;
        case DEBET:
            viennit_[index.row()].debetSnt = value.toLongLong();
            if(value.toLongLong())
            {
                viennit_[index.row()].kreditSnt = 0;
                emit siirryRuutuun(index.sibling(index.row(), KOHDENNUS));
            }
            emit muuttunut();
            return true;
        case KREDIT:
            viennit_[index.row()].kreditSnt = value.toLongLong();
            if( value.toLongLong())
            {
                viennit_[index.row()].debetSnt = 0;
                emit siirryRuutuun(index.sibling(index.row(), KOHDENNUS));
            }
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
        viennit_[rivi].debetSnt = value.toLongLong();
        emit muuttunut();
    }
    else if( role == KreditRooli)
    {
        viennit_[rivi].kreditSnt = value.toLongLong();
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
    else if( role == ViiteRooli )
    {
        viennit_[rivi].viite = value.toString();
    }
    else if( role == SaajanTiliRooli )
    {
        viennit_[rivi].saajanTili = value.toString();
    }
    else if( role == SaajanNimiRooli)
    {
        viennit_[rivi].json.set("SaajanNimi", value.toString());
    }
    else if( role == EraPvmRooli )
    {
        viennit_[rivi].erapvm = value.toDate();
    }
    else if(role == ArkistoTunnusRooli)
    {
        viennit_[rivi].arkistotunnus = value.toString();
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
    // Kun lisätään uusi insertillä, yritetään arvata oikeat täytöt

    VientiRivi uusirivi;
    uusirivi.pvm = tositeModel_->pvm();
    uusirivi.selite = tositeModel_->otsikko();

    if( debetSumma() == kreditSumma() )
    {
        // Tositelajin oletustili
        int oletustili = tositeModel_->tositelaji().json()->luku("Oletustili");
        uusirivi.tili = kp()->tilit()->tiliNumerolla(oletustili);
        uusirivi.alvkoodi = uusirivi.tili.json()->luku("AlvLaji");

        // Kuitenkin käsin kirjattaessa käytetään bruttokirjauksia (#37)
        if( uusirivi.alvkoodi % 10 == 1)
            uusirivi.alvkoodi++;

        uusirivi.alvprosentti = uusirivi.tili.json()->luku("AlvProsentti");
    }
    else
    {
        if( debetSumma() > kreditSumma() )
            uusirivi.kreditSnt = debetSumma() - kreditSumma();
        else
            uusirivi.debetSnt = kreditSumma() - debetSumma();

        int vastatili = tositeModel_->tositelaji().json()->luku("Vastatili");
        if( !vastatili && viennit_.count())
        {
            vastatili = viennit_.last().tili.json()->luku("Vastatili");
        }
        uusirivi.tili = kp()->tilit()->tiliNumerolla( vastatili);
    }

    return lisaaVienti( uusirivi );
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

qlonglong VientiModel::debetSumma() const
{
    qlonglong summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.debetSnt;
    }
    return summa;
}

qlonglong VientiModel::kreditSumma() const
{
    qlonglong summa = 0;
    foreach (VientiRivi rivi, viennit_)
    {
        summa += rivi.kreditSnt;
    }
    return summa;
}

bool VientiModel::tallenna()
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
                          "kohdennus=:kohdennus, eraid=:eraid, alvprosentti=:alvprosentti, "
                          "viite=:viite, iban=:iban, erapvm:=erapvm, arkistotunnus=:arkistotunnus, "
                          "muokattu=:muokattu, json=:json"
                          " WHERE id=:id");
            query.bindValue(":id", rivi.vientiId);
        }
        else
        {
            query.prepare("INSERT INTO vienti(tosite,pvm,tili,debetsnt,kreditsnt,selite,"
                           "alvkoodi, alvprosentti, luotu, muokattu, json, kohdennus, eraid, vientirivi,"
                           "viite, iban, erapvm, arkistotunnus) "
                            "VALUES(:tosite,:pvm,:tili,:debetsnt,:kreditsnt,:selite,"
                            ":alvkoodi, :alvprosentti, :luotu, :muokattu, :json, :kohdennus, :eraid, :rivinro,"
                            ":viite, :iban, :erapvm, :arkistotunnus)");
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
        query.bindValue(":viite", rivi.viite);
        query.bindValue(":iban", rivi.saajanTili);
        query.bindValue(":erapvm", rivi.erapvm);
        query.bindValue(":arkistotunnus", rivi.arkistotunnus);
        query.bindValue(":json", rivi.json.toSqlJson());

        if( !query.exec() )
            return false;

        if( !rivi.vientiId )
            viennit_[i].vientiId = query.lastInsertId().toInt();

        if( rivi.maksaaLaskua)
        {
            // Tämä kirjaus maksaa laskua eli vähentää sen avointa summaa
            QSqlQuery laskunMaksuKysely;
            if( !laskunMaksuKysely.exec( QString("UPDATE lasku SET avoinSnt = avoinSnt - %1 WHERE id=%2")
                                    .arg(rivi.debetSnt).arg(rivi.maksaaLaskua) ) )
                return false;
        }
    }
    // Lopuksi pitäisi vielä poistaa ne rivit, jotka on poistettu...
    foreach (int id, poistetutVientiIdt_)
    {
        if( !query.exec( QString("DELETE FROM vienti WHERE id=%1").arg(id)) )
            return false;
    }

    muokattu_ = false;

    return true;
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
                       "kohdennus, eraid, vientirivi, viite, iban, erapvm, arkistotunnus "
                       "FROM vienti WHERE tosite=%1 "
                       "ORDER BY id").arg( tositeModel_->id() ));
    while( query.next())
    {
        VientiRivi rivi;
        rivi.vientiId = query.value("id").toInt();
        rivi.pvm = query.value("pvm").toDate();
        rivi.tili = Kirjanpito::db()->tilit()->tiliIdlla( query.value("tili").toInt());
        rivi.debetSnt = query.value("debetsnt").toLongLong();
        rivi.kreditSnt = query.value("kreditsnt").toLongLong();
        rivi.selite = query.value("selite").toString();
        rivi.alvkoodi = query.value("alvkoodi").toInt();
        rivi.alvprosentti = query.value("alvprosentti").toInt();
        rivi.luotu = query.value("luotu").toDateTime();
        rivi.muokattu = query.value("muokattu").toDateTime();
        rivi.kohdennus = kp()->kohdennukset()->kohdennus( query.value("kohdennus").toInt());
        rivi.eraId = query.value("eraid").toInt();
        rivi.riviNro = query.value("vientirivi").toInt();
        rivi.json.fromJson( query.value("json").toByteArray() );
        rivi.viite = query.value("viite").toString();
        rivi.saajanTili = query.value("iban").toString();
        rivi.erapvm = query.value("erapvm").toDate();
        rivi.arkistotunnus = query.value("arkistotunnus").toString();
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



