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
#include "alvlaskelma.h"
#include "db/kirjanpito.h"
#include "db/verotyyppimodel.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "db/tositetyyppimodel.h"
#include "model/tositeliitteet.h"
#include "alvsivu.h"

#include <QDebug>

AlvLaskelma::AlvLaskelma(QObject *parent) :
  Raportteri (parent),
  tosite_( new Tosite(this))
{

}

AlvLaskelma::~AlvLaskelma()
{
}

void AlvLaskelma::kirjoitaLaskelma()
{
    kirjoitaOtsikot();
    kirjoitaYhteenveto();
    kirjoitaErittely();
}

void AlvLaskelma::kirjoitaOtsikot()
{
    rk.asetaOtsikko( tr("ARVONLISÄVEROLASKELMA"));
    rk.asetaKausiteksti( QString("%1 - %2").arg(alkupvm_.toString("dd.MM.yyyy")).arg(loppupvm_.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();
    rk.lisaaSarake("TOSITE12345");
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("24,00 ");
    rk.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Selite");
    otsikko.lisaa("%",1,true);
    otsikko.lisaa("€",1,true);
    rk.lisaaOtsake(otsikko);
}

void AlvLaskelma::kirjoitaYhteenveto()
{

    RaporttiRivi otsikko;
    otsikko.lisaa(tr("Arvonlisäveroilmoituksen tiedot"),4);
    otsikko.lihavoi();
    rk.lisaaRivi(otsikko);
    rk.lisaaTyhjaRivi();

    if( suhteutuskuukaudet_ ) {
        if( liikevaihto_ < 1000000)
            huojennus_ = verohuojennukseen_;
        else {
            huojennus_ = verohuojennukseen_ - ( (liikevaihto_ - 1000000) * verohuojennukseen_ ) / 2000000;
        }
        if( huojennus_ > verohuojennukseen_)
            huojennus_ = verohuojennukseen_;
        if( huojennus_ < 0)
            huojennus_ = 0;
    }

    // Kotimaan myynti
    yvRivi(301, tr("Suoritettava 24%:n vero kotimaan myynnistä"), kotimaanmyyntivero(2400) );
    yvRivi(302, tr("Suoritettava 14%:n vero kotimaan myynnistä"), kotimaanmyyntivero(1400));
    yvRivi(303, tr("Suoritettava 10%:n vero kotimaan myynnistä"), kotimaanmyyntivero(1000));

    rk.lisaaTyhjaRivi();

    yvRivi(304, tr("Vero tavaroiden maahantuonnista EU:n ulkopuolelta"), taulu_.summa( AlvKoodi::MAAHANTUONTI + AlvKoodi::ALVKIRJAUS ));
    yvRivi(305, tr("Vero tavaraostoista muista EU-maista"),taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT + AlvKoodi::ALVKIRJAUS));
    yvRivi(306,tr("Vero palveluostoista muista EU-maista"),taulu_.summa(AlvKoodi::YHTEISOMYYNTI_PALVELUT + AlvKoodi::ALVKIRJAUS));

    rk.lisaaTyhjaRivi();

    yvRivi(309, tr("0-verokannan alainen liikevaihto"), taulu_.summa(AlvKoodi::ALV0));
    yvRivi(310, tr("Tavaroiden maahantuonnit EU:n ulkopuolelta"), taulu_.summa(AlvKoodi::MAAHANTUONTI));
    yvRivi(311, tr("Tavaroiden myynnit muihin EU-maihin"), taulu_.summa(AlvKoodi::YHTEISOMYYNTI_TAVARAT));
    yvRivi(312, tr("Palveluiden myynnit muihin EU-maihin"), taulu_.summa(AlvKoodi::YHTEISOMYYNTI_PALVELUT));
    yvRivi(313, tr("Tavaraostot muista EU-maista"), taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT));
    yvRivi(314, tr("Palveluostot muista EU-maista"), taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_PALVELUT));

    yvRivi(318, tr("Vero rakentamispalveluiden ja metalliromun ostoista"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_OSTO + AlvKoodi::ALVKIRJAUS));
    yvRivi(319, tr("Rakentamispalveluiden ja metaalliromun myynnit"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_MYYNTI));
    yvRivi(320, tr("Rakentamispalveluiden ja metalliromun ostot"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_OSTO));

    yvRivi(315, tr("Alarajahuojennukseen oikeuttava liikevaihto"), liikevaihto_ );
    yvRivi(316, tr("Alarajahuojennukseen oikeuttava vero"), verohuojennukseen_);
    yvRivi(317, tr("Alarajahuojennuksen määrä"), huojennus());

    rk.lisaaTyhjaRivi();

    yvRivi(307, tr("Verokauden vähennettävä vero"), taulu_.summa(200,299));
    maksettava_ = taulu_.summa(100,199) - taulu_.summa(200,299) - huojennus();
    if( maksettava_ > 0)
        yvRivi(308,tr("Maksettava vero"),maksettava_);
    else
        yvRivi(308, tr("Palautettava vero"), maksettava_);

    rk.lisaaTyhjaRivi();

}

