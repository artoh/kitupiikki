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
#include "kiertowidget.h"
#include "model/tosite.h"
#include "model/tositeloki.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "ui_kierto.h"
#include "kiertomodel.h"
#include "laskutus/myyntilaskuntulostaja.h"
#include "laskutus/nayukiQR/QrCode.hpp"
#include "db/tositetyyppimodel.h"
#include "model/tositeviennit.h"

#include <QScreen>
#include <QClipboard>
#include <QSvgWidget>

KiertoWidget::KiertoWidget(Tosite *tosite, QWidget *parent) : QWidget(parent),
    ui( new Ui::Kierto), tosite_(tosite)
{
    ui->setupUi(this);
    ui->polkuCombo->setModel(kp()->kierrot());

    connect( tosite, &Tosite::ladattu, this, &KiertoWidget::lataaTosite);    

    connect( ui->vAloita, &QPushButton::clicked, [this] { this->valmis(Tosite::SAAPUNUT); });

    connect( ui->tOk, &QPushButton::clicked, [this] { this->valmis(Tosite::TARKASTETTU); });
    connect( ui->tHylkaa, &QPushButton::clicked, [this] { this->valmis(Tosite::HYLATTY); });

    connect( ui->hOk, &QPushButton::clicked, [this] { this->valmis(Tosite::HYVAKSYTTY); });
    connect( ui->hHylkaa, &QPushButton::clicked, [this] { this->valmis(Tosite::HYLATTY); });

    connect( ui->polkuCombo, &QComboBox::currentTextChanged, [this] {
        this->ui->siirraNappi->setVisible( this->ui->polkuCombo->currentData().toInt() != this->tosite_->kierto()
                && this->tosite_->tositetila() >= Tosite::HYLATTY
                && this->tosite_->tositetila() <= Tosite::HYVAKSYTTY);
    });
    connect( ui->siirraNappi, &QPushButton::clicked, [this] { this->valmis( this->tosite_->tositetila() );});

    connect( ui->kopioiIban, &QToolButton::clicked, [this] { qApp->clipboard()->setText(iban_);});
    connect( ui->barCopyButton, &QToolButton::clicked, [this] { qApp->clipboard()->setText(this->kululaskuVirtuaalikoodi());});


    if( qApp->screens().value(0)->size().height() < 1200) {
        // Jotta käyttökelpoisempi pienellä näytöllä, poistetaan kuvakkeeita näkyviltä
        ui->kuva1->hide();
        ui->kuva2->hide();
        ui->kuva3->hide();
    }
}

KiertoWidget::~KiertoWidget()
{
    delete ui;
}

QString KiertoWidget::kululaskuVirtuaalikoodi() const
{
    if( iban_.isEmpty() || ( tosite_->tyyppi() != TositeTyyppi::KULULASKU && tosite_->viite().isEmpty()))
        return QString();

    QString viite = tosite_->viite().isEmpty() ? luotuviite() : tosite_->viite();
    if(viite.startsWith("RF"))
        return QString();   // Toistaiseksi ei tueta RF-viitteitä

    QString erapv = tosite_->erapvm().isValid() ? tosite_->erapvm().toString("yyMMdd") : QString("000000");

    qlonglong sntsumma = 0l;
    for(int i = 0; i < tosite_->viennit()->rowCount(); i++) {
        sntsumma += qRound64( tosite_->viennit()->vienti(i).kredit() * 100.0 );
    }

    QString koodi = QString("4 %1 %2 000 %3 %4")
            .arg( iban_.mid(2,16) )  // Tilinumeron numeerinen osuus
            .arg( sntsumma, 8, 10, QChar('0') )  // Rahamäärä
            .arg( viite, 20, QChar('0'))
            .arg( erapv );

    return koodi.remove(QChar(' '));
}

