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

}

KiertoWidget::~KiertoWidget()
{
    delete ui;
}

void KiertoWidget::lataaTosite()
{
    if( !kp()->yhteysModel() || !tosite_)
        return;


    int ntila = tosite_ ? tosite_->tositetila() : 0;
    ui->ibanLabel->setText( MyyntiLaskunTulostaja::valeilla( tosite_->data(Tosite::PORTAALI).toMap().value("iban").toString() ) );

    ui->polkuCombo->setCurrentIndex( tosite_->kierto() ? ui->polkuCombo->findData( tosite_->kierto()  ) : 0 );
    ui->siirraNappi->hide();
    ui->polkuCombo->setEnabled( kp()->yhteysModel()->onkoOikeutta( YhteysModel::KIERTO_LISAAMINEN | YhteysModel::KIERTO_TARKASTAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN ));

    ui->vCheck->hide();
    ui->vAloita->setVisible( kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_LISAAMINEN) &&
                             (ntila == Tosite::LUONNOS || ntila < Tosite::SAAPUNUT));

    ui->tInfo->hide();
    ui->tCheck->hide();
    ui->tHylatty->hide();
    ui->tOk->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_TARKASTAMINEN) &&
                        (ntila == Tosite::LUONNOS || ntila < Tosite::TARKASTETTU));
    ui->tHylkaa->setVisible(kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_TARKASTAMINEN) &&
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
                    info.append(tr("p. %1 \n").arg(portaali.value("puhelin").toString()));
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

void KiertoWidget::valmis(int tilaan)
{
    tosite_->asetaKierto( ui->polkuCombo->currentData().toInt() );
    emit tallenna(tilaan);
}
