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



#include "laskumodel.h"
#include "laskutusverodelegaatti.h"
#include "db/kirjanpito.h"
#include "db/tilinvalintadialogi.h"
#include "kirjaus/verodialogi.h"
#include "laskuntulostaja.h"
#include "laskuryhmamodel.h"

#include <cmath>
#include <QSqlQuery>
#include <QPrinter>
#include <QBuffer>
#include <QPdfWriter>
#include <QApplication>
#include <QMessageBox>
#include <QSqlError>
#include <memory>

#include <QDebug>
#include <QSqlError>
#include <QJsonDocument>

LaskuModel::LaskuModel(QObject *parent) :
    QAbstractTableModel( parent )
{
    toimituspaiva_ = kp()->paivamaara();
    erapaiva_ = kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika"));
    kirjausperuste_ = kp()->asetukset()->luku("LaskuKirjausperuste") ;
}

LaskuModel *LaskuModel::teeHyvityslasku(int hyvitettavaVientiId)
{
    LaskuModel *model = new LaskuModel;
    model->viittausLasku_.haeLasku(hyvitettavaVientiId);

    model->tyyppi_ = HYVITYSLASKU;
    model->kirjausperuste_ = model->viittausLasku().kirjausperuste;
    model->asetaErapaiva( model->viittausLasku().erapvm );
    model->asetaLaskunsaajannimi( model->viittausLasku().asiakas );
    model->asetaOsoite( model->viittausLasku().json.str("Osoite") );
    model->asetaEmail( model->viittausLasku().json.str("Email") );
    model->asetaYTunnus( model->viittausLasku().json.str("YTunnus"));
    model->asetaToimituspaiva( model->viittausLasku().json.date("Toimituspvm"));
    model->asetaLisatieto( tr("Hyvityslasku laskulle %1, päiväys %2")
                                     .arg( model->viittausLasku().viite)
                                     .arg( model->viittausLasku().pvm.toString("dd.MM.yyyy")));


    return model;
}

LaskuModel *LaskuModel::teeMaksumuistutus(int muistutettavaVientiId)
{
    LaskuModel *model = new LaskuModel;
    model->viittausLasku_.haeLasku(muistutettavaVientiId);

    model->tyyppi_ = MAKSUMUISTUTUS;
    model->kirjausperuste_ = model->viittausLasku().kirjausperuste;
    model->asetaLaskunsaajannimi( model->viittausLasku().asiakas );
    model->asetaOsoite( model->viittausLasku().json.str("Osoite") );
    model->asetaEmail( model->viittausLasku().json.str("Email") );
    model->asetaYTunnus( model->viittausLasku().json.str("YTunnus"));
    model->asetaToimituspaiva( model->viittausLasku().json.date("Toimituspvm"));

    model->asetaLisatieto( tr("Kirjanpitomme mukaan emme ole saaneet maksusuoritustanne. Pyydämme maksuanne alla olevan erittelyn mukaisesti. "
                              "Mikäli olette jo maksaneet laskun tai lasku on virheellinen, ottakaa yhteyttä asian selvittämiseksi.") );


    model->haeAvoinSaldo();
    return model;
}



LaskuModel *LaskuModel::haeLasku(int vientiId)
{
    LaskuModel *model = new LaskuModel;

    AvoinLasku lasku;
    lasku.haeLasku(vientiId);

    if( lasku.json.luku("Hyvityslasku"))
    {
        model->tyyppi_ = HYVITYSLASKU;
        model->viittausLasku_.haeLasku( lasku.eraId );
    }
    else if( lasku.json.luku("Maksumuistutus"))
    {
        model->tyyppi_ = MAKSUMUISTUTUS;
        model->viittausLasku_.haeLasku( lasku.eraId );
        model->haeAvoinSaldo();
    }

    model->kirjausperuste_ = lasku.kirjausperuste;
    model->asetaLaskunsaajannimi( lasku.asiakas );
    model->asetaOsoite( lasku.json.str("Osoite") );
    model->asetaEmail( lasku.json.str("Email"));
    model->asetaYTunnus( lasku.json.str("YTunnus"));
    model->asetaToimituspaiva( lasku.json.date("Toimituspvm"));
    model->tositeId_ = lasku.tosite;
    model->vientiId_ = lasku.vientiId;
    model->laskunNumero_ = lasku.viite.toULongLong();


    QVariantList lista = lasku.json.variant("Laskurivit").toList();
    for( const QVariant& var : lista)
    {
        LaskuRivi rivi;
        QVariantMap map = var.toMap();

        rivi.nimike = map.value("Nimike").toString();
        rivi.maara = map.value("Maara").toDouble();
        rivi.yksikko = map.value("Yksikko").toString();
        rivi.ahintaSnt = map.value("YksikkohintaSnt").toDouble();
        rivi.alvKoodi = map.value("Alvkoodi").toInt();
        rivi.alvProsentti = map.value("Alvprosentti").toInt();
        rivi.myyntiTili = kp()->tilit()->tiliNumerolla( map.value("Tili").toInt() );
        rivi.kohdennus = kp()->kohdennukset()->kohdennus( map.value("Kohdennus").toInt() );
        rivi.tuoteKoodi = map.value("Tuotekoodi").toInt();
        model->rivit_.append(rivi);
    }

    model->muokattu_ = false;
    return model;
}

