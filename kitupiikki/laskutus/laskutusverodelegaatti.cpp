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

#include "laskutusverodelegaatti.h"

#include <QComboBox>
#include "db/verotyyppimodel.h"
#include "laskumodel.h"
#include "db/kirjanpito.h"

LaskutusVeroDelegaatti::LaskutusVeroDelegaatti()
{

}

QWidget *LaskutusVeroDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new QComboBox(parent);

    cbox->addItem(QIcon(":/pic/0pros.png"),"0%", QVariant(AlvKoodi::ALV0));
    cbox->addItem(QIcon(":/pic/netto.png"),"10%", QVariant(AlvKoodi::MYYNNIT_NETTO + 10 * 100 ));
    cbox->addItem(QIcon(":/pic/netto.png"),"14%", QVariant(AlvKoodi::MYYNNIT_NETTO + 14 * 100));
    cbox->addItem(QIcon(":/pic/netto.png"),"24%", QVariant(AlvKoodi::MYYNNIT_NETTO + 24 * 100 ));
    cbox->addItem(QIcon(":/pic/tyhja.png"),"Veroton myynti", QVariant(AlvKoodi::EIALV));
    cbox->addItem(QIcon(":/pic/vasara.png"), tr("Rakennuspalvelut"), QVariant( AlvKoodi::RAKENNUSPALVELU_MYYNTI ));
    cbox->addItem(QIcon(":/pic/eu.png"), tr("Tavaramyynti"), QVariant( AlvKoodi::YHTEISOMYYNTI_TAVARAT ));
    cbox->addItem(QIcon(":/pic/eu.png"), tr("Palvelumyynti"), QVariant( AlvKoodi::YHTEISOMYYNTI_PALVELUT ));

    if( !kp()->onkoMaksuperusteinenAlv(kp()->paivamaara()))
    {
        cbox->addItem(QIcon(":/pic/marginaali.png"),"Voittomarginaalimenettely - käytetyt tavarat", QVariant(LaskuModel::Kaytetyt));
        cbox->addItem(QIcon(":/pic/marginaali.png"),"Voittomarginaalimenettely - taide-esineet", QVariant(LaskuModel::Taide));
        cbox->addItem(QIcon(":/pic/marginaali.png"),"Voittomarginaalimenettely - keräily- ja antiikkiesineet", QVariant(LaskuModel::KerailyAntiikki));
    }

    return cbox;
}

void LaskutusVeroDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int koodi = index.data(LaskuModel::AlvProsenttiRooli).toInt() * 100 + index.data(LaskuModel::AlvKoodiRooli).toInt();
    if( index.data(LaskuModel::VoittomarginaaliRooli).toInt())
        koodi = index.data(LaskuModel::VoittomarginaaliRooli).toInt();

    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    cbox->setCurrentIndex( cbox->findData(koodi) );
}

void LaskutusVeroDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    int koodi = cbox->currentData().toInt();

    if( koodi >= LaskuModel::Kaytetyt)
    {
        model->setData(index, koodi, LaskuModel::VoittomarginaaliRooli);
    }
    else
    {
        model->setData(index, koodi / 100, LaskuModel::AlvProsenttiRooli);
        model->setData(index, koodi % 100, LaskuModel::AlvKoodiRooli);
    }

}


