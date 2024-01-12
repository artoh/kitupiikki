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

#include "ui_laskuntoimittaja.h"

#include "laskuntoimittaja.h"
#include "db/kirjanpito.h"

#include "sahkopostitoimittaja.h"
#include "finvoicetoimittaja.h"
#include "pdftoimittaja.h"
#include "../tulostus/laskuntulostaja.h"
#include "liite/liitteetmodel.h"

#include "db/tositetyyppimodel.h"

#include <QPrintDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>

#include <QMessageBox>

LaskunToimittaja::LaskunToimittaja(QWidget *parent) : QWidget(parent), ui( new Ui::LaskunToimittaja)
{

    ui->setupUi(this);
    hide();
    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    rekisteroiToimittaja(Lasku::SAHKOPOSTI, new SahkopostiToimittaja(this));
    rekisteroiToimittaja(Lasku::VERKKOLASKU, new FinvoiceToimittaja(this));
    rekisteroiToimittaja(Lasku::PDF, new PdfToimittaja(this));

    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &LaskunToimittaja::tyhjennaJono);
}


LaskunToimittaja *LaskunToimittaja::luoInstanssi(QWidget *parent)
{
    instanssi__ = new LaskunToimittaja(parent);
    return instanssi__;
}

void LaskunToimittaja::toimita(const QList<int> laskuTositeIdt)
{
    instanssi__->toimitaLasku(laskuTositeIdt);
}

void LaskunToimittaja::toimitaLasku(const QList<int> laskuTositeIdt)
{
    for(int id : laskuTositeIdt) {
        if( !toimitettavaIdt_.contains(id)) {
            toimitettavaIdt_.enqueue( id );
        }
    }
    ui->progressBar->setMaximum( ui->progressBar->maximum() + laskuTositeIdt.count() );

    setStyleSheet("background-color: rgba(230, 230, 230, 125); color: black;");

    QWidget* pw = qobject_cast<QWidget*>(parent());

    move( ( pw->width() - width()) / 2 ,
            pw->height() -height());
    show();

    if( !kaynnissa_ ) {
        kaynnissa_ = true;
        toimitaSeuraava();
    }
}


void LaskunToimittaja::laskuKasitelty()
{
    if( toimitettavaIdt_.isEmpty()) {
        // Kaikki toimitettu, näytetään tulos

        hide();
        emit kp()->kirjanpitoaMuokattu();

        if( onnistuneet_ && !epaonnistuneet_) {
            if( onnistuneet_ == 1) {
                emit kp()->onni(tr("Lasku toimitettu"), Kirjanpito::Onnistui);
            } else {
                emit kp()->onni(tr("%1 laskua toimitettu").arg(onnistuneet_), Kirjanpito::Onnistui);
            }
        } else if( epaonnistuneet_) {
            QStringList virhelista = virheet_.values();
            if( onnistuneet_ > 0) {
                QMessageBox::critical(nullptr,
                                      tr("Laskujen toimittaminen epäonnistui"),
                                      tr("%1 laskua toimitettu\n%2 laskun toimittaminen epäonnistui")
                                        .arg(onnistuneet_)
                                        .arg(epaonnistuneet_)
                                      + "\n" +
                                      tr("Toimittamatta jääneet laskut löytyvät Lähetettävät-välilehdeltä.") +
                                      QString("\n")
                             + virhelista.join("\n")) ;
            } else {
                QMessageBox::critical(nullptr,
                                      tr("Laskujen toimittaminen epäonnistui"),
                                      tr("Toimittamatta jääneet laskut löytyvät Lähetettävät-välilehdeltä.") +
                                      QString("\n")
                             + virhelista.join("\n")) ;
            }
        }

        ui->progressBar->setValue(-1);
        ui->progressBar->setMaximum(0);
        onnistuneet_ = 0;
        epaonnistuneet_ = 0;
        virheet_.clear();
        kaynnissa_ = false;

        if( painter_) {
            // Tulostuksen lopputoimet
            painter_->end();
            kp()->printer()->setPageLayout(vanhaleiska_);
            delete painter_;
            painter_ = nullptr;
        }

    } else {
        ui->progressBar->setValue( onnistuneet_ + epaonnistuneet_);
        toimitaSeuraava();
    }
}

void LaskunToimittaja::toimitaSeuraava()
{
    int tositeId = toimitettavaIdt_.dequeue();
    Tosite* tosite = new Tosite(this);

    connect( tosite, &Tosite::ladattu, this, &LaskunToimittaja::tositeLadattu);
    connect( tosite, &Tosite::latausvirhe, this, [this] { this->virhe(tr("Tositteen lataus epäonnistui")); });

    tosite->lataa(tositeId);
}