LaskuModel *LaskuModel::ryhmaLasku()
{
    LaskuModel *model = new LaskuModel();
    model->tyyppi_ = RYHMALASKU;
    model->ryhma_ = new LaskuRyhmaModel(model);
    return model;
}


int LaskuModel::rowCount(const QModelIndex & /* parent */) const
{
    if( tyyppi() == MAKSUMUISTUTUS)
        return  rivit_.count() + 1;
    return rivit_.count();
}

int LaskuModel::columnCount(const QModelIndex & /* parent */) const
{
    return 8;
}

QVariant LaskuModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case NIMIKE:
                return tr("Nimike");
            case MAARA:
                return tr("Määrä");
            case YKSIKKO:
                return tr("Yksikkö");
            case AHINTA :
                return tr("á netto");
            case ALV:
                return tr("Alv");
            case TILI:
                return tr("Tili");
            case KOHDENNUS:
                return tr("Kohdennus");
            case BRUTTOSUMMA:
                return tr("Yhteensä");
        }

    }
    return QVariant( section + 1);
}

QVariant LaskuModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==BRUTTOSUMMA || index.column() == MAARA || index.column() == ALV || index.column() == AHINTA)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }

    LaskuRivi rivi;
    if( tyyppi() == LaskuModel::MAKSUMUISTUTUS)
    {
        // Maksumuistutuksessa ylimmäksi riviksi tulostuu laskun avoin saldo
        if( index.row() > 0)
        {
            rivi = rivit_.value(index.row() - 1);
        }
        else
        {
            if( role == Qt::DisplayRole )
            {
                switch(index.column())
                {
                    case NIMIKE:
                        return tr("Laskun %1 avoin saldo").arg(viittausLasku().viite);
                    case BRUTTOSUMMA:
                        return QString("%L1 €").arg( avoinSaldo() / 100.0,0,'f',2);
                }
            }
            else if( role == Qt::EditRole && index.column() == BRUTTOSUMMA)
                return avoinSaldo();

            return QVariant();
        }
    }
    else
    {
        rivi = rivit_.value(index.row());
    }

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column()) {
        case NIMIKE:
            return rivi.nimike;
        case MAARA:
            if( role == Qt::DisplayRole)
                return QString("%L1").arg(rivi.maara,0,'f',2);
            else
                return rivi.maara;
        case YKSIKKO:
            return rivi.yksikko;
        case AHINTA:
            if( role == Qt::DisplayRole)
                return QString("%L1 €").arg(rivi.ahintaSnt / 100.0,0,'f',2);
            else
                return rivi.ahintaSnt;
        case ALV:
            switch (rivi.alvKoodi) {
            case AlvKoodi::EIALV :
                return QVariant();
            case AlvKoodi::ALV0:
                return tr("Veroton myynti");
            case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
                return tr("AVL 8 c §");
            case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
            case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
                return tr("AVL 72 a §");
            default:
                return QVariant( QString("%1 %").arg(rivi.alvProsentti));
            }
        case KOHDENNUS:
            if( role == Qt::DisplayRole)
            {
                if( rivi.kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    return rivi.kohdennus.nimi();
            }
            else if( role == Qt::EditRole)
                return rivi.kohdennus.id();
            return QVariant();
        case TILI:
            if( rivi.myyntiTili.numero())
                return QVariant( QString("%1 %2").arg(rivi.myyntiTili.numero()).arg(rivi.myyntiTili.nimi()) );
            return QVariant();
        case BRUTTOSUMMA:
            if( role == Qt::DisplayRole)
                return QString("%L1 €").arg(rivi.yhteensaSnt() / 100.0,0,'f',2);
            else
                return rivi.yhteensaSnt();
        }
    }
    else if( role == NimikeRooli)
        return rivi.nimike;
    else if( role == AlvKoodiRooli)
        return rivi.alvKoodi;
    else if( role == AlvProsenttiRooli)
        return rivi.alvProsentti;
    else if( role == NettoRooli)
    {
        return rivi.maara * rivi.ahintaSnt;
    }
    else if( role == VeroRooli)
    {
        return rivi.yhteensaSnt() - rivi.maara * rivi.ahintaSnt;
    }
    else if( role == TuoteKoodiRooli)
        return rivi.tuoteKoodi;
    else if( role == Qt::DecorationRole && index.column() == ALV)
    {
        // Jos käytössä maksuperusteinen alv, niin nettokirjaukset muunnetaan tallennusvaiheessa
        // maksuperusteisiksi kirjauksiksi
        if( rivi.alvKoodi % 100 == AlvKoodi::MYYNNIT_NETTO && kp()->onkoMaksuperusteinenAlv( pvm() ) )
            return kp()->alvTyypit()->kuvakeKoodilla( AlvKoodi::MAKSUPERUSTEINEN_MYYNTI );

        return kp()->alvTyypit()->kuvakeKoodilla( rivi.alvKoodi % 100 );
    }

    return QVariant();
}

