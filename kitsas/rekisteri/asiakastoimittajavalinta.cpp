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
#include "db/kirjanpito.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCompleter>
#include <QComboBox>
#include <QCompleter>

#include <QDebug>
#include <QKeyEvent>
#include <QFocusEvent>

AsiakasToimittajaValinta::AsiakasToimittajaValinta(QWidget *parent) :
    QWidget(parent),
    combo_( new QComboBox(this)),
    button_(new QPushButton(this)),
    model_( AsiakasToimittajaListaModel::instanssi()),
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
    connect( model_, &AsiakasToimittajaListaModel::modelAboutToBeReset, this, [this] () {this->modelLataa_=true;});
    connect( model_, &AsiakasToimittajaListaModel::modelReset, this, &AsiakasToimittajaValinta::modelLadattu);


    connect(combo_, QOverload<int>::of(&QComboBox::activated), this, &AsiakasToimittajaValinta::nimiMuuttui);
    connect( combo_->lineEdit(), &QLineEdit::editingFinished, this, &AsiakasToimittajaValinta::syotettyNimi);
    connect( combo_, &QComboBox::currentTextChanged, this, &AsiakasToimittajaValinta::nimiMuuttui );

    connect( dlg_, &AsiakasToimittajaDlg::kumppaniTallennettu, this, &AsiakasToimittajaValinta::tallennettu);

    combo_->lineEdit()->setPlaceholderText(tr("Y-tunnus tai nimi"));
    combo_->installEventFilter(this);

    setFocusProxy( combo_ );
    button_->setFocusPolicy(Qt::NoFocus);    
    clear();
}

int AsiakasToimittajaValinta::id() const
{
    return map_.value("id").toInt();
}

QString AsiakasToimittajaValinta::nimi() const
{
    return map_.value("nimi").toString().trimmed();
}

QStringList AsiakasToimittajaValinta::ibanit() const
{
    return map_.value("iban").toStringList();
}

QVariantMap AsiakasToimittajaValinta::map() const
{
    return map_;
}

void AsiakasToimittajaValinta::clear()
{
    map_.clear();
    setId(0);
    combo_->setCurrentText(QString());
}


void AsiakasToimittajaValinta::tuonti(const QVariantMap &data)
{    
    QString alvtunnari = data.contains("alvtunnus") ? data.value("alvtunnus").toString() :
            "FI" + data.value("kumppaniytunnus").toString();
    alvtunnari.remove('-');
    const QString nimi = data.contains("kumppaninimi") ? data.value("kumppaninimi").toString() : data.value("nimi").toString();

    if( model_->idAlvTunnuksella( alvtunnari) ) {
        // Valitaan Y-tunnuksella
        combo_->setCurrentIndex( combo_->findData( model_->idAlvTunnuksella(alvtunnari) ) );        
    } else if( combo_->findText(nimi) > -1) {
        // Valitaan nimellä
        combo_->setCurrentIndex( combo_->findText( nimi ) );
    } else {
        // Siirrytään dialogiin                
        // Pitäisikö yrittää vielä tilinumerolla ?
        if( data.value("iban").toStringList().isEmpty()) {
            ibanLoytyi(data, nullptr);
        } else {
            QString ekaIban = data.value("iban").toStringList().value(0);
            KpKysely* ibankysely = kpk("/kumppanit");
            ibankysely->lisaaAttribuutti("iban", ekaIban);
            connect(ibankysely, &KpKysely::vastaus, this, [this,data] (QVariant* vastaus) {this->ibanLoytyi(data, vastaus);} );
            connect(ibankysely, &KpKysely::virhe, this, [this, data] {this->ibanLoytyi(data, nullptr);});
            ibankysely->kysy();
        }
    }    
}

void AsiakasToimittajaValinta::valitse(const QVariantMap &map)
{
    map_ = map;
    lataa_ = 0;
    if( id()) {
        int indeksi = combo_->findData( id() );
        if( indeksi > -1)
            combo_->setCurrentIndex(indeksi);
        else
            combo_->setCurrentText( nimi());
    } else {
        combo_->setCurrentText( nimi() );
    }
    setId( id() );
}

