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

#include "ui_onniwidget.h"

#include "laskuntoimittaja.h"
#include "db/kirjanpito.h"
#include "../tulostus/laskuntulostaja.h"

#include "eitulostetatoimittaja.h"
#include "tulostustoimittaja.h"
#include "sahkopostitoimittaja.h"
#include "finvoicetoimittaja.h"
#include "pdftoimittaja.h"

#include <QMessageBox>

LaskunToimittaja::LaskunToimittaja(QWidget *parent) : QWidget(parent)
{
    ui = new Ui::onniWidget;
    ui->setupUi(this);
    hide();
    setStyleSheet("background-color: rgba(255, 255, 0, 125)");
    ui->kuvaLabel->setPixmap(QPixmap(":/pic/lasku.png"));
    ui->viestiLabel->setText(tr("Laskuja toimitetaan ... "));
    QFont font;
    font.setPointSize( font.pointSize() * 3 / 2);
    ui->viestiLabel->setFont(font);
    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    rekisteroiToimittaja(Lasku::TULOSTETTAVA, new TulostusToimittaja(this));
    rekisteroiToimittaja(Lasku::EITULOSTETA, new EiTulostetaToimittaja(this));
    rekisteroiToimittaja(Lasku::SAHKOPOSTI, new SahkopostiToimittaja(this));
    rekisteroiToimittaja(Lasku::VERKKOLASKU, new FinvoiceToimittaja(this));
    rekisteroiToimittaja(Lasku::PDF, new PdfToimittaja(this));
}


LaskunToimittaja *LaskunToimittaja::luoInstanssi(QWidget *parent)
{
    instanssi__ = new LaskunToimittaja(parent);
    return instanssi__;
}

void LaskunToimittaja::toimita(const QVariantMap &tosite)
{
    instanssi__->toimitaLasku(tosite);
}

void LaskunToimittaja::toimita(const int id)
{
    instanssi__->toimitaLasku(id);
}

void LaskunToimittaja::peru()
{
    hide();
    emit kp()->onni(tr("Laskujen toimittaminen peruttiin"), Kirjanpito::Stop);
    tositteet_.clear();
    haettavat_.clear();

}

void LaskunToimittaja::toimitaLasku(const QVariantMap &tosite)
{
    tositteet_.enqueue(tosite);
    silmukka();
}

void LaskunToimittaja::toimitaLasku(const int tositeid)
{
    haettavat_.enqueue(tositeid);
    silmukka();
}

void LaskunToimittaja::haeLasku()
{
    noutoKaynnissa_ = true;
    KpKysely* kysely = kpk(QString("/tositteet/%1").arg(haettavat_.dequeue()));
    connect( kysely, &KpKysely::vastaus, this, &LaskunToimittaja::laskuSaapuu, Qt::QueuedConnection );
    kysely->kysy();
}

void LaskunToimittaja::laskuSaapuu(QVariant *data)
{
    tositteet_.enqueue(data->toMap());
    noutoKaynnissa_ = false;
    silmukka();
}

void LaskunToimittaja::tallennaLiite()
{    
    tallennusTosite_ = new Tosite(this);
    tallennusTosite_->lataa(tositteet_.head());
    connect( tallennusTosite_, &Tosite::laskuTallennettu, this, &LaskunToimittaja::liiteTallennettu, Qt::QueuedConnection);
    tallennusTosite_->tallennaLasku(Tosite::LAHETETAAN);
}

void LaskunToimittaja::liiteTallennettu()
{
    int toimitustapa = tositteet_.head().value("lasku").toMap().value("laskutapa").toInt();
    if(toimitustapa == Lasku::POSTITUS && kp()->asetukset()->onko("MaventaPostitus")) {
        toimitustapa = Lasku::VERKKOLASKU;
    }

    AbstraktiToimittaja* toimittaja = toimittajat_.value(toimitustapa, toimittajat_.value(Lasku::TULOSTETTAVA));
    toimittaja->lisaaLasku(tallennusTosite_->tallennettava());
    tositteet_.dequeue();

    tallennusTosite_->deleteLater();
    tallennusTosite_ = nullptr;
    silmukka();
}

void LaskunToimittaja::silmukka()
{
    if( !noutoKaynnissa_ && !haettavat_.isEmpty())
        haeLasku();
    if( !tallennusTosite_ && !tositteet_.isEmpty())
        tallennaLiite();

    show();
    QWidget* wg = qobject_cast<QWidget*>(parent());
    move( wg->width() / 2 - width() / 2, wg->height() - height() * 5 / 3  );
}

void LaskunToimittaja::onnistui()
{
    onnistuneet_++;
    tarkastaValmis();
}

void LaskunToimittaja::virhe(const QString virhe)
{
    virheet_.insert(virhe);
    epaonnistuneet_++;
    tarkastaValmis();
}

void LaskunToimittaja::tarkastaValmis()
{
    bool kaynnissa = tallennusTosite_ || noutoKaynnissa_;
    for(const auto& toimittaja : toimittajat_) {
        kaynnissa |= !toimittaja->vapaa();
    }

    if( !kaynnissa) {
        hide();
        emit kp()->kirjanpitoaMuokattu();

        if( onnistuneet_ && !epaonnistuneet_) {
            if( onnistuneet_ == 1) {
                emit kp()->onni(tr("Lasku toimitettu"), Kirjanpito::Onnistui);
            } else {
                emit kp()->onni(tr("%1 laskua toimitettu").arg(onnistuneet_), Kirjanpito::Onnistui);
            }
        } else if( epaonnistuneet_) {
            QStringList virhelista = virheet_.toList();
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
                                      .arg(onnistuneet_)
                                      .arg(epaonnistuneet_)
                             + virhelista.join("\n")) ;
            }
        }

        onnistuneet_ = 0;
        epaonnistuneet_ = 0;
        virheet_.clear();
    }

}

void LaskunToimittaja::rekisteroiToimittaja(int tyyppi, AbstraktiToimittaja *toimittaja)
{
    toimittajat_.insert(tyyppi, toimittaja);
    connect( toimittaja, &AbstraktiToimittaja::toimitettu, this, &LaskunToimittaja::onnistui);
    connect( toimittaja, &AbstraktiToimittaja::epaonnistui, this, &LaskunToimittaja::virhe);
}



LaskunToimittaja* LaskunToimittaja::instanssi__ = nullptr;