bool LaskuModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int rivi = index.row();
    if( tyyppi() == MAKSUMUISTUTUS)
        rivi = rivi - 1;    // Alussa avoin saldo

    if( role == Qt::EditRole)
    {
        muokattu_ = true;

        switch (index.column()) {
        case NIMIKE:
            rivit_[rivi].nimike = value.toString();
            return true;
        case MAARA:
            rivit_[rivi].maara = value.toDouble();
            // Uusi summa
            paivitaSumma(rivi);
            return true;

        case AHINTA:
            rivit_[rivi].ahintaSnt = value.toInt();
            if( value.toInt())
            {
                // Lisätään loppuun aina automaattisesti uusi rivi
                // kun tähän on syötetty kelpo summa
                if( rivi == rivit_.count() - 1)
                    lisaaRivi();
                // Uusi summa
                paivitaSumma(rivi);
            }
            return true;
        case YKSIKKO:
            rivit_[rivi].yksikko = value.toString();
            return true;
        case KOHDENNUS:
            rivit_[rivi].kohdennus = kp()->kohdennukset()->kohdennus(value.toInt());
            return true;

        case TILI:
        {
            // Tili asetetaan numerolla!
            Tili uusitili;
            if( value.toInt() )
                uusitili = kp()->tilit()->tiliNumerolla( value.toInt());
            else if(!value.toString().isEmpty() && value.toString() != " " && value.toString() != "0")
                uusitili = TilinValintaDialogi::valitseTili(value.toString());
            else if( value.toString()==" " || !rivit_[rivi].myyntiTili.onkoValidi())
                uusitili = TilinValintaDialogi::valitseTili( QString());

            if( uusitili.onkoValidi())
                rivit_[rivi].myyntiTili = uusitili;
            return true;
        }
        case BRUTTOSUMMA:
            // Lasketaan bruton avulla nettoyksikköhinta ja laitetaan se paikalleen
            if( qAbs(rivit_[rivi].maara) < 0.0001)
                return false;

            int alvprosentti = rivit_[rivi].alvProsentti;
            double netto =  100.0 * value.toInt() / rivit_[rivi].maara / ( 100.0 + alvprosentti) ;
            rivit_[rivi].ahintaSnt = netto;
            paivitaSumma(rivi);

            // Lisätään loppuun aina automaattisesti uusi rivi
            // kun tähän on syötetty kelpo summa
            if( rivi == rivit_.count() - 1)
                lisaaRivi();
            return true;
        }
    }
    else if( role == AlvKoodiRooli)
    {
        rivit_[rivi].alvKoodi = value.toInt();
        paivitaSumma(rivi);
        muokattu_ = true;
        return true;
    }
    else if( role == AlvProsenttiRooli)
    {
        rivit_[rivi].alvProsentti = value.toInt();
        paivitaSumma(rivi);
        muokattu_ = true;
        return true;
    }
    else if( role == TuoteKoodiRooli)
    {
        rivit_[rivi].tuoteKoodi = value.toInt();
        muokattu_ = true;
        return true;
    }

    return false;
}

