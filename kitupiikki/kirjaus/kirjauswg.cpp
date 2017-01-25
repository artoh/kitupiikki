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

#include "kirjauswg.h"
#include "vientimodel.h"
#include "tilidelegaatti.h"
#include "eurodelegaatti.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QSqlQuery>
#include <QMessageBox>

KirjausWg::KirjausWg(TositeWg *tosite) : QWidget(), tositewg(tosite), tositeId(0)
{
    viennitModel = new VientiModel(this);

    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel(viennitModel);
    connect( viennitModel, SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(setCurrentIndex(QModelIndex)));
    connect( viennitModel, SIGNAL(siirryRuutuun(QModelIndex)), ui->viennitView, SLOT(edit(QModelIndex)));
    connect( viennitModel, SIGNAL(muuttunut()), this, SLOT(naytaSummat()));

    ui->viennitView->setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti() );

    ui->viennitView->setItemDelegateForColumn( VientiModel::DEBET, new EuroDelegaatti);
    ui->viennitView->setItemDelegateForColumn( VientiModel::KREDIT, new EuroDelegaatti);

    ui->viennitView->hideColumn(VientiModel::PROJEKTI);
    ui->viennitView->hideColumn(VientiModel::KUSTANNUSPAIKKA);
    ui->viennitView->horizontalHeader()->setStretchLastSection(true);

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->tallennaButton, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
    connect( ui->hylkaaNappi, SIGNAL(clicked(bool)), this, SLOT(hylkaa()));
    connect( ui->kommentitEdit, SIGNAL(textChanged()), this, SLOT(paivitaKommenttiMerkki()));

    connect( ui->tunnisteEdit, SIGNAL(textChanged(QString)), this, SLOT(tarkistaTunniste()));
    connect( ui->tositePvmEdit, SIGNAL(dateChanged(QDate)), this, SLOT(korjaaTunniste()));

    tyhjenna();
}

KirjausWg::~KirjausWg()
{
    delete ui;
    delete viennitModel;
}

QDate KirjausWg::tositePvm() const
{
    return ui->tositePvmEdit->date();
}

void KirjausWg::lisaaRivi()
{
    viennitModel->lisaaRivi();
    ui->viennitView->setFocus();

    QModelIndex indeksi = viennitModel->index( viennitModel->rowCount(QModelIndex()) - 1, VientiModel::TILI );

    ui->viennitView->setCurrentIndex( indeksi  );
    ui->viennitView->edit( indeksi );
}

void KirjausWg::tyhjenna()
{
    tositeId = 0;
    ui->tositePvmEdit->setDate( Kirjanpito::db()->paivamaara() );
    ui->otsikkoEdit->clear();
    ui->kommentitEdit->clear();
    ui->tositePvmEdit->setFocus();

    ui->tunnisteEdit->clear();

    // Haetaan seuraava vapaa tunniste
    QSqlQuery query;

    ui->tunnisteEdit->setText( QString::number( seuraavaNumero() ) );

    ui->idLabel->clear();

    tositewg->tyhjenna();


    viennitModel->tyhjaa();
    ui->tabWidget->setCurrentIndex(0);
}