void AlvLaskelma::kirjaaVerot()
{
    qlonglong vero = taulu_.summa(100,199);
    qlonglong vahennys = taulu_.summa(200,299);

    QString selite = tr("Arvonlisävero %1 - %2")
            .arg( alkupvm_.toString("dd.MM.yyyy") )
            .arg( loppupvm_.toString("dd.MM.yyyy"));

    if( vero ) {
        TositeVienti verot;
        verot.setSelite(selite);
        verot.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
        verot.setDebet( vero / 100.0 );
        verot.setAlvKoodi( AlvKoodi::TILITYS );

        lisaaKirjausVienti(verot);
    }
    if( vahennys )
    {
        TositeVienti vahennysRivi;
        vahennysRivi.setSelite(selite);
        vahennysRivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero() );
        vahennysRivi.setKredit( vahennys / 100.0  );
        vahennysRivi.setAlvKoodi( AlvKoodi::TILITYS);

        lisaaKirjausVienti( vahennysRivi );
    }
    if( vero != vahennys) {
        TositeVienti maksu;
        maksu.setSelite( selite );
        maksu.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero() );
        maksu.setAlvKoodi( AlvKoodi::TILITYS );
        if( vero > vahennys )
            maksu.setKredit( ( vero - vahennys) / 100.0 );
        else
            maksu.setDebet( ( vero - vahennys) / 100.0 );

        lisaaKirjausVienti( maksu );
    }
}