Qt::ItemFlags LaskuModel::flags(const QModelIndex &index) const
{
    if( index.row() > 0 || tyyppi() != MAKSUMUISTUTUS)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return  QAbstractTableModel::flags(index);  // Avoin summa ei ole muokattavissa!

}

qlonglong LaskuModel::laskunSumma() const
{
    qlonglong summa = avoinSaldo();     // Maksumuistusta varten
    foreach (LaskuRivi rivi, rivit_)
    {
        summa += std::round(rivi.yhteensaSnt());
    }
    return summa;
}

LaskuRivi LaskuModel::rivi(int indeksi) const
{
    return rivit_.value(indeksi);
}

QDate LaskuModel::pvm() const
{
    if( kirjausperuste()==SUORITEPERUSTE)
        return toimituspaiva();
    else if(kirjausperuste()==LASKUTUSPERUSTE || kirjausperuste()==KATEISLASKU)
        return kp()->paivamaara();
    else
        return QDate(); // Ei saada päivämäärää (Maksuperuste)
}

qulonglong LaskuModel::laskunro() const
{
    if( laskunNumero_)
        return laskunNumero_;

    qulonglong pohjanro = kp()->asetukset()->isoluku("LaskuSeuraavaId") / 10;
    if( pohjanro < 100)
        pohjanro = 100;

    while(true)
    {
        // Lasketaan aina tunnistenumero uudelleen!!!
        qulonglong numero = pohjanro * 10 + laskeViiteTarkiste(pohjanro);
        // Varmistetaan, että tämä numero ei vielä ole käytössä!
        QSqlQuery kysely(QString("SELECT viite FROM vienti WHERE viite='%1' AND iban IS NULL").arg(numero));
        if( !kysely.next())
            return numero;
        pohjanro++;
    }
}

QString LaskuModel::viitenumero() const
{
    // Muotoilee viitenumeron tulosteasun
    QString str = QString::number( laskunro() );
    for(int i=0; i< str.count() / 5; i++)
    {
        str.insert(i * 5 + i,' ');
    }
    return str;
}