void AsiakasToimittajaValinta::valitse(int kumppaniId)
{
    KpKysely* kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    lataa_ = kumppaniId;
    connect( kysely, &KpKysely::vastaus, this, &AsiakasToimittajaValinta::lataa);
    kysely->kysy();
}

void AsiakasToimittajaValinta::lataa(QVariant *data)
{
    QVariantMap map = data->toMap();
    if( lataa_ != map.value("id").toInt())
        return;

    map_ = map;
    lataa_ = 0;    

    int indeksi = combo_->findData( id() );
    combo_->setCurrentIndex(indeksi);
    setId(id());
    emit muuttui( map_);
}

void AsiakasToimittajaValinta::naytaNappi(bool nayta)
{
    button_->setVisible(nayta);
}

void AsiakasToimittajaValinta::nimiMuuttui()
{
    if( modelLataa_)
        return;

    QString syotetty = combo_->currentText().trimmed();

    if( syotetty == nimi() )
        return;

    int lid = combo_->currentData(AsiakasToimittajaListaModel::IdRooli).toInt();
    const QString& haettuNimi = model_->nimi(lid);
    if( haettuNimi != syotetty )
        lid = model_->idNimella(syotetty);

    if( lid > 0) {
        if( lid == lataa_ || lid == id()) {
            // On jo latauksessa taikka esillä ;)
            return;
        }
        // Ladataan tämä
        valitse(lid);

    }
    // Jos on syötetty y-tunnus, haetaan sillä
    else if( YTunnusValidator::kelpaako( syotetty )) {
        QString alvtunnari = "FI" + combo_->currentText();
        int hakuId = model_->idAlvTunnuksella( alvtunnari.remove('-'));
        if( hakuId )
            combo_->setCurrentIndex( combo_->findData(hakuId) );
        else
            dlg_->ytunnuksella( combo_->currentText());
    } else {        
        map_.clear();        
        setId(0);
        if( !syotetty.isEmpty()) {
            map_.insert("nimi", syotetty);
        };
        emit muuttui(map_);
    }
}

void AsiakasToimittajaValinta::syotettyNimi()
{
    if(property("MuokkaaUusi").toBool() && !id() && !nimi().trimmed().isEmpty() )
        muokkaa();
}

void AsiakasToimittajaValinta::muokkaa()
{

    if( id() ) {
        dlg_->tauluun( map() );
        dlg_->show();
    } else
        dlg_->uusi( combo_->currentText());

}

void AsiakasToimittajaValinta::modelLadattu()
{
    if( id() )
    {
        int indeksi = combo_->findData( id() );
        if( indeksi > 0)
            combo_->setCurrentIndex(indeksi);
    } else {
        combo_->setCurrentIndex(-1);
        combo_->setCurrentText(QString());
    }
    modelLataa_ = false;
}

void AsiakasToimittajaValinta::ibanLoytyi(const QVariantMap &tuontiData, QVariant *data)
{
    QVariantMap map = data ? data->toMap() : QVariantMap();
    if(map.isEmpty()) {
        dlg_->tuonti(tuontiData);
    } else {
        map_ = data ? data->toMap() : QVariantMap();
        if( id()) {
            int indeksi = combo_->findData( id() );
            combo_->setCurrentIndex(indeksi);
        } else {
            combo_->setCurrentText( nimi() );
        }
    }    
}

void AsiakasToimittajaValinta::tallennettu(const QVariantMap &map)
{
    map_ = map;
    lataa_ = 0;
    combo_->setCurrentText( map.value("nimi").toString());
    emit muuttui(map);
}


void AsiakasToimittajaValinta::setId(int id)
{    
    if( id ) {
        button_->setIcon(QIcon(":/pic/muokkaaasiakas.png"));
    } else
        button_->setIcon(QIcon(":/pic/uusiasiakas.png"));
}

bool AsiakasToimittajaValinta::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == combo_ && event->type()==QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if( keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Return) {
            muokkaa();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