void AlvLaskelma::kirjoitaErittely()
{
    QMapIterator<int, KoodiTaulu> koodiIter(taulu_.koodit);
    while( koodiIter.hasNext()) {
        koodiIter.next();
        int koodi = koodiIter.key();

        const KoodiTaulu &taulu = koodiIter.value();
        QMapIterator<int, KantaTaulu> kantaIter(taulu.kannat);
        while( kantaIter.hasNext()) {
            kantaIter.next();
            double verokanta = kantaIter.key() / 100.0;

            RaporttiRivi kantaOtsikko;
            kantaOtsikko.lisaa( kp()->alvTyypit()->yhdistelmaSeliteKoodilla(koodi), 3 );
            kantaOtsikko.lisaa( QString("%L1").arg(verokanta,0,'f',2));
            kantaOtsikko.lisaa( kantaIter.value().summa(debetistaKoodilla(koodi)) );
            kantaOtsikko.lihavoi();
            rk.lisaaRivi(kantaOtsikko);

            QMapIterator<int,TiliTaulu> tiliIter( kantaIter.value().tilit );
            while( tiliIter.hasNext()) {
                tiliIter.next();

                RaporttiRivi tiliOtsikko;
                tiliOtsikko.lisaa( kp()->tilit()->tiliNumerolla( tiliIter.key() ).nimiNumero(), 4 );
                rk.lisaaRivi(tiliOtsikko);

                for(auto vienti : tiliIter.value().viennit) {
                    RaporttiRivi rivi;
                    rivi.lisaa( vienti.value("pvm").toDate() );
                    rivi.lisaa( vienti.value("tosite").toMap().value("tunniste").toString() );
                    rivi.lisaa( vienti.value("selite").toString());
                    rivi.lisaa(  QString("%L1").arg(verokanta,0,'f',2) );

                    qlonglong debetsnt = qRound(vienti.value("debet").toDouble() * 100);
                    qlonglong kreditsnt = qRound( vienti.value("kredit").toDouble() * 100);

                    if( debetistaKoodilla( koodi ) )
                        rivi.lisaa( debetsnt - kreditsnt );
                    else
                        rivi.lisaa( kreditsnt - debetsnt );

                    rk.lisaaRivi( rivi );
                }
                // Tilin summa
                RaporttiRivi tiliSumma;
                tiliSumma.lisaa(QString(), 3);
                tiliSumma.lisaa(  QString("%L1").arg(verokanta,0,'f',2) );
                tiliSumma.lisaa( tiliIter.value().summa( debetistaKoodilla(koodi) ) );
                tiliSumma.viivaYlle();
                rk.lisaaRivi(tiliSumma);
                rk.lisaaTyhjaRivi();
            }
        }
    }
}

void AlvLaskelma::yvRivi(int koodi, const QString &selite, qlonglong sentit)
{
    if( sentit ) {
        RaporttiRivi rivi;
        rivi.lisaa(QString(), 1);
        rivi.lisaa( QString::number(koodi));
        rivi.lisaa(selite, 2);
        rivi.lisaa( sentit);
        rk.lisaaRivi(rivi);
    }
    if( koodi && sentit)
        koodattu_.insert(koodi, sentit);
}

