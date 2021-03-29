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

#include <QSettings>
#include <QJsonDocument>
#include <QVariant>
#include <QClipboard>
#include <QTableView>
#include <QMessageBox>
#include <QFileDialog>

#include "devtool.h"
#include "ui_devtool.h"

#include "db/kirjanpito.h"

#include "db/kpkysely.h"

#include "kitsaslokimodel.h"

DevTool::DevTool(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DevTool)
{
    ui->setupUi(this);

    connect( ui->avainEdit, SIGNAL(textChanged(QString)), this, SLOT(haeAsetus(QString)));

    connect( ui->tallennaAsetusNappi, SIGNAL(clicked(bool)), this, SLOT(tallennaAsetus()));
    connect( ui->poistaNappi, SIGNAL(clicked(bool)), this, SLOT(poistaAsetus()));

    connect( ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabMuuttui(int)));
    connect( ui->kyselyLine, &QLineEdit::returnPressed, this, &DevTool::kysely);    

    ui->avainLista->setCurrentRow(0);

    ui->lokiView->setModel(KitsasLokiModel::instanssi());
    connect( ui->lokiView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DevTool::lokiLeikepoydalle );
    connect( ui->copyButton, &QPushButton::clicked, [] {KitsasLokiModel::instanssi()->copyAll();});
    connect( ui->vieNappi, &QPushButton::clicked, this, &DevTool::vie);

    alustaRistinolla();

}

DevTool::~DevTool()
{
    delete ui;
}

void DevTool::haeAsetus(const QString &asetus)
{    
    if(asetus.contains("Password", Qt::CaseInsensitive))
        ui->arvoEdit->setPlainText("**********");
    else
        ui->arvoEdit->setPlainText( kp()->asetukset()->asetus(asetus) );
}

void DevTool::tallennaAsetus()
{
    QString avain = ui->avainEdit->text();
    QString arvo = ui->arvoEdit->toPlainText();

    // Tallennetaan asetus
    kp()->asetukset()->aseta(avain, arvo);

    // Jos asetusta ei vielä ollut, lisätään se listaan

    QList<QListWidgetItem*> items = ui->avainLista->findItems(avain, Qt::MatchExactly);
    if( !arvo.isEmpty() && !items.count() )
    {
        // Etsitään paikka aakkosista
        int indeksi = 0;
        for( indeksi = 0; indeksi < ui->avainLista->count(); indeksi++)
            if( ui->avainLista->item(indeksi)->text() > avain)
            {
                ui->avainLista->insertItem(indeksi, avain);
                ui->avainLista->setCurrentRow(indeksi);
                return;
            }
        ui->avainLista->addItem(avain);
        ui->avainLista->setCurrentRow( ui->avainLista->count() );
    }

}

void DevTool::poistaAsetus()
{
    QString avain = ui->avainEdit->text();

    kp()->asetukset()->poista(avain);

    // Jos asetus poistui, poistetaan
    QList<QListWidgetItem*> items = ui->avainLista->findItems(avain, Qt::MatchExactly);
    if( items.count())
    {
        delete items.first();
    }
}

void DevTool::tabMuuttui(int tab)
{
    if( tab == 1)
    {
        ui->avainLista->clear();
        ui->avainLista->addItems( kp()->asetukset()->avaimet() );
    }
}

void DevTool::kysely()
{
    if( !kp()->yhteysModel() )
        return;

    KpKysely::Metodi metodi = KpKysely::GET;
    if( ui->postRadio->isChecked())
        metodi = KpKysely::POST;
    else if( ui->putRadio->isChecked())
        metodi = KpKysely::PUT;
    else if( ui->patchRadio->isChecked())
        metodi = KpKysely::PATCH;
    else if( ui->deleteRadio->isChecked())
        metodi = KpKysely::DELETE;

    KpKysely *kysely = kpk( QString(), metodi);
    kysely->asetaUrl(ui->kyselyLine->text());

    connect( kysely, &KpKysely::vastaus, this, &DevTool::vastausSaapui);        
    kysely->kysy( QJsonDocument::fromJson( ui->jsonInput->toPlainText().toUtf8() ).toVariant());
}

void DevTool::vastausSaapui(QVariant *vastaus)
{
    ui->kyselyBrowser->setPlainText(  QString::fromUtf8( QJsonDocument::fromVariant(*vastaus).toJson(QJsonDocument::Indented) ) );
    sender()->deleteLater();
}

void DevTool::uusiPeli()
{
    ui->tulosLabel->clear();
    pelissa_ = true;
    peliRuudut_.fill(0);
    for( int i : pelinapit_.keys())
        pelinapit_[i]->setIcon(QIcon(":/pic/tyhja.png"));
}

void DevTool::peliNapautus(int ruutu)
{
    if(!pelissa_ || peliRuudut_.at(ruutu))
        return;

    pelinapit_[ruutu]->setIcon(QIcon(":/pic/Possu64.png"));
    peliRuudut_[ruutu] = 1;
    if( tarkastaVoitto() )
        return;

    if( !peliRuudut_.contains(0))
    {
        ui->tulosLabel->setText("Tasapeli");
        return;
    }

    int siirto = 0;
    if( peliRuudut_.at(4) == 0)
        siirto = 4;     // Valtaa mielellään keskiruudun

    qsrand( QTime::currentTime().msecsSinceStartOfDay());
    while( !siirto )
    {
        int ehdokas = qrand() % 9;
        if( !peliRuudut_.at(ehdokas))
            siirto = ehdokas;
    }

    // Etsii voittoa
    // Yksinkertaisella tekoälyllä yrittää ensin voittaa itse,
    // toissijaisesti estää toisen voiton

    bool voittosiirtoloytyi = false;

    for(int i=0; i < 9; i++)
    {
        if(peliRuudut_.at(i))
            continue;

        // Kokeillaan
        QVector<int> koe(peliRuudut_);
        koe[i]=2;
        if( voitonTarkastaja(koe) == 2)
        {
            siirto = i;
            voittosiirtoloytyi = true;
            break;
        }
    }
    // Vielä kokeillaan mitä toinen voi vastata
    if( !voittosiirtoloytyi)
    {
        for(int j=0; j<9; j++)
        {
            if( peliRuudut_.at(j))
                continue;
            QVector<int> vasta(peliRuudut_);
            vasta[j]=1;
            if( voitonTarkastaja(vasta) == 1)
                siirto = j;
        }
    }

    // Laittaa siirron
    pelinapit_[siirto]->setIcon(QIcon(":/pic/vero.png"));
    peliRuudut_[siirto] = 2;
    tarkastaVoitto();

}

