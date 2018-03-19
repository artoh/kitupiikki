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

#include <QFileDialog>
#include <QDirIterator>
#include <QLineEdit>
#include <QDate>

#include "tilikarttasivu.h"

#include "uusikirjanpito.h"


TilikarttaSivu::TilikarttaSivu()
{
    setTitle("Tilikartta");
    ui = new Ui::TilikarttaSivu();
    ui->setupUi(this);

    // Lisätään piilotettu QLineEdit, jotta voidaan laittaa
    // valitun tiedoston polku velhon kenttään
    QLineEdit *kedit = new QLineEdit(this);
    kedit->hide();
    registerField("tilikartta*", kedit);

    connect(ui->lataaTiedostoButton, SIGNAL(clicked(bool)),
            this, SLOT(lataaTiedostosta()));

    connect(ui->tilikarttaList, SIGNAL(itemSelectionChanged()),
            this, SLOT(listaltaValittu()));

    lataaSisaisetKartat();
}

TilikarttaSivu::~TilikarttaSivu()
{
    delete ui;
}

void TilikarttaSivu::lataaSisaisetKartat()
{
    // Lataa listanäytölle resursseihin tallennetut tilikartat

    QDirIterator it(":/tilikartat");
    QStringList karttalista;
    while( it.hasNext())
    {
        karttalista << it.next();
    }

    for(QString polku : karttalista)
    {
        QString nimi = UusiKirjanpito::lueKtkTiedosto(polku).value("TilikarttaNimi").join(" ");

        QListWidgetItem *item = new QListWidgetItem(nimi, ui->tilikarttaList);
        item->setData(Qt::UserRole, polku);
    }

    ui->tilikarttaList->sortItems();

}

void TilikarttaSivu::initializePage()
{
    ui->tilikarttaList->selectionModel()->clearSelection();
    ui->tilikarttaList->setCurrentRow(-1);
    ui->kuvausBrowser->clear();
}

void TilikarttaSivu::lataaTiedostosta()
{
    QString tnimi = QFileDialog::getOpenFileName(this,
                                                 "Valitse tilikartta",
                                                 QDir::homePath(),
                                                 "Tilikartta (*.kpk)");
    if( !tnimi.isEmpty() )
    {
        valitseTilikartta(tnimi);
        ui->tilikarttaList->selectionModel()->clearSelection();
    }
}

void TilikarttaSivu::valitseTilikartta(const QString &polku)
{
    QMap<QString,QStringList> tiedot = UusiKirjanpito::lueKtkTiedosto(polku);

    QString kuvaus = tiedot.value("TilikarttaKuvaus").join("\n");
    QDate karttapaiva = QDate::fromString( tiedot.value("TilikarttaPvm").join(""), Qt::ISODate);

    QString info = tr("<b>%1</b><br>%2<br>%3<p>")
            .arg( tiedot.value("TilikarttaNimi").join(" "))
            .arg( tiedot.value("TilikarttaTekija").join(" "))
            .arg( karttapaiva.toString("dd.MM.yyyy"));

    ui->kuvausBrowser->setHtml( info + "</p>" + kuvaus );

    setField("tilikartta", polku);
}

void TilikarttaSivu::listaltaValittu()
{
    QList<QListWidgetItem*> valitut = ui->tilikarttaList->selectedItems();
    if( valitut.count())
    {
        QString polku = valitut.first()->data(Qt::UserRole).toString();
        valitseTilikartta(polku);
    }
}