qlonglong AlvLaskelma::kotimaanmyyntivero(int prosentinsadasosa)
{
    return taulu_.koodit.value(AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::MYYNNIT_BRUTTO + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa();
}

void AlvLaskelma::hae()
{
    KpKysely* kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", alkupvm_);
    kysely->lisaaAttribuutti("loppupvm", loppupvm_);
    connect( kysely, &KpKysely::vastaus, this, &AlvLaskelma::viennitSaapuu);
    kysely->kysy();
}

void AlvLaskelma::laske(const QDate &alkupvm, const QDate &loppupvm)
{
    alkupvm_ = alkupvm;
    loppupvm_ = loppupvm;

    QDate huojennusalku;
    QDate huojennusloppu;

    // Tässä huomioidaan ensin alarajahuojennus
    // TODO: Muu verokausi kuin kuukausi

    if( loppupvm == kp()->tilikaudet()->tilikausiPaivalle(loppupvm).paattyy() ) {
        huojennusalku = kp()->tilikaudet()->tilikausiPaivalle(loppupvm).alkaa();
        huojennusloppu = loppupvm;
    }

    if( huojennusalku.isValid()) {
        if( huojennusalku.day() == 1)
            suhteutuskuukaudet_ = 1;
        else
            suhteutuskuukaudet_ = 0;
        for( QDate pvm = huojennusalku.addMonths(1); pvm < loppupvm; pvm = pvm.addMonths(1))
            suhteutuskuukaudet_++;

        if( loppupvm.addDays(1).day() != 1)
            suhteutuskuukaudet_--;

        // Sitten tehdään huojennushaku
        KpKysely* kysely = kpk("/viennit");
        kysely->lisaaAttribuutti("alkupvm", huojennusalku);
        kysely->lisaaAttribuutti("loppupvm", huojennusloppu);
        connect( kysely, &KpKysely::vastaus, this, &AlvLaskelma::laskeHuojennus);
        kysely->kysy();
    } else
        hae();
}

void AlvLaskelma::viennitSaapuu(QVariant *viennit)
{

    taulu_.koodit.clear();

    QVariantList lista = viennit->toList();
    for(auto item : lista) {
        QVariantMap map = item.toMap();
        if( map.value("alvkoodi").toInt() )
            taulu_.lisaa(map);
    }

    // Valmistelutoimet pitäisi tehdän vain jos ilmoitusta ei annettu
    oikaiseBruttoKirjaukset();

    kirjoitaLaskelma();

    kirjaaVerot();

    emit valmis( rk );
}

void AlvLaskelma::laskeHuojennus(QVariant *viennit)
{
    liikevaihto_ = 0;
    verohuojennukseen_ = 0;

    for( QVariant var : viennit->toList() ) {
        TositeVienti vienti = var.toMap();

        if( vienti.tyyppi() == TositeVienti::BRUTTOOIKAISU)
            continue;

        int alvkoodi = vienti.alvKoodi();
        qlonglong debet = qRound( vienti.debet() * 100);
        qlonglong kredit = qRound( vienti.kredit() * 100);

        if( alvkoodi > 0 && alvkoodi < 100) {
            // Tämä on veron tai vähennyksen peruste
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO ||
                    alvkoodi == AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
                    alvkoodi == AlvKoodi::ALV0 ||
                    alvkoodi == AlvKoodi::RAKENNUSPALVELU_MYYNTI) {
                liikevaihto_ += kredit - debet;
            } else if( alvkoodi == AlvKoodi::MYYNNIT_BRUTTO) {
                qlonglong brutto = kredit - debet;
                qlonglong netto = qRound( ( 100 * brutto / (100 + vienti.alvProsentti()) )) ;
                liikevaihto_ += netto;
                verohuojennukseen_ += brutto - netto;
            } else if( alvkoodi == AlvKoodi::OSTOT_BRUTTO) {
                qlonglong brutto = debet - kredit;
                qlonglong netto = qRound( ( 100 * brutto / (100 + vienti.alvProsentti()) )) ;
                verohuojennukseen_ -= brutto - netto;
            }
        } else if( alvkoodi > 100 && alvkoodi < 200) {
            // Tämä on maksettava vero
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS) {
                verohuojennukseen_ += kredit - debet;
            }
        } else if( alvkoodi > 200 && alvkoodi < 300) {
            verohuojennukseen_ -= debet - kredit;
        }
    }

    liikevaihto_ = liikevaihto_ * 12 / suhteutuskuukaudet_;
    hae();

}

void AlvLaskelma::tallennusValmis()
{
    kp()->asetukset()->aseta("AlvIlmoitus", loppupvm_);
    emit tallennettu();
}

void AlvLaskelma::kirjaaHuojennus()
{
    if( !huojennus() || !kp()->asetukset()->luku("AlvHuojennusTili"))
        return;

    QString selite = tr("Arvonlisäveron alarajahuojennus");

    TositeVienti huojennettava;
    huojennettava.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero() );
    huojennettava.setSelite( selite );
    huojennettava.setDebet( huojennus() / 100.0 );
    lisaaKirjausVienti( huojennettava );

    TositeVienti huojentaja;
    huojentaja.setTili( kp()->asetukset()->luku("AlvHuojennusTili") );
    huojentaja.setSelite( selite );
    huojentaja.setKredit( huojennus() / 100.0);
    lisaaKirjausVienti( huojentaja );
}