QByteArray KiertoWidget::kululaskuQr() const
{
    if( iban_.isEmpty() || ( tosite_->tyyppi() != TositeTyyppi::KULULASKU && tosite_->viite().isEmpty()))
        return QByteArray();

    QString viite = tosite_->viite().isEmpty() ? luotuviite() : tosite_->viite();
    qlonglong sntsumma = 0l;
    for(int i = 0; i < tosite_->viennit()->rowCount(); i++) {
        sntsumma += qRound64( tosite_->viennit()->vienti(i).kredit() * 100.0 );
    }

    QString data("BCD\n001\n1\nSCT\n");

    QString bic = MyyntiLaskunTulostaja::bicIbanilla(iban_);
    if( bic.isEmpty())
        return QByteArray();
    data.append(bic + "\n");
    data.append( tosite_->kumppaninimi() + "\n");
    data.append(iban_ + "\n");
    data.append( QString("EUR%1\n\n").arg( sntsumma / 100.0, 0, 'f', 2 ));
    data.append( viite + "\n\n");
    if( tosite_->erapvm().isValid())
        data.append( QString("ReqdExctnDt/%1").arg( tosite_->erapvm().toString(Qt::ISODate)));

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText( data.toUtf8().data() , qrcodegen::QrCode::Ecc::LOW);
    return QByteArray::fromStdString( qr.toSvgString(1) );
}

QString KiertoWidget::luotuviite() const
{
    QString numero = QString::number(tosite_->id() + 1000);

    int summa = 0;
    int indeksi = 0;
    for( int i = numero.length() - 1; i > -1; i--) {
        QChar ch = numero.at(i);
        int numero = ch.digitValue();

        if( indeksi % 3 == 0)
            summa += 7 * numero;
        else if( indeksi % 3 == 1)
            summa += 3 * numero;
        else
            summa += numero;

        indeksi++;
    }
    int tarkaste = ( 10 - summa % 10) % 10;
    return numero + QString::number(tarkaste);
}