bool LaskuModel::tallenna(Tili rahatili)
{
    // Ensin tehdään tosite
    TositeModel tosite( kp()->tietokanta() );

    if( tositeId_)
    {
        tosite.lataa(tositeId_);
        // Tyhjennetään tosite
        while( tosite.vientiModel()->rowCount(QModelIndex()))
            tosite.vientiModel()->poistaRivi(0);
        // Poistetaan vanha liite
        while( tosite.liiteModel()->rowCount(QModelIndex()))
            tosite.liiteModel()->poistaLiite(0);

    }

    if( tyyppi() == HYVITYSLASKU )
        tosite.asetaOtsikko( tr("%1 [Hyvityslasku %2]").arg(laskunsaajanNimi()).arg(laskunro()) );
    else
        tosite.asetaOtsikko( tr("%1 [%2]").arg(laskunsaajanNimi()).arg(laskunro()) );

    tosite.asetaKommentti( lisatieto() );

    if( kirjausperuste() == MAKSUPERUSTE )
    {
        tosite.asetaTositelaji(-1);
    }
    else
        tosite.asetaTositelaji( kp()->asetukset()->luku("LaskuTositelaji", 1) );


    tosite.json()->set("Lasku", laskunro());

    tosite.asetaPvm(pvm() );

    if(  (tyyppi() == HYVITYSLASKU || tyyppi() == MAKSUMUISTUTUS) && kirjausperuste() == MAKSUPERUSTE )
    {
        // Maksuperusteinen kirjataan samaan tositteeseen alkuperäisen laskun kanssa
        tosite.lataa( viittausLasku().tosite );
        // Lisätään hyvityslaskun kommentit
        tosite.asetaKommentti( tosite.kommentti() + "\n\n[Hyvityslasku]\n" + lisatieto() );
    }

    // Tallennetaan liite
    // Luo tilapäisen pdf-tiedoston

    QString liiteOtsikko = tr("Lasku nr %1").arg(laskunro());
    LaskunTulostaja tulostaja(this);

    int liitenro = tosite.liiteModel()->lisaaLiite( tulostaja.pdf(), liiteOtsikko );


    // #96 Laskun kirjaaminen yhdistelmäriveillä
    // 11.3.2017 v0.8 Muutettu niin, että kirjanpitoon ei enää kirjata yksittäisiä
    // tuoterivejä, vaan yhdistelmäviennit jokaiselta erilaiselta veroryhmältä
    QList<VientiRivi> vientiRivit;
    QVariantList rivitTalteen;
    // key: AlvKoodi * 1000 + alvProsentti - value: alv sentteinä
    QMap<int,qlonglong> veroSentit;

    VientiModel *viennit = tosite.vientiModel();

    // #123: Jos rahatilillä käytössä kohdennukset, tehdään kohdennus jos koko maksu samaa kohdennusta
    Kohdennus tasekohdennus;
    if( rivit_.count() && rahatili.json()->luku("Kohdennukset"))
        tasekohdennus = rivit_.first().kohdennus;

    foreach (LaskuRivi rivi, rivit_)
    {
        if( qAbs(rivi.ahintaSnt) < 0.001 )
            continue;   // Ei tyhjiä rivejä

        // Maksuperusteisen koodaus, jos käytössä maksuperusteinen
        if( kp()->onkoMaksuperusteinenAlv(pvm()) && rivi.alvKoodi == AlvKoodi::MYYNNIT_NETTO &&
                ( kirjausperuste() == SUORITEPERUSTE || kirjausperuste() == LASKUTUSPERUSTE  ))
        {
                rivi.alvKoodi = AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;
        }

        // Tallennetaan laskun json-kenttään myös rivitiedot
        // mahdollisen myöhemmän käytön varalle (näistä voisi koota vaikkapa myyntitilastot)
        QVariantMap riviTalteen;
        riviTalteen["Nimike"] = rivi.nimike;
        riviTalteen["Tili"] = rivi.myyntiTili.numero();
        riviTalteen["Alvkoodi"] = rivi.alvKoodi;
        riviTalteen["Alvprosentti"] = rivi.alvProsentti;
        riviTalteen["Maara"] = QString("%1").arg(rivi.maara,0,'f',2);
        riviTalteen["Yksikko"] = rivi.yksikko;
        if( rivi.tuoteKoodi)
            riviTalteen["Tuotekoodi"] = rivi.tuoteKoodi;
        riviTalteen["YksikkohintaSnt"] = rivi.ahintaSnt;
        riviTalteen["Kohdennus"] = rivi.kohdennus.id();

        qlonglong nettoSnt = qRound( rivi.ahintaSnt * rivi.maara );
        qlonglong bruttoSnt = qRound( rivi.yhteensaSnt() );
        qlonglong veroSnt = bruttoSnt - nettoSnt;

        riviTalteen["Nettoyht"] = nettoSnt;
        riviTalteen["Alv"] = veroSnt;
        riviTalteen["Yhteensa"] = bruttoSnt;

        rivitTalteen.append(riviTalteen);

        // Etsitään, löytyykö jo sellainen vienti, johon tämän rivin voipi yhdistää
        int vientiin = -1;
        for( int vI=0; vI < vientiRivit.count(); vI++)
        {
            VientiRivi vientiRivi = vientiRivit.at(vI);

            if( vientiRivi.tili.id() == rivi.myyntiTili.id() &&
                vientiRivi.kohdennus.id() == rivi.kohdennus.id() &&
                vientiRivi.alvkoodi == rivi.alvKoodi &&
                vientiRivi.alvprosentti == rivi.alvProsentti)
            {
                vientiin = vI;
                break;
            }
        }

        if( vientiin == -1)
        {
            VientiRivi uusi;
            uusi.selite = tr("%1 [%2]").arg(laskunsaajanNimi()).arg(laskunro());
            uusi.pvm = pvm();
            uusi.tili = rivi.myyntiTili;
            uusi.kohdennus = rivi.kohdennus;
            uusi.alvkoodi = rivi.alvKoodi;
            uusi.alvprosentti = rivi.alvProsentti;

            // Lisätään tämä rivi ja merkitään sen indeksi aktiiviseksi
            vientiRivit.append(uusi);
            vientiin = vientiRivit.count() - 1;
        }

        qlonglong summaSnt = vientiRivit.at(vientiin).kreditSnt - vientiRivit.at(vientiin).debetSnt;
        summaSnt += nettoSnt;

        if( summaSnt > 0)
        {
            vientiRivit[vientiin].kreditSnt = summaSnt;
            vientiRivit[vientiin].debetSnt = 0;
        }
        else
        {
            vientiRivit[vientiin].debetSnt =  0 - summaSnt;
            vientiRivit[vientiin].kreditSnt = 0;
        }

        // Tallennetaan myös asiaankuuluvaan verotauluun
        veroSentit[ rivi.alvKoodi * 1000 + rivi.alvProsentti] = veroSentit.value( rivi.alvKoodi * 1000 + rivi.alvProsentti, 0) + veroSnt;

        // Kohdennustarkastus
        if( rivi.kohdennus.id() != tasekohdennus.id() )
            tasekohdennus = Kohdennus();

    }
    // Kirjataan maksurivit vienteihin
    for( const VientiRivi& vienti : vientiRivit)
        viennit->lisaaVienti(vienti);

    // Kirjataan nettokirjausten alv-kirjaukset
    QMapIterator<int,qlonglong> veroIter(veroSentit);

    while( veroIter.hasNext())
    {
        veroIter.next();

        VientiRivi verorivi;
        // Nettokirjauksessa kirjataan alv erikseen
        if( veroIter.key() / 1000 == AlvKoodi::MYYNNIT_NETTO || veroIter.key() / 1000 == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI)
        {
            int alvprosentti = veroIter.key() % 1000;
            verorivi.pvm = pvm();
            verorivi.selite = tr("%1 [%2] ALV %3 %").arg(laskunsaajanNimi()).arg(laskunro()).arg(alvprosentti);

            if( veroIter.key() / 1000 == AlvKoodi::MYYNNIT_NETTO)
            {
                verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
                verorivi.alvkoodi = AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO;
            }
            else    // Maksuperusteinen ALV
            {
                verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA);
                verorivi.alvkoodi = AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;
                verorivi.eraId = TaseEra::UUSIERA;
            }

            verorivi.alvprosentti = alvprosentti;


            if( veroIter.value() > 0)
            {
                verorivi.kreditSnt = veroIter.value();
            }
            else
            {
                verorivi.debetSnt = 0 - veroIter.value();
            }
            if( verorivi.tili.onkoValidi())
                viennit->lisaaVienti(verorivi);
        }
    }

    VientiRivi raharivi;
    raharivi.vientiId = vientiId_;
    raharivi.tili =  kirjausperuste() == MAKSUPERUSTE ? Tili() : rahatili;      // Maksuperusteinen kirjataan NULL-tilille
    raharivi.pvm = kirjausperuste() == MAKSUPERUSTE ? kp()->paivamaara() : pvm();
    raharivi.selite = tr("%1 [%2]").arg(laskunsaajanNimi()).arg(laskunro());
    raharivi.kohdennus = tasekohdennus;

    if( laskunSumma() > 0 )
        raharivi.debetSnt = laskunSumma() - avoinSaldo();
    else
        raharivi.kreditSnt = 0 - laskunSumma() - avoinSaldo();


    // Sitten tälle rahariville kirjataan aiemmin laskut-taulussa olleet tiedot
    // #149 since 0.11

    raharivi.viite = QString::number( laskunro() );
    raharivi.asiakas = laskunsaajanNimi();
    raharivi.erapvm = erapaiva();
    raharivi.laskupvm = kp()->paivamaara();

    // Käteislaskulta ei jää velkaa, joten eräId:ksi tulee NULL
    raharivi.eraId = kirjausperuste() == KATEISLASKU ? TaseEra::EIERAA :  TaseEra::UUSIERA;

    if( tyyppi() == HYVITYSLASKU )
    {
        raharivi.json.set("Hyvityslasku", viittausLasku().viite.toULongLong());
        raharivi.eraId = viittausLasku().eraId;        // EräId
    }
    else if( tyyppi() == MAKSUMUISTUTUS)
    {
        raharivi.json.set("Maksumuistutus", viittausLasku().viite.toULongLong());
        raharivi.eraId = viittausLasku().eraId;
    }
    else if( kirjausperuste() != KATEISLASKU)
        raharivi.eraId = TaseEra::UUSIERA;

    raharivi.json.set("Osoite", osoite());
    raharivi.json.set("Toimituspvm", toimituspaiva());
    raharivi.json.set("Lisatieto", lisatieto());
    raharivi.json.set("Email", email());
    raharivi.json.setVar("Laskurivit", rivitTalteen);
    raharivi.json.set("Kirjausperuste", kirjausperuste());
    raharivi.json.set("Liite", liitenro);
    if( !ytunnus().isEmpty() )
        raharivi.json.set("YTunnus", ytunnus());

    viennit->lisaaVienti(raharivi);
    if( !tosite.tallenna() )
    {
        return false;
    }

    if( laskunro() > kp()->asetukset()->isoluku("LaskuSeuraavaId"))
        kp()->asetukset()->aseta("LaskuSeuraavaId", laskunro() );

    return true;
}