void KirjausWg::tallenna()
{
    // Ensin tarkistetaan, että täsmää
    int debetit = viennitModel->debetSumma();
    int kreditit = viennitModel->kreditSumma();

    if( debetit == 0 && kreditit == 0)
    {
        QMessageBox::critical(this, tr("Kitupiikki"), tr("Vientejä ei ole kirjattu"));
        return;
    }
    else if( debetit != kreditit)
    {
        // Kirjausten puolet eivät täsmää.
        if( QMessageBox::critical(this,tr("Kitupiikki"), tr("Debet- ja kredit-kirjaukset eivät täsmää.\n"
                                                       "Haluatko kuitenkin tallentaa kirjaukset?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes )
            return;
    }
    // Tarkistetaan, että joka rivillä on tili
    for(int i=0; i<viennitModel->rowCount(QModelIndex()); i++)
    {
        if( viennitModel->index(i, VientiModel::TILI).data().isNull() )
        {
            QMessageBox::critical(this, tr("Kitupiikki"), tr("Rivillä %1 tili on tyhjä").arg(i + 1));
            return;
        }
    }


    QSqlQuery query;

    if( tositeId )
    {
        query.prepare("UPDATE tosite SET pvm=:pvm, otsikko=:otsikko, kommentti=:kommentti, tunniste=:tunniste "
                      "WHERE id=:id");
        query.bindValue(":id", tositeId);
    }
    else
        query.prepare("INSERT INTO tosite(pvm,otsikko,kommentti,tunniste) values(:pvm,:otsikko,:kommentti,:tunniste)");

    query.bindValue(":pvm", ui->tositePvmEdit->date());
    query.bindValue(":otsikko", ui->otsikkoEdit->text());
    query.bindValue(":kommentti", ui->kommentitEdit->document()->toPlainText());
    if( !ui->tunnisteEdit->text().isEmpty())
        query.bindValue(":tunniste", ui->tunnisteEdit->text());
    query.exec();

    if( !tositeId)
        tositeId = query.lastInsertId().toInt();


    tositewg->tallennaTosite(tositeId); // Tallentaa tositetiedoston

    viennitModel->tallenna(tositeId);   // Tallentaa viennit

    Kirjanpito::db()->muokattu(); // Ilmoittaa, että kirjanpitoa on muokattu ja näkymät pitää päivittää

    tyhjenna(); // Tyhjennetään
    emit Kirjanpito::db()->palaaEdelliselleSivulle();
}

void KirjausWg::hylkaa()
{
    tyhjenna();
    emit Kirjanpito::db()->palaaEdelliselleSivulle();
}

void KirjausWg::naytaSummat()
{
    ui->summaLabel->setText( tr("Debet %L1 €    Kredit %L2 €").arg(((double)viennitModel->debetSumma())/100.0 ,0,'f',2)
                             .arg(((double)viennitModel->kreditSumma()) / 100.0 ,0,'f',2));
}

void KirjausWg::lataaTosite(int id)
{
    QSqlQuery query;
    query.exec( QString("SELECT pvm, otsikko, kommentti, tunniste, tiedosto, tunniste FROM tosite WHERE id=%1").arg(id) );
    if( query.next())
    {
        tositeId = id;
        ui->tositePvmEdit->setDate( query.value("pvm").toDate() );
        ui->otsikkoEdit->setText( query.value("otsikko").toString());
        ui->kommentitEdit->setPlainText( query.value("kommentti").toString());
        ui->tunnisteEdit->setText( query.value("tunniste").toString());

        tositewg->tyhjenna( query.value("tunniste").toString(), query.value("tiedosto").toString() );

        ui->idLabel->setText(tr("# %1").arg(id));

        // Sitten ladataan vielä viennit
        viennitModel->lataa(id);
        naytaSummat();
        ui->tabWidget->setCurrentIndex(VIENNIT);
    }
}

void KirjausWg::paivitaKommenttiMerkki()
{
    if( ui->kommentitEdit->document()->toPlainText().isEmpty())
    {
        ui->tabWidget->setTabIcon(1, QIcon());
    }
    else
    {
        ui->tabWidget->setTabIcon(1, QIcon(":/pic/kommentti.png"));
    }

}

void KirjausWg::tarkistaTunniste()
{

    if( kelpaakoTunniste() )
    {
        ui->tunnisteEdit->setStyleSheet("color: black;");
    }
    else
    {
        ui->tunnisteEdit->setStyleSheet("color: red;");
    }
}

void KirjausWg::korjaaTunniste()
{
    if( !kelpaakoTunniste())
        ui->tunnisteEdit->setText( QString::number(seuraavaNumero()) );
    tarkistaTunniste();
}

int KirjausWg::seuraavaNumero()
{
    Tilikausi kausi = Kirjanpito::db()->tilikausiPaivalle( ui->tositePvmEdit->date());

    QString kysymys = QString("SELECT max(tunniste) FROM tosite WHERE abs(tunniste)>0 "
                    "AND pvm BETWEEN \"%1\" AND \"%2\" ")
                                .arg(kausi.alkaa().toString(Qt::ISODate))
                                .arg(kausi.paattyy().toString(Qt::ISODate));
    qDebug() << kysymys;

    QSqlQuery kysely;
    kysely.exec(kysymys);
    if( kysely.next())
        return kysely.value(0).toInt() + 1;
    else
        return 0;
}

bool KirjausWg::kelpaakoTunniste()
{
    // Onko kyseisellä kaudella jo tämä tunniste käytössä????
    Tilikausi kausi = Kirjanpito::db()->tilikausiPaivalle( ui->tositePvmEdit->date() );
    QString kysymys = QString("SELECT id FROM tosite WHERE tunniste=\"%1\" "
                              "AND pvm BETWEEN \"%2\" AND \"%3\" AND id <> %4").arg(ui->tunnisteEdit->text())
                                                          .arg(kausi.alkaa().toString(Qt::ISODate))
                                                          .arg(kausi.paattyy().toString(Qt::ISODate))
                                                          .arg(tositeId);
    qDebug() << kysymys;
    QSqlQuery kysely;
    kysely.exec(kysymys);
    return !kysely.next();
}
