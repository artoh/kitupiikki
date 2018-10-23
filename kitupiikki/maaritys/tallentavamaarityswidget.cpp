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
#include "tallentavamaarityswidget.h"
#include "db/kirjanpito.h"

#include <QLineEdit>
#include <QCheckBox>

TallentavaMaaritysWidget::TallentavaMaaritysWidget(QWidget *parent)
    : MaaritysWidget (parent)
{

}

bool TallentavaMaaritysWidget::nollaa()
{
    for(QWidget *widget : widgetit_)
    {
        QString asetustunnus = widget->property("AsetusTunnus").toString();
        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit )
        {
            edit->setText( kp()->asetukset()->asetus(asetustunnus) );
            continue;
        }
        QCheckBox *box = qobject_cast<QCheckBox*>(widget);
        if( box )
        {
            box->setChecked( kp()->asetukset()->onko(asetustunnus));
            continue;
        }
    }
    return true;
}

bool TallentavaMaaritysWidget::tallenna()
{
    for(QWidget *widget : widgetit_)
    {
        QString asetustunnus = widget->property("AsetusTunnus").toString();
        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit )
        {
            kp()->asetukset()->aseta(asetustunnus, edit->text());
            continue;
        }

        QCheckBox *box = qobject_cast<QCheckBox*>(widget);
        if( box )
            kp()->asetukset()->aseta(asetustunnus, box->isChecked());
    }
    return true;
}

bool TallentavaMaaritysWidget::onkoMuokattu()
{
    for(QWidget *widget : widgetit_)
    {
        QString asetustunnus = widget->property("AsetusTunnus").toString();
        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit )
            if( kp()->asetukset()->asetus(asetustunnus) != edit->text())
                return true;

        QCheckBox *box = qobject_cast<QCheckBox*>(widget);
        if( box )
            if( kp()->asetukset()->onko(asetustunnus) != box->isChecked())
                return true;

    }
    return false;
}

void TallentavaMaaritysWidget::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

void TallentavaMaaritysWidget::rekisteroi(QWidget *widget, const QString &asetustunnus)
{
    widget->setProperty("AsetusTunnus", asetustunnus);
    widgetit_.append(widget);

    QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
    if( edit )
        connect(edit, &QLineEdit::textChanged, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);

    QCheckBox *box = qobject_cast<QCheckBox*>(widget);
    if( box )
        connect(box, &QCheckBox::stateChanged, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);

}