void KiertoWidget::lataaTosite()
{
    if( !kp()->yhteysModel() || !tosite_)
        return;

    if( !tosite_->data(Tosite::PORTAALI).toMap().isEmpty()) {
        ui->kuva1->setPixmap(QPixmap(":/pixaby/naytto.png"));
    } else if( tosite_->tyyppi() == TositeTyyppi::SAAPUNUTVERKKOLASKU) {
        ui->kuva1->setPixmap(QPixmap(":/pic/e64.png"));
    } else {
        ui->kuva1->setPixmap(QPixmap(":/pixaby/skanneri.png"));
    }


    int ntila = tosite_ ? tosite_->tositetila() : 0;
    iban_ = tosite_->data(Tosite::PORTAALI).toMap().contains("iban") ?
                tosite_->data(Tosite::PORTAALI).toMap().value("iban").toString() :
                tosite_->lasku().iban();

    ui->ibanLabel->setText( MyyntiLaskunTulostaja::valeilla( iban_) );
    ui->ibanLabel->setVisible(!iban_.isEmpty());
    ui->kopioiIban->setVisible(!iban_.isEmpty());
    ui->barCopyButton->setVisible(!iban_.isEmpty() && (tosite_->tyyppi() == TositeTyyppi::KULULASKU || !tosite_->viite().isEmpty() ) );    
    paivitaViivakoodi();
    connect( tosite_, &Tosite::tilaTieto, this, &KiertoWidget::paivitaViivakoodi);

    ui->polkuCombo->setCurrentIndex( tosite_->kierto() ? ui->polkuCombo->findData( tosite_->kierto()  ) : 0 );
    ui->siirraNappi->hide();
    ui->polkuCombo->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::KIERTO_LISAAMINEN | YhteysModel::KIERTO_TARKASTAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN ));

    ui->vCheck->hide();
    ui->vInfo->clear();
    ui->vAloita->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_LISAAMINEN) &&
                             (ntila == Tosite::LUONNOS || ntila < Tosite::SAAPUNUT));

    ui->tInfo->hide();
    ui->tCheck->hide();
    ui->tHylatty->hide();
    ui->tOk->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_TARKASTAMINEN) &&
                        (ntila == Tosite::LUONNOS || ntila < Tosite::TARKASTETTU));
    ui->tHylkaa->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_TARKASTAMINEN) &&
                            !kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_HYVAKSYMINEN) &&
                            (ntila == Tosite::LUONNOS || ntila < Tosite::TARKASTETTU));

    ui->hInfo->hide();
    ui->hCheck->hide();
    ui->hHylatty->hide();
    ui->hOk->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_HYVAKSYMINEN) &&
                        (ntila == Tosite::LUONNOS || ntila < Tosite::HYVAKSYTTY));
    ui->hHylkaa->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_HYVAKSYMINEN) &&
                        (ntila == Tosite::LUONNOS || ntila < Tosite::HYVAKSYTTY));


    if( !tosite_)
        return;

    TositeLoki* loki = tosite_->loki();
    int edellinentila = 0;

    for(int i = loki->rowCount()-1; i >= 0; i--) {
        int tila = loki->index(i, TositeLoki::TILA).data(Qt::EditRole).toInt();
        if( tila == Tosite::SAAPUNUT) {
            ui->vCheck->show();
            ui->vInfo->show();
            QString info = "";
            QVariantMap portaali = tosite_->data(Tosite::PORTAALI).toMap();
            if( !portaali.isEmpty()) {
                QString info = QString("%1\n%2\n")
                        .arg(portaali.value("nimi").toString())
                        .arg(portaali.value("email").toString());
                if( !portaali.value("puhelin").toString().isEmpty())
                    info.append(tr("puh. %1 \n").arg(portaali.value("puhelin").toString()));
                info.append( loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")) );
                ui->vInfo->setText(info);
            } else {
            ui->vInfo->setText(loki->index(i, TositeLoki::KAYTTAJA).data().toString() + "\n" +
                        loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")));
            }


        } else if( tila == Tosite::TARKASTETTU) {
            ui->tCheck->show();
            ui->tHylatty->hide();
            ui->tInfo->show();
            ui->tInfo->setText( loki->index(i, TositeLoki::KAYTTAJA).data().toString() + "\n" +
                                loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")) );
        } else if( tila == Tosite::HYVAKSYTTY) {
            ui->hCheck->show();
            ui->hHylatty->hide();
            ui->hInfo->show();
            ui->hInfo->setText( loki->index(i, TositeLoki::KAYTTAJA).data().toString() + "\n" +
                                loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")) );
        }
        else if( tila == Tosite::HYLATTY) {
            if( edellinentila == Tosite::TARKASTETTU) {
                ui->hCheck->hide();
                ui->hHylatty->show();
                ui->hInfo->show();
                ui->hInfo->setText( loki->index(i, TositeLoki::KAYTTAJA).data().toString() + "\n" +
                                    loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")) );
            } else {
                ui->hCheck->hide();
                ui->hHylatty->hide();
                ui->hInfo->hide();

                ui->tCheck->hide();
                ui->tHylatty->show();

                ui->tInfo->setText( loki->index(i, TositeLoki::KAYTTAJA).data().toString() + "\n" +
                                    loki->index(i, TositeLoki::AIKA).data(Qt::DisplayRole).toDateTime().toString(tr("dd.MM.yyyy klo hh.mm")) );


            }
        }

        edellinentila = tila;
    }

}

void KiertoWidget::paivitaViivakoodi()
{
    if( iban_.isEmpty() || ( tosite_->tyyppi() != TositeTyyppi::KULULASKU && tosite_->viite().isEmpty())) {
        ui->viivakoodiLabel->hide();
        ui->svgWidget->hide();
    } else {
        QFont koodifontti( "code128_XL", 36);
        koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
        ui->viivakoodiLabel->setFont(koodifontti);
        ui->viivakoodiLabel->setText(MyyntiLaskunTulostaja::code128(kululaskuVirtuaalikoodi()));
        ui->viivakoodiLabel->show();

        ui->svgWidget->show();
        ui->svgWidget->load(kululaskuQr());
    }
}


void KiertoWidget::valmis(int tilaan)
{
    tosite_->asetaKierto( ui->polkuCombo->currentData().toInt() );
    emit tallenna(tilaan);
}