void DevTool::lokiLeikepoydalle()
{
    int rivi = ui->lokiView->currentIndex().row();
    QModelIndex index = ui->lokiView->currentIndex().sibling(rivi, KitsasLokiModel::MESSAGE);
    QString message = ui->lokiView->model()->data(index, Qt::DisplayRole).toString();

    if(!message.isEmpty())
        qApp->clipboard()->setText(message);
}

void DevTool::vie()
{
    QString nimi = ui->tilikarttaNimi->text().toLower().replace(QRegularExpression("\\W"),"") + ".kitsaskartta";

    QString tiedosto = QFileDialog::getSaveFileName(this, QString(), nimi,
                                                    tr("Kitsaan tilikartta (*.kitsaskartta)","kitsaskartta on tiedostopääte - älä käännä!"));
    if( !tiedosto.isEmpty()) {
        KpKysely* kysely = kpk("/init");
        connect(kysely, &KpKysely::vastaus, [this, tiedosto] (QVariant* data) {this->vieKartta(data, tiedosto);}  );
        kysely->kysy();
    }
}

int DevTool::voitonTarkastaja(const QVector<int>& taulu)
{
    QStringList rivit;
    rivit << "012" << "345" << "678" << "036" << "147" << "258" << "048" << "246";
    for(const QString& r : rivit)
    {
        int tulos = voittajaRivilla(taulu, r.at(0).digitValue(), r.at(1).digitValue(), r.at(2).digitValue());
        if( tulos )
            return tulos;
    }
    return 0;
}

int DevTool::tarkastaVoitto()
{
    int tulos = voitonTarkastaja(peliRuudut_);
    if( tulos )
    {
        if( tulos == 1 )
            ui->tulosLabel->setText("Saat veronpalautusta!");
        else
            ui->tulosLabel->setText("Maksat lisäveroa!");
        pelissa_ = false;
    }
    return tulos;
}

int DevTool::voittajaRivilla(const QVector<int>& taulu, int a, int b, int c) const
{
    if( taulu.at(a) == taulu.at(b) && taulu.at(b) == taulu.at(c))
        return taulu.at(a);
    else
        return 0;
}

void DevTool::vieKartta(QVariant *init, const QString &tiedosto)
{
    QVariantMap map = init->toMap();

    for(const auto& avain :  QStringList { "kierrot", "kohdennukset", "tilikaudet", "tositesarjat"})
        map.remove(avain);

    QVariantMap asetukset = map.value("asetukset").toMap();
    for(const auto& avain : QStringList{"AlvAlkaa", "LaskuAputoiminimi", "LaskuIbanit", "LaskuNumerointialkaa",
        "Nimi", "Tilinavaus", "TilinavausPvm", "TilitPaatetty", "UID", "KpVersio", "Harjoitus",
        "Luotu", "LuotuVersiolla"})
        asetukset.remove(avain);

    for(const auto& avain : asetukset.keys()) {
        QString arvo = asetukset.value(avain).toString();
        QJsonDocument json = QJsonDocument::fromJson( arvo.toUtf8() );
        if( json.isArray() || json.isObject())
            asetukset.insert(avain, json.toVariant());
    }

    asetukset.insert("Tilikartta", ui->tilikarttaNimi->text());
    asetukset.insert("TilikarttaTekija", ui->tilikarttaTekija->text());
    asetukset.insert("TilikarttaPvm", QDate::currentDate().toString(Qt::ISODate));
    map.insert("asetukset", asetukset);

    QFile file(tiedosto);
    if( file.open(QIODevice::WriteOnly)) {
        file.write( QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
        QMessageBox::information(this, tr("Tilikartan vieminen onnistui"),
                                 tr("Tilikartta viety tiedostoon %1").arg(tiedosto));
    } else {
        QMessageBox::critical(this, tr("Tilikartan vieminen epäonnistui"),
                                     tr("Tiedostoon %1 ei voitu kirjoittaa").arg(tiedosto));
    }

}

void DevTool::alustaRistinolla()
{
    peliRuudut_.fill(0,9);

    for(int x=0;x<3;x++)
    {
        for(int y=0;y<3;y++)
        {
            QPushButton *peliNappi = new QPushButton();
            pelinapit_.insert(y*3+x, peliNappi);
            peliNappi->setIcon(QIcon(":/pic/tyhja.png"));
            peliNappi->setIconSize(QSize(64,64));
            ui->peliLeiska->addWidget(peliNappi,x,y);

            connect( peliNappi, &QPushButton::clicked, [this, x,y] { peliNapautus(y*3+x); });
        }
    }
    connect( ui->uusipeliNappi, SIGNAL(clicked(bool)), this, SLOT(uusiPeli()));
}

