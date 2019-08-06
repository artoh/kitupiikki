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
#include "asiakastoimittajavalinta.h"
#include "asiakastoimittajalistamodel.h"

#include "asiakastoimittajadlg.h"

#include "validator/ytunnusvalidator.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCompleter>
#include <QComboBox>
#include <QCompleter>

#include <QDebug>
#include <QKeyEvent>

AsiakasToimittajaValinta::AsiakasToimittajaValinta(QWidget *parent) :
    QWidget(parent),
    combo_( new QComboBox(this)),
    button_(new QPushButton(this)),
    model_( new AsiakasToimittajaListaModel(this)),
    dlg_( new AsiakasToimittajaDlg(this))

{
    button_->setIcon(QIcon(":/pic/uusiasiakas.png"));

    QHBoxLayout* leiska = new QHBoxLayout(this);
    leiska->setContentsMargins(0,0,0,0);
    leiska->addWidget( combo_ );
    leiska->addWidget(button_);
    leiska->setStretchFactor(combo_, 9);
    setLayout(leiska);

    combo_->setEditable(true);
    combo_->setModel( model_ );
    combo_->completer()->setCompletionMode(QCompleter::PopupCompletion);

    connect( button_, &QPushButton::clicked, this, &AsiakasToimittajaValinta::muokkaa);
    connect( model_, &AsiakasToimittajaListaModel::modelReset, this, &AsiakasToimittajaValinta::modelLadattu);


    connect(combo_, QOverload<int>::of(&QComboBox::activated), this, &AsiakasToimittajaValinta::valitseAsiakas);
    connect( combo_->lineEdit(), &QLineEdit::editingFinished, this, &AsiakasToimittajaValinta::syotettyNimi);

    connect( combo_, &QComboBox::currentTextChanged, this, &AsiakasToimittajaValinta::nimiMuuttui );

    connect( dlg_, &AsiakasToimittajaDlg::tallennettu, this, &AsiakasToimittajaValinta::talletettu);

    combo_->lineEdit()->setPlaceholderText(tr("Y-tunnus tai nimi"));
    combo_->installEventFilter(this);
}

QString AsiakasToimittajaValinta::nimi() const
{
    return combo_->currentText();
}

void AsiakasToimittajaValinta::set(int id, const QString &nimi)
{
    combo_->setCurrentText(nimi);
    ladattu_ = id;
    modelLadattu();
}


void AsiakasToimittajaValinta::clear()
{
    set(0);

}

void AsiakasToimittajaValinta::alusta(bool toimittaja)
{

    toimittaja_ = toimittaja;
    if( isEnabled() )
        model_->lataa(toimittaja);
}

void AsiakasToimittajaValinta::valitseAsiakas()
{
    setId( combo_->currentData().toInt() );
}

void AsiakasToimittajaValinta::nimiMuuttui()
{
    setId( combo_->itemData( combo_->findData( combo_->currentText(), Qt::EditRole ) ).toInt() );

    // Jos on syötetty y-tunnus, haetaan sillä
    if( YTunnusValidator::kelpaako( combo_->currentText() )) {
        QString alvtunnari = "FI" + combo_->currentText();
        int hakuId = model_->idAlvTunnuksella( alvtunnari.remove('-'));
        if( hakuId )
            combo_->setCurrentIndex( combo_->findData(hakuId) );
        else
            dlg_->ytunnuksella( combo_->currentText(), toimittaja_ );
    }
}

void AsiakasToimittajaValinta::syotettyNimi()
{
    if( !id_ && !combo_->currentText().isEmpty() && !YTunnusValidator::kelpaako( combo_->currentText() ))
        muokkaa();
}

void AsiakasToimittajaValinta::muokkaa()
{

    if( id_ )
        dlg_->muokkaa(id_, toimittaja_);
    else
        dlg_->uusi( combo_->currentText() , toimittaja_);

}

void AsiakasToimittajaValinta::talletettu(int id, const QString& nimi)
{
    ladattu_ = id;
    model_->lataa( toimittaja_ );

}

void AsiakasToimittajaValinta::modelLadattu()
{
    if( ladattu_ )
    {
        setId(ladattu_);
        int indeksi = combo_->findData( id_ );
        combo_->setCurrentIndex(indeksi);
    }
}

void AsiakasToimittajaValinta::setId(int id)
{
    id_ = id;
    if( id )
        button_->setIcon(QIcon(":/pic/muokkaaasiakas.png"));
    else
        button_->setIcon(QIcon(":/pic/uusiasiakas.png"));
}

bool AsiakasToimittajaValinta::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == combo_ && event->type()==QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        qDebug() << keyEvent->key();
        if( keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Return) {
            syotettyNimi();
            qDebug() << " EEVEE";
        }
    }
    return QWidget::eventFilter(watched, event);
}

