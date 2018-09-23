/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include "tuontisarakedelegaatti.h"
#include "csvtuonti.h"

#include <QComboBox>

TuontiSarakeDelegaatti::TuontiSarakeDelegaatti(QObject *parent)
    : QItemDelegate(parent), tuokirjauksia_(true)
{

}

void TuontiSarakeDelegaatti::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int tuominen = index.data().toInt();
    QString tekstina = CsvTuonti::tuontiTeksti(tuominen);

    drawDisplay(painter, option, option.rect, tekstina);
    drawFocus(painter, option, option.rect);
}

QWidget *TuontiSarakeDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex &index) const
{
    QComboBox *combo = new QComboBox(parent);

    int tyyppi = index.data(CsvTuonti::TyyppiRooli).toInt();

    combo->addItem(tr("Ei tuoda"), CsvTuonti::EITUODA);

    if(  tyyppi >= CsvTuonti::SUOMIPVM)
        combo->addItem(tr("Päivämäärä"), CsvTuonti::PAIVAMAARA);
    else if( tuokirjauksia_ && ( tyyppi == CsvTuonti::LUKU || tyyppi == CsvTuonti::LUKUTEKSTI))
        combo->addItem(tr("Tilin numero"), CsvTuonti::TILINUMERO);
    else if( tuokirjauksia_ &&  tyyppi == CsvTuonti::RAHA)
    {
        combo->addItem(tr("Debet euroa"), CsvTuonti::DEBETEURO);
        combo->addItem(tr("Kredit euroa"), CsvTuonti::KREDITEURO);
        combo->addItem(tr("Määrä euroa"), CsvTuonti::RAHAMAARA);
    }
    else if( tyyppi == CsvTuonti::RAHA || tyyppi == CsvTuonti::LUKU)
    {
        combo->addItem(tr("Määrä euroa"), CsvTuonti::RAHAMAARA);
    }
    else if( !tuokirjauksia_ && tyyppi == CsvTuonti::VIITE)
        combo->addItem(tr("Viitenumero"), CsvTuonti::VIITENRO);
    else if( !tuokirjauksia_ && tyyppi == CsvTuonti::TILI )
        combo->addItem(tr("IBAN-tilinumero"), CsvTuonti::IBAN);

    if( tyyppi == CsvTuonti::TEKSTI || tyyppi == CsvTuonti::LUKUTEKSTI
            || tyyppi == CsvTuonti::LUKU)
    {
        combo->addItem("Selite", CsvTuonti::SELITE);
        if( !tuokirjauksia_)
            combo->addItem("Arkistotunnus", CsvTuonti::ARKISTOTUNNUS);
        else
        {
            combo->addItem("Tositteen tunnus", CsvTuonti::TOSITETUNNUS);
            if( tyyppi == CsvTuonti::TEKSTI)
            {
                combo->addItem("Kohdennus", CsvTuonti::KOHDENNUS);
                combo->addItem("Tilin nimi", CsvTuonti::TILINIMI);
            }
        }
    }
    return combo;
}

void TuontiSarakeDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    combo->setCurrentIndex( combo->findData( index.data() ) );
}

void TuontiSarakeDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    model->setData(index, combo->currentData());
}

void TuontiSarakeDelegaatti::asetaTyyppi(bool kirjauksia)
{
    tuokirjauksia_ = kirjauksia;
}