void AlvLaskelma::tallenna()
{
    tosite_->setData( Tosite::PVM, loppupvm_ );
    tosite_->setData( Tosite::OTSIKKO, tr("Arvonlisäveroilmoitus %1 - %2")
                     .arg(alkupvm_.toString("dd.MM.yyyy"))
                     .arg(loppupvm_.toString("dd.MM.yyyy")));
    tosite_->setData( Tosite::TYYPPI, TositeTyyppi::ALVLASKELMA  );

    tosite_->liitteet()->lisaa( rk.pdf(), "alv" );

    QVariantMap lisat;
    QVariantMap koodit;
    QMapIterator<int,qlonglong> iter(koodattu_);
    while( iter.hasNext()) {
        iter.next();
        koodit.insert( QString::number( iter.key() ), iter.value() / 100.0 );
    }
    lisat.insert("koodit", koodit);
    lisat.insert("kausialkaa", alkupvm_);
    lisat.insert("kausipaattyy", loppupvm_);
    lisat.insert("erapvm", AlvSivu::erapaiva(loppupvm_));
    lisat.insert("maksettava", maksettava() / 100.0);
    tosite_->setData( Tosite::ALV, lisat);


    connect( tosite_, &Tosite::talletettu, this, &AlvLaskelma::tallennusValmis);
    tosite_->tallenna();
}

void AlvLaskelma::lisaaKirjausVienti(TositeVienti vienti)
{
    vienti.setPvm( loppupvm_ );
    taulu_.lisaa(vienti);

    tosite_->viennit()->lisaa(vienti);
}

void AlvLaskelma::oikaiseBruttoKirjaukset()
{
    QMapIterator<int,KantaTaulu> myyntiIter( taulu_.koodit.value( AlvKoodi::MYYNNIT_BRUTTO ).kannat );
    while( myyntiIter.hasNext())
    {
        myyntiIter.next();
        QMapIterator<int,TiliTaulu> tiliIter( myyntiIter.value().tilit );
        while( tiliIter.hasNext())
        {
            tiliIter.next();
            int tili = tiliIter.key();
            qlonglong brutto = tiliIter.value().summa();
            int sadasosaprosentti = myyntiIter.key();

            qlonglong netto = brutto * 10000 / ( 10000 + sadasosaprosentti);
            qlonglong vero = sadasosaprosentti * netto / 10000;

            QString selite = tr("Bruttomyyntien oikaisu %3 BRUTTO %L1, NETTO %L2")
                    .arg(brutto / 100.0, 0, 'f', 2 )
                    .arg(netto/100.0, 0, 'f', 2)
                    .arg( kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

            TositeVienti pois;
            pois.setTili(tili);
            pois.setDebet( vero / 100.0 );
            pois.setAlvKoodi( AlvKoodi::MYYNNIT_BRUTTO );
            pois.setAlvProsentti( sadasosaprosentti / 100.0 );
            pois.setSelite(selite);
            lisaaKirjausVienti( pois );

            TositeVienti veroon;
            veroon.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVVELKA ).numero() );
            veroon.setKredit( vero / 100.0);
            veroon.setAlvKoodi( AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_BRUTTO);
            veroon.setAlvProsentti( sadasosaprosentti / 100.0);
            veroon.setSelite(selite);
            lisaaKirjausVienti( veroon );
        }
    }

    QMapIterator<int,KantaTaulu> osto( taulu_.koodit.value( AlvKoodi::OSTOT_BRUTTO ).kannat );
    while( myyntiIter.hasNext())
    {
        myyntiIter.next();
        QMapIterator<int,TiliTaulu> tiliIter( myyntiIter.value().tilit );
        while( tiliIter.hasNext())
        {
            tiliIter.next();
            int tili = tiliIter.key();
            qlonglong brutto = tiliIter.value().summa();
            int sadasosaprosentti = myyntiIter.key();

            qlonglong netto = brutto * 10000 / ( 10000 + sadasosaprosentti);
            qlonglong vero = sadasosaprosentti * netto / 10000;

            QString selite = tr("Brutto-ostojen oikaisu %3 BRUTTO %L1, NETTO %L2")
                    .arg(brutto / 100.0, 0, 'f', 2 )
                    .arg(netto/100.0, 0, 'f', 2)
                    .arg( kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

            TositeVienti pois;
            pois.setTili(tili);
            pois.setKredit( vero / 100.0 );
            pois.setAlvKoodi( AlvKoodi::OSTOT_BRUTTO );
            pois.setAlvProsentti( sadasosaprosentti / 100.0 );
            pois.setSelite(selite);
            pois.setTyyppi( TositeVienti::BRUTTOOIKAISU );
            lisaaKirjausVienti( pois );

            TositeVienti veroon;
            veroon.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVVELKA ).numero() );
            veroon.setDebet( vero / 100.0);
            veroon.setAlvKoodi( AlvKoodi::ALVKIRJAUS + AlvKoodi::OSTOT_BRUTTO);
            veroon.setAlvProsentti( sadasosaprosentti / 100.0);
            veroon.setSelite(selite);
            veroon.setTyyppi( TositeVienti::BRUTTOOIKAISU );
            lisaaKirjausVienti( veroon );
        }
    }
}

