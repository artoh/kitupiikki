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
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include "tools/tilicombo.h"

#include "db/kielikentta.h"
#include <QJsonDocument>
#include <QVariant>

#include <QDebug>

TallentavaMaaritysWidget::TallentavaMaaritysWidget(const QString &ohjesivu, QWidget *parent)
    : MaaritysWidget (parent), ohjesivu_(ohjesivu)
{

}

bool TallentavaMaaritysWidget::nollaa()
{
    for(QWidget *widget : findChildren<QWidget*>() ) {
        QString asetusavain = widget->property("Asetus").toString();

        if( asetusavain.isEmpty())
            continue;

        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit ) {
            edit->setText( kp()->asetukset()->asetus(asetusavain) );
            connect( edit, &QLineEdit::textEdited, this, &TallentavaMaaritysWidget::ilmoitaMuokattu );
            continue;
        }

        QAbstractButton *box = qobject_cast<QAbstractButton*>(widget);
        if( box ) {
            box->setChecked( kp()->asetukset()->onko(asetusavain) );
            connect( box, &QAbstractButton::clicked, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);
        }

        TiliCombo *tcombo = qobject_cast<TiliCombo*>(widget);
        if( tcombo ) {
            QString tilisuodatin = widget->property("Tilit").toString();
            tcombo->suodataTyypilla(tilisuodatin);
            tcombo->valitseTili( kp()->asetukset()->luku(asetusavain) );
            connect( tcombo, &TiliCombo::tiliValittu, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);
            continue;
        }

        QComboBox *combo = qobject_cast<QComboBox*>(widget);
        if( combo ) {

            // Haetaan combon vaihtoehdot
            QString vaihtoehdot = widget->property("Vaihtoehdot").toString();
            if( !vaihtoehdot.isEmpty() ) {
                bool liput = widget->property("Liput").isValid();
                combo->clear();
                QVariantMap vemap = QJsonDocument::fromJson( kp()->asetukset()->asetus(vaihtoehdot).toUtf8() ).toVariant().toMap();
                QMapIterator<QString,QVariant> viter(vemap);
                while( viter.hasNext()) {
                    viter.next();
                    KieliKentta kielet(viter.value());
                    if(liput)
                        combo->addItem(QIcon(":/liput/" + viter.key() + ".png"), kielet.teksti(), viter.key() );
                    else
                        combo->addItem(kielet.teksti(), viter.key());
                }
            }

            combo->setCurrentIndex( combo->findData( kp()->asetukset()->asetus(asetusavain) ) );
            connect( combo, &QComboBox::currentTextChanged, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);
            continue;
        }

        QSpinBox *spin = qobject_cast<QSpinBox*>(widget);
        if( spin ) {
            spin->setValue( kp()->asetukset()->luku(asetusavain) );
            connect( spin, SIGNAL(valueChanged(int)), this, SLOT(ilmoitaMuokattu()));
            continue;
        }

        QDoubleSpinBox *dspin = qobject_cast<QDoubleSpinBox*>(widget);
        if( dspin ) {
            dspin->setValue( kp()->asetukset()->asetus(asetusavain).toDouble() );
            connect( dspin, SIGNAL(valueChanged(double)), this, SLOT(ilmoitaMuokattu()));
            continue;
        }

        QTextEdit *tedit = qobject_cast<QTextEdit*>(widget);
        if( tedit ) {
            tedit->setPlainText( kp()->asetukset()->asetus(asetusavain) );
            connect( tedit, &QTextEdit::textChanged, this, &TallentavaMaaritysWidget::ilmoitaMuokattu);
            continue;
        }

    }
    alustettu_ = true;

    return true;
}

bool TallentavaMaaritysWidget::tallenna()
{
    QVariantMap asetukset;

    for(QWidget *widget : findChildren<QWidget*>() ) {
        QString asetusavain = widget->property("Asetus").toString();

        if( asetusavain.isEmpty())
            continue;

        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit ) {
            asetukset.insert(asetusavain, edit->text());
            continue;
        }

        QAbstractButton *box = qobject_cast<QAbstractButton*>(widget);
        if( box ) {
            if( box->isChecked())
                asetukset.insert(asetusavain, "ON");
            else
                asetukset.insert(asetusavain, QVariant());
            continue;
        }

        TiliCombo *tcombo = qobject_cast<TiliCombo*>(widget);
        if( tcombo ) {
            asetukset.insert(asetusavain, tcombo->valittuTilinumero());
            continue;
        }

        QComboBox *combo = qobject_cast<QComboBox*>(widget);
        if( combo ) {
            asetukset.insert(asetusavain, combo->currentData());
            continue;
        }

        QSpinBox *spin = qobject_cast<QSpinBox*>(widget);
        if( spin ) {
            asetukset.insert(asetusavain, spin->value());
            continue;
        }

        QTextEdit *tedit = qobject_cast<QTextEdit*>(widget);
        if( tedit ) {
            asetukset.insert( asetusavain, tedit->toPlainText());
            continue;
        }

        QDoubleSpinBox *dspin = qobject_cast<QDoubleSpinBox*>(widget);
        if( dspin ) {
            asetukset.insert(asetusavain, QString::number( dspin->value(),'f',2 ));
            continue;
        }

    }
    kp()->asetukset()->aseta(asetukset);
    emit tallennaKaytossa(false);
    emit kp()->onni("Asetukset tallennettu");
    return true;
}

bool TallentavaMaaritysWidget::onkoMuokattu()
{
    if( !alustettu_)
        return false;


    for(QWidget *widget : findChildren<QWidget*>() ) {
        QString asetusavain = widget->property("Asetus").toString();

        if( asetusavain.isEmpty())
            continue;


        QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
        if( edit ) {
            if( kp()->asetukset()->asetus(asetusavain) != edit->text())
                return true;
            continue;
        }

        QAbstractButton *box = qobject_cast<QAbstractButton*>(widget);
        if( box ) {
            if( kp()->asetukset()->onko(asetusavain) != box->isChecked()  )
                return true;
            continue;
        }

        TiliCombo *tcombo = qobject_cast<TiliCombo*>(widget);
        if( tcombo ) {
            if( tcombo->valittuTilinumero() != kp()->asetukset()->luku(asetusavain))
                return true;
            continue;
        }

        QComboBox *combo = qobject_cast<QComboBox*>(widget);
        if( combo ) {
            if( kp()->asetukset()->asetus(asetusavain) != combo->currentData().toString())
                return true;
            continue;
        }

        QSpinBox *spin = qobject_cast<QSpinBox*>(widget);
        if( spin ) {
            if( kp()->asetukset()->luku(asetusavain) != spin->value() )
                return true;
            continue;
        }

        QTextEdit *tedit = qobject_cast<QTextEdit*>(widget);
        if( tedit ) {
            if( kp()->asetukset()->asetus(asetusavain) != tedit->toPlainText() )
                return true;
            continue;
        }

        QDoubleSpinBox *dspin = qobject_cast<QDoubleSpinBox*>(widget);
        if( dspin ) {
            if( qAbs(kp()->asetus(asetusavain).toDouble() - dspin->value() ) > 1e-3 )
                return true;
            continue;
        }

    }    
    return false;
}

void TallentavaMaaritysWidget::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

