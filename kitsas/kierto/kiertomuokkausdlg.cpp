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
#include "kiertomuokkausdlg.h"
#include "ui_kiertomuokkausdlg.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"
#include "kiertomuokkausmodel.h"

#include <QSortFilterProxyModel>
#include <QPushButton>

KiertoMuokkausDlg::KiertoMuokkausDlg(int id, QWidget *parent, bool portaali) :
    QDialog(parent),
    ui(new Ui::KiertoMuokkausDlg),
    model(new KiertoMuokkausModel(this)),
    proxy(new QSortFilterProxyModel(this)),
    kiertoId_(id)
{
    ui->setupUi(this);

    ui->portaaliRyhma->setChecked( portaali );
    ui->portaaliRyhma->setEnabled( portaali );

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect( ui->nimiEdit, &QLineEdit::textChanged, [this] () {
        this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!this->ui->nimiEdit->text().isEmpty());
    });

    alusta();

    connect( ui->osallistujaCombo, &QComboBox::currentTextChanged, this, &KiertoMuokkausDlg::paivitaRooliValinta);
    connect( ui->lisaaNappi, &QPushButton::clicked, this, &KiertoMuokkausDlg::lisaaRivi);
    connect( ui->osallistujaView->selectionModel(), &QItemSelectionModel::currentChanged, [this] (const QModelIndex& index ) {
        this->ui->poistaNappi->setEnabled(index.isValid());
    });
    connect( ui->poistaNappi, &QPushButton::clicked, this, &KiertoMuokkausDlg::poistaRivi);
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/kierto/"); });

}

KiertoMuokkausDlg::~KiertoMuokkausDlg()
{
    delete ui;
}

void KiertoMuokkausDlg::accept()
{
    KpKysely *tallennus = kiertoId_ ?
                kpk(QString("/kierrot/%1").arg(kiertoId_), KpKysely::PUT) :
                kpk("/kierrot", KpKysely::POST);
    connect(tallennus, &KpKysely::vastaus, this, &KiertoMuokkausDlg::tallennettu);
    tallennus->kysy(data());
}

void KiertoMuokkausDlg::tyyppiMuuttuu()
{
    if( ui->tyyppiCombo->currentData().toInt() == TositeTyyppi::TULO) {
        ui->tiliCombo->suodataTyypilla("C.*");
        if( kp()->tilit()->tiliNumerolla(ui->tiliCombo->valittuTilinumero()).onko(TiliLaji::MENO))
            ui->tiliCombo->valitseTili(kp()->asetukset()->luku("OletusTulotili"));
    } else {
        ui->tiliCombo->suodataTyypilla("D.*");
        if( kp()->tilit()->tiliNumerolla(ui->tiliCombo->valittuTilinumero()).onko(TiliLaji::TULO))
            ui->tiliCombo->valitseTili(kp()->asetukset()->luku("OletusMenotili"));
    }
}

void KiertoMuokkausDlg::tallennettu()
{
    QDialog::accept();
}

void KiertoMuokkausDlg::alusta()
{
    ui->tyyppiCombo->addItem(kp()->tositeTyypit()->kuvake(TositeTyyppi::KULULASKU), tr("Kululasku"), TositeTyyppi::KULULASKU);
    ui->tyyppiCombo->addItem(kp()->tositeTyypit()->kuvake(TositeTyyppi::MENO), tr("Ostolasku"), TositeTyyppi::MENO);
    ui->tyyppiCombo->addItem(kp()->tositeTyypit()->kuvake(TositeTyyppi::SAAPUNUTVERKKOLASKU), tr("Verkkolasku"), TositeTyyppi::SAAPUNUTVERKKOLASKU);
    ui->tyyppiCombo->addItem(kp()->tositeTyypit()->kuvake(TositeTyyppi::TULO), tr("Tulotosite"), TositeTyyppi::TULO);

    ui->tiliCombo->suodataTyypilla("D.*");
    ui->tiliCombo->valitseTili(kp()->asetukset()->luku("OletusMenotili"));
    ui->vastaCombo->suodataTyypilla("(AR.|BO)");
    ui->vastaCombo->valitseTili(kp()->tilit()->tiliTyypilla(TiliLaji::PANKKITILI).numero());

    proxy->setSourceModel(model);
    ui->osallistujaView->setSortingEnabled(true);
    ui->osallistujaView->sortByColumn(KiertoMuokkausModel::ROOLI, Qt::AscendingOrder);
    proxy->setSortRole(Qt::EditRole);
    ui->osallistujaView->setModel(proxy);
    ui->osallistujaView->horizontalHeader()->setSectionResizeMode(KiertoMuokkausModel::NIMI, QHeaderView::Stretch);

    KpKysely* kysely = kpk(QString("%1/permissions/%2")
                           .arg(kp()->pilvi()->pilviLoginOsoite())
                           .arg(kp()->pilvi()->pilviId()));
    connect(kysely, &KpKysely::vastaus, this, &KiertoMuokkausDlg::kayttajatSaapuu);
    kysely->kysy();

    if(kiertoId_) {
        KpKysely* lkysely = kpk(QString("/kierrot/%1").arg(kiertoId_));
        connect(lkysely, &KpKysely::vastaus, this, &KiertoMuokkausDlg::lataa );
        lkysely->kysy();
    } else {
        setWindowTitle(tr("Uusi kierto"));
    }
    connect( ui->tyyppiCombo, &QComboBox::currentTextChanged, this, &KiertoMuokkausDlg::tyyppiMuuttuu);
}