bool AlvLaskelma::debetistaKoodilla(int alvkoodi)
{
    return  (( alvkoodi / 100 == 0 || alvkoodi / 100 == 4 ) && alvkoodi % 20 / 10 == 0  ) ||  ( alvkoodi / 100 == 2 ) ;
}

void AlvLaskelma::AlvTaulu::lisaa(const QVariantMap &rivi)
{
    int koodi = rivi.value("alvkoodi").toInt();
    if( !koodit.contains(koodi))
        koodit.insert(koodi, KoodiTaulu());
    koodit[koodi].lisaa(rivi);
}

qlonglong AlvLaskelma::AlvTaulu::summa(int koodista, int koodiin)
{
    if( !koodiin)
        koodiin = koodista;

    qlonglong s = 0;
    QMapIterator<int,KoodiTaulu> iter(koodit);
    while( iter.hasNext()) {
        iter.next();
        if( iter.key() >= koodista && iter.key() <= koodiin )
            s += iter.value().summa( debetistaKoodilla( iter.key() ) );
    }
    return s;
}

void AlvLaskelma::KoodiTaulu::lisaa(const QVariantMap &rivi)
{
    int kanta = qRound( rivi.value("alvprosentti").toDouble() * 100 );
    if( !kannat.contains(kanta))
        kannat.insert(kanta, KantaTaulu());
    kannat[kanta].lisaa(rivi);
}

qlonglong AlvLaskelma::KoodiTaulu::summa(bool debetista) const
{
    qlonglong s = 0;
    QMapIterator<int,KantaTaulu> iter(kannat);
    while( iter.hasNext()) {
        iter.next();
        s += iter.value().summa(debetista);
    }
    return s;
}

void AlvLaskelma::KantaTaulu::lisaa(const QVariantMap &rivi)
{
    int tili = rivi.value("tili").toInt();
    if( !tilit.contains(tili))
        tilit.insert(tili, TiliTaulu());
    tilit[tili].lisaa(rivi);
}

qlonglong AlvLaskelma::KantaTaulu::summa(bool debetista) const
{
    qlonglong s = 0;
    QMapIterator<int,TiliTaulu> iter(tilit);
    while( iter.hasNext()) {
        iter.next();
        s += iter.value().summa(debetista);
    }
    return s;
}

void AlvLaskelma::TiliTaulu::lisaa(const QVariantMap &rivi)
{
    viennit.append(rivi);
}

qlonglong AlvLaskelma::TiliTaulu::summa(bool debetista) const
{
    qlonglong s = 0;
    for( auto vienti : viennit ) {
        if( debetista ) {
            s += qRound( vienti.value("debet").toDouble() * 100.0 );
            s -= qRound( vienti.value("kredit").toDouble() * 100.0);
        } else {
            s -= qRound( vienti.value("debet").toDouble() * 100.0 );
            s += qRound( vienti.value("kredit").toDouble() * 100.0);
        }
    }
    return s;
}