unsigned int LaskuModel::laskeViiteTarkiste(qulonglong luvusta)
{
    int indeksi = 0;
    int summa = 0;

    while( luvusta > 0)
    {
        int tarkasteltava = luvusta % 10;
        luvusta = luvusta / 10;
        if( indeksi % 3 == 0)
            summa += 7 * tarkasteltava;
        else if( indeksi % 3 == 1)
            summa += 3 * tarkasteltava;
        else
            summa += tarkasteltava;
        indeksi++;
    }
    return ( 10 - summa % 10) % 10;

}

QString LaskuModel::tositetunnus()
{
    if( kirjausperuste() == LaskuModel::MAKSUPERUSTE)
        return QString();

    if( tositeId_ )
    {
        TositeModel tositeModel( kp()->tietokanta());
        tositeModel.lataa(tositeId_);
        return QString("%1%2/%3")
               .arg(tositeModel.tositelaji().tunnus() ).arg( tositeModel.tunniste() )
                .arg( kp()->tilikausiPaivalle( tositeModel.pvm() ).kausitunnus());
    }
    else
    {
        Tositelaji laji = kp()->tositelajit()->tositelaji( kp()->asetukset()->luku("LaskuTositelaji") );
        return QString("%1%2/%3").arg(laji.tunnus()).arg(laji.seuraavanTunnistenumero( pvm() ))
                          .arg( kp()->tilikausiPaivalle(kp()->paivamaara()).kausitunnus());
    }
}