void KiertoMuokkausDlg::kayttajatSaapuu(QVariant *data)
{
    QMap<int,QString> kayttajat;
    const QVariantList& lista = data->toList();
    ui->osallistujaCombo->clear();
    int comboIndeksi = 0;
    for(auto &item : lista) {
        const QVariantMap& map = item.toMap();
        qlonglong oikeudet = PilviModel::oikeudet(map.value("rights").toList());
        if( oikeudet & (YhteysModel::KIERTO_TARKASTAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN | YhteysModel::KIERTO_SELAAMINEN | YhteysModel::TOSITE_MUOKKAUS)) {
            ui->osallistujaCombo->addItem(map.value("name").toString(), map.value("userid"));
            ui->osallistujaCombo->setItemData(comboIndeksi++, oikeudet, Qt::UserRole + 1);
            kayttajat.insert(map.value("userid").toInt(), map.value("name").toString());
        }
    }
    model->lisaaNimet(kayttajat);
}

void KiertoMuokkausDlg::paivitaRooliValinta()
{
    ui->rooliCombo->clear();
    qlonglong oikeudet = ui->osallistujaCombo->currentData(Qt::UserRole + 1).toLongLong();
    if( oikeudet & (YhteysModel::KIERTO_TARKASTAMINEN | YhteysModel::KIERTO_HYVAKSYMINEN | YhteysModel::TOSITE_MUOKKAUS))
        ui->rooliCombo->addItem(QIcon(":/pic/inbox.png"),tr("Saapunut"), Tosite::SAAPUNUT );
    if( oikeudet & (YhteysModel::KIERTO_HYVAKSYMINEN | YhteysModel::TOSITE_MUOKKAUS))
        ui->rooliCombo->addItem(QIcon(":/pixaby/tarkastettu.svg"),tr("Tarkastettu"), Tosite::TARKASTETTU);
    if( oikeudet & YhteysModel::TOSITE_MUOKKAUS)
        ui->rooliCombo->addItem(QIcon(":/pixaby/hyvaksytty.svg"),tr("Hyväksytty"), Tosite::HYVAKSYTTY);
}

void KiertoMuokkausDlg::lisaaRivi()
{
    QVariantMap map;
    map.insert("userid", ui->osallistujaCombo->currentData(Qt::UserRole));
    map.insert("rooli", ui->rooliCombo->currentData());
    map.insert("ilmoita", ui->ilmoitaCheck->isChecked());
    map.insert("nimi", ui->osallistujaCombo->currentText());
    map.insert("tyyppi", ui->tyyppiCombo->currentData());
    model->lisaaRivi(map);

}

void KiertoMuokkausDlg::poistaRivi()
{
    QModelIndex index = ui->osallistujaView->currentIndex();
    if( index.isValid() ) {
        model->poistaRivi( proxy->mapToSource(index).row() );
    }
}

void KiertoMuokkausDlg::lataa(QVariant *data)
{
    const QVariantMap& map = data->toMap();
    ui->nimiEdit->setText(map.value("nimi").toString());
    ui->portaaliRyhma->setChecked( map.value("portaalissa").toBool() );
    ui->tiliCombo->valitseTili( map.value("tili").toInt());
    ui->vastaCombo->valitseTili( map.value("vastatili").toInt());
    ui->kohdennusCombo->valitseKohdennus( map.value("kohdennus").toInt());
    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData(map.value("tyyppi").toInt()) );
    ui->tyyppiCombo->setDisabled( map.value("id").toInt() == 1 );
    model->lataa(map.value("kierto").toList());
}

QVariantMap KiertoMuokkausDlg::data() const
{
    QVariantMap map;
    map.insert("portaalissa", ui->portaaliRyhma->isChecked());
    map.insert("nimi", ui->nimiEdit->text());
    map.insert("tili", ui->tiliCombo->valittuTilinumero());
    map.insert("vastatili", ui->vastaCombo->valittuTilinumero());
    map.insert("kohdennus", ui->kohdennusCombo->kohdennus());
    map.insert("tyyppi", ui->tyyppiCombo->currentData());
    map.insert("kierto", model->kiertoLista());
    return map;
}