void LaskunToimittaja::tositeLadattu()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    tosite->disconnect();

    if( tosite->tyyppi() < TositeTyyppi::MYYNTILASKU || tosite->tyyppi() > TositeTyyppi::MAKSUMUISTUTUS) {
        virhe(tr("Ei ole myyntilasku #%1"));
        qCritical() << "Tosite# " << tosite->id() << " Tyyppi " << tosite->tyyppi();
        return;
    } else if( tosite->tila() == Tosite::POISTETTU) {
        virhe(tr("Lasku on poistettu"));
        qCritical() << "Tosite# " << tosite->id() << " on poistettu ";
        return;
    } else if( tosite->pvm() < kp()->tilitpaatetty()) {
        virhe(tr("Lukitulla tilikaudella"));
        qCritical() << "Tosite# " << tosite->id() << " lukitulla tilikaudella";
        return;
    }

    connect( tosite, &Tosite::laskuTallennettu, this, &LaskunToimittaja::laskuTallennettu);
    connect( tosite, &Tosite::tallennusvirhe, this, [this] { this->virhe(tr("Tallennusvirhe"));} );

    tosite->tallennaLasku(Tosite::LAHETETAAN);
}

void LaskunToimittaja::laskuTallennettu()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    tosite->disconnect();

    int toimitustapa = tosite->lasku().lahetystapa();
    if( toimitustapa == Lasku::POSTITUS && kp()->asetukset()->onko("MaventaPostitus")) {
        toimitustapa = Lasku::VERKKOLASKU;  // Postitus Maventan kautta
    }

    if( toimitustapa == Lasku::EITULOSTETA) {
        merkkaa(tosite->id());
        tosite->deleteLater();
    } else if( toimittajat_.contains(toimitustapa)) {
        AbstraktiToimittaja* toimittaja = toimittajat_.value(toimitustapa);
        toimittaja->toimitaLasku(tosite);
    } else {
        // Tulostetaan
        tulosta(tosite);
        tosite->deleteLater();
    }
}

void LaskunToimittaja::tulosta(Tosite *tosite)
{
    // Tulostus integroitu laskuntoimittajaan jotta
    // onnistuu yhdellä dialogilla
    const QString peruutusviesti = tr("Tulostaminen peruttiin");
    if( virheet_.contains(peruutusviesti)) {
        virhe(peruutusviesti);
        return;
    }

    if( !painter_ ) {
        vanhaleiska_ = kp()->printer()->pageLayout();
        QPageLayout uusileiska = vanhaleiska_;
        uusileiska.setUnits(QPageLayout::Millimeter);
        uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
        kp()->printer()->setPageLayout(uusileiska);

        QPrintDialog printDialog( kp()->printer() );
        if( printDialog.exec()) {
            painter_ = new QPainter( kp()->printer() );
        } else {
            virhe(peruutusviesti);
            return;
        }
    } else {
        kp()->printer()->newPage();
    }
    LaskunTulostaja tulostaja(kp());
    tulostaja.tulosta(*tosite, kp()->printer(), painter_);
    merkkaa( tosite->id());
}

void LaskunToimittaja::merkkaa(const int tositeId)
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::PATCH);
    QVariantMap map;
    map.insert("tila", Tosite::LAHETETTYLASKU);
    connect( kysely, &KpKysely::vastaus, this, &LaskunToimittaja::merkattu, Qt::QueuedConnection);
    connect( kysely, &KpKysely::virhe, this, [this] { this->virhe(tr("Tositteen päivittäminen epäonnistui")); });
    kysely->kysy(map);

}

void LaskunToimittaja::merkattu()
{
    onnistuneet_++;
    laskuKasitelty();
}


void LaskunToimittaja::virhe(const QString virhe)
{
    virheet_.insert(virhe);
    epaonnistuneet_++;
    laskuKasitelty();
}


void LaskunToimittaja::rekisteroiToimittaja(int tyyppi, AbstraktiToimittaja *toimittaja)
{
    toimittajat_.insert(tyyppi, toimittaja);
    connect( toimittaja, &AbstraktiToimittaja::onnistui, this, &LaskunToimittaja::merkkaa, Qt::QueuedConnection);
    connect( toimittaja, &AbstraktiToimittaja::onnistuiMerkitty, this, &LaskunToimittaja::merkattu, Qt::QueuedConnection);
    connect( toimittaja, &AbstraktiToimittaja::epaonnistui, this, &LaskunToimittaja::virhe);
}

void LaskunToimittaja::tyhjennaJono()
{
    toimitettavaIdt_.clear();
    kaynnissa_ = false;
}



LaskunToimittaja* LaskunToimittaja::instanssi__ = nullptr;
