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

namespace Tuonti {



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
    int nykyinen = index.data(Qt::EditRole).toInt();

    comboon(combo, CsvTuonti::EITUODA);


    if(  tyyppi >= CsvTuonti::SUOMIPVM)
        comboon(combo, CsvTuonti::PAIVAMAARA);
    else if( tuokirjauksia_ && ( tyyppi == CsvTuonti::LUKU || tyyppi == CsvTuonti::LUKUTEKSTI))
    {
        comboon(combo, CsvTuonti::TILINUMERO);
        comboon(combo, CsvTuonti::ALVPROSENTTI);
        comboon(combo, CsvTuonti::ALVKOODI);
        comboon(combo, CsvTuonti::DEBETTILI);
        comboon(combo, CsvTuonti::KREDITTILI);
    }
    else if( tuokirjauksia_ &&  tyyppi == CsvTuonti::RAHA)
    {
        comboon(combo, CsvTuonti::DEBETEURO);
        comboon(combo, CsvTuonti::KREDITEURO);
        comboon(combo, CsvTuonti::RAHAMAARA);
        comboon(combo, CsvTuonti::BRUTTOALVP);
    }
    else if( tyyppi == CsvTuonti::RAHA || tyyppi == CsvTuonti::LUKU)
    {
        comboon(combo, CsvTuonti::RAHAMAARA);
    }
    else if( !tuokirjauksia_ && ( tyyppi == CsvTuonti::VIITE || nykyinen == CsvTuonti::VIITE))
        comboon(combo, CsvTuonti::VIITENRO);
    else if( !tuokirjauksia_ && tyyppi == CsvTuonti::TILI )
        comboon(combo, CsvTuonti::IBAN);
    if( tyyppi == CsvTuonti::LUKU && !tuokirjauksia_)
        comboon(combo, CsvTuonti::KTOKOODI);
    if( tyyppi == CsvTuonti::TEKSTI || tyyppi == CsvTuonti::LUKUTEKSTI
            || tyyppi == CsvTuonti::LUKU)
    {
        comboon(combo, CsvTuonti::SELITE);
        if( !tuokirjauksia_) {
            comboon(combo, CsvTuonti::SAAJAMAKSAJA);
            comboon(combo, CsvTuonti::ARKISTOTUNNUS);
        } else {
            comboon(combo, CsvTuonti::TOSITETUNNUS);
            if( tyyppi == CsvTuonti::TEKSTI)
            {
                comboon(combo, CsvTuonti::KOHDENNUS);
                comboon(combo, CsvTuonti::TILINIMI);
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

void TuontiSarakeDelegaatti::comboon(QComboBox *combo, int tyyppi)
{
    combo->addItem( CsvTuonti::tuontiTeksti(tyyppi), tyyppi );
}

}
