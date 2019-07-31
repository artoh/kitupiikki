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
#include "asiakastoimittajataydentaja.h"

#include "asiakasdlg.h"
#include "toimittajadlg.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCompleter>

#include <QDebug>

AsiakasToimittajaValinta::AsiakasToimittajaValinta(QWidget *parent) :
    QWidget(parent),
    edit_(new QLineEdit(this)),
    button_(new QPushButton(this)),
    taydentajaModel_(new AsiakasToimittajaTaydentaja(this)),
    completer_(new QCompleter(this))

{
    button_->setIcon(QIcon(":/pic/yrittaja.png"));

    QHBoxLayout* leiska = new QHBoxLayout(this);
    leiska->setContentsMargins(0,0,0,0);
    leiska->addWidget(edit_);
    leiska->addWidget(button_);
    setLayout(leiska);

    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setModel(taydentajaModel_);
    edit_->setCompleter(completer_);

    connect( edit_, &QLineEdit::textEdited, this, &AsiakasToimittajaValinta::valitseAsiakas);
    connect( edit_, &QLineEdit::editingFinished, this, &AsiakasToimittajaValinta::valitseAsiakas);
    connect( button_, &QPushButton::clicked, this, &AsiakasToimittajaValinta::muokkaa);
}

QString AsiakasToimittajaValinta::nimi() const
{
    return edit_->text();
}

void AsiakasToimittajaValinta::set(int id, const QString &nimi, bool toimittaja)
{
    toimittaja_ = toimittaja;
    id_ = id;
    edit_->setText(nimi);
}

void AsiakasToimittajaValinta::clear()
{
    id_ = 0;
    edit_->clear();
}

void AsiakasToimittajaValinta::alusta(bool toimittaja)
{
    toimittaja_ = toimittaja;
    if( isEnabled() )
        taydentajaModel_->lataa( toimittaja ? AsiakasToimittajaTaydentaja::TOIMITTAJAT : AsiakasToimittajaTaydentaja::ASIAKKAAT );

}

void AsiakasToimittajaValinta::valitseAsiakas()
{
    id_ = taydentajaModel_->haeNimella( edit_->text() );
    qDebug() << edit_->text() << " " << id_;
}

void AsiakasToimittajaValinta::muokkaa()
{
    if( toimittaja_ ) {
        if( !toimittajaDlg_ ) {
            toimittajaDlg_ = new ToimittajaDlg(this);
            connect( toimittajaDlg_, &ToimittajaDlg::toimittajaTallennettu, this, &AsiakasToimittajaValinta::talletettu);
        }
        if( id_ > 0)
            toimittajaDlg_->muokkaa( id_);
        else
            toimittajaDlg_->uusi( edit_->text() );

    } else {
        if( !asiakasDlg_ ) {
            asiakasDlg_ = new AsiakasDlg(this);
            connect( asiakasDlg_, &AsiakasDlg::asiakasTallennettu, this, &AsiakasToimittajaValinta::talletettu);
        }
        if( id_ > 0)
            asiakasDlg_->muokkaa( id_);
        else
            asiakasDlg_->uusi( edit_->text() );
    }

}

void AsiakasToimittajaValinta::talletettu(int id, const QString& nimi)
{
    id_ = id;
    edit_->setText( nimi );
}