void LaskuModel::lisaaRivi(LaskuRivi rivi)
{
    int rivia = rivit_.count();
    if( rivia  && qAbs(rivit_.value(rivia-1).ahintaSnt) < 0.001)
    {
        // Jos viimeisenä on tyhjä rivi, korvataan se tällä ja lisätään sen jälkeen uusi
        rivit_[rivia-1] = rivi;
        emit dataChanged( index(rivia-1, 0), index(rivia-1, columnCount(QModelIndex())) );
        rivi = LaskuRivi();     // Tyhjennetään uusi rivi
    }

    beginInsertRows( QModelIndex(), rivit_.count(), rivit_.count());
    rivit_.append(rivi);
    endInsertRows();
    emit summaMuuttunut(laskunSumma());
}

void LaskuModel::poistaRivi(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    rivit_.removeAt(indeksi);
    endRemoveRows();
    emit summaMuuttunut(laskunSumma());
}

void LaskuModel::haeAvoinSaldo()
{
    // Haetaan maksumuistusta varten tämän erän avoin saldo

    QString kysymys = QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM vienti WHERE eraid=%1 AND id!=%2")
            .arg( viittausLasku().eraId ).arg( vientiId_ );

    QSqlQuery kysely(kysymys);
    if( kysely.next())
    {
        avoinSaldo_ = kysely.value(0).toLongLong() - kysely.value(1).toLongLong();
    }

}

void LaskuModel::paivitaSumma(int rivi)
{
    if( tyyppi() == MAKSUMUISTUTUS)
        rivi++; // Koska ylimmällä rivillä avoin saldo
    emit dataChanged( createIndex(rivi, BRUTTOSUMMA), createIndex(rivi, BRUTTOSUMMA) );
    emit summaMuuttunut(laskunSumma());

}

LaskuRivi::LaskuRivi()
{
    if( kp()->asetukset()->onko("AlvVelvollinen"))
    {
        alvKoodi = AlvKoodi::MYYNNIT_NETTO;
        alvProsentti = VerotyyppiModel::oletusAlvProsentti();
    }
    else
        alvKoodi = AlvKoodi::EIALV;

    Tositelaji laji = kp()->tositelajit()->tositelaji( kp()->asetukset()->luku("LaskuTositelaji") );
    myyntiTili = kp()->tilit()->tiliNumerolla( laji.json()->luku("Oletustili") );
}

double LaskuRivi::yhteensaSnt() const
{
    // TODO: Eri alv-tyypeillä
    return std::round( maara *  ahintaSnt * (100 + alvProsentti ) / 100 );
}
