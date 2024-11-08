#include "toiminimimaaritys.h"

#include "ui_toiminimimaaritys.h"

#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "db/kirjanpito.h"
#include "model/toiminimimodel.h"
#include "rekisteri/postinumerot.h"

ToiminimiMaaritys::ToiminimiMaaritys() :
    MaaritysWidget(nullptr),
    ui(new Ui::ToiminimiMaaritys),
    proxy( new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    connect( ui->combo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToiminimiMaaritys::lataa);
    connectMuutokset();

    connect( ui->uusiButton, &QPushButton::clicked, this, &ToiminimiMaaritys::uusiToiminimi);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &ToiminimiMaaritys::poistaAputoiminimi);
    connect( ui->postinumeroEdit, &QLineEdit::textEdited, this, &ToiminimiMaaritys::haeKaupunki);
    connect( ui->logoButton, &QPushButton::clicked, this, &ToiminimiMaaritys::vaihdaLogo);
    connect( ui->eilogoButton, &QPushButton::clicked, this, &ToiminimiMaaritys::poistaLogo);

    proxy->setSourceModel(kp()->toiminimet());
    proxy->setFilterRole(ToiminimiModel::Nakyva);
    proxy->setFilterFixedString("X");
    ui->combo->setModel(proxy);

    ui->logonSijaintiCombo->addItem(QIcon(":/pic/logovieressa.png"),tr("Logo nimen vieressä"),"VIERESSA");
    ui->logonSijaintiCombo->addItem(QIcon(":/pic/logoylla.png"),tr("Logo nimen yläpuolella"),"YLLA");
    ui->logonSijaintiCombo->addItem(QIcon(":/pic/Possu64.png"),tr("Näytä vain logo"),"VAINLOGO");

}

ToiminimiMaaritys::~ToiminimiMaaritys()
{
    delete ui;
}

bool ToiminimiMaaritys::nollaa()
{
    lataa();
    return true;
}

bool ToiminimiMaaritys::onkoMuokattu()
{

    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        const ToiminimiModel* model = kp()->toiminimet();

        const QString logonSijainti = model->tieto(ToiminimiModel::LogonSijainti, indeksi);
        const QString nimi = model->tieto(ToiminimiModel::Nimi, indeksi);
        const QString osoite = model->tieto(ToiminimiModel::Katuosoite, indeksi);
        const QString postinumero = model->tieto(ToiminimiModel::Postinumero, indeksi);
        const QString kaupunki = model->tieto(ToiminimiModel::Kaupunki, indeksi);
        const QString puhelin = model->tieto(ToiminimiModel::Puhelin, indeksi);
        const QString email = model->tieto(ToiminimiModel::Sahkoposti, indeksi);
        const QString kotisivu = model->tieto(ToiminimiModel::Kotisivu, indeksi);
        const int logonKorkus = model->tieto(ToiminimiModel::LogonKorkeus, indeksi, "20").toInt();
        const QString kehys = model->tieto(ToiminimiModel::VariKehys, indeksi, "128,128,128");
        const QString varjo = model->tieto(ToiminimiModel::VariVarjo, indeksi, "230,230,230");


        return  logonSijainti != ui->logonSijaintiCombo->currentData().toString() ||
                nimi != ui->nimiLabel->text() ||
                osoite != ui->osoiteEdit->toPlainText() ||
                postinumero != ui->postinumeroEdit->text() ||
                kaupunki != ui->kaupunkiEdit->text() ||
                puhelin != ui->puhelinEdit->text() ||
                email != ui->emailEdit->text() ||
                kotisivu != ui->kotisivuEdit->text() ||
                logonKorkus != ui->logoKorkeusSpin->value() ||
                kehys != ui->kehysVari->color() ||
                varjo != ui->varjoVari->color() ;
    }
    return false;

}

bool ToiminimiMaaritys::tallenna()
{
    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        ToiminimiModel* model = kp()->toiminimet();

        model->aseta(indeksi, ToiminimiModel::LogonSijainti, ui->logonSijaintiCombo->currentData().toString());
        model->aseta(indeksi, ToiminimiModel::Nimi, ui->nimiLabel->text());
        model->aseta(indeksi, ToiminimiModel::Katuosoite, ui->osoiteEdit->toPlainText());
        model->aseta(indeksi, ToiminimiModel::Postinumero, ui->postinumeroEdit->text());
        model->aseta(indeksi, ToiminimiModel::Kaupunki, ui->kaupunkiEdit->text());
        model->aseta(indeksi, ToiminimiModel::Puhelin, ui->puhelinEdit->text());
        model->aseta(indeksi, ToiminimiModel::Sahkoposti, ui->emailEdit->text().trimmed());
        model->aseta(indeksi, ToiminimiModel::Kotisivu, ui->kotisivuEdit->text());
        model->aseta(indeksi, ToiminimiModel::LogonKorkeus, QString::number(ui->logoKorkeusSpin->value()));
        model->aseta(indeksi, ToiminimiModel::VariKehys, ui->kehysVari->color());
        model->aseta(indeksi, ToiminimiModel::VariVarjo, ui->varjoVari->color());

        model->tallenna();
        return true;
    }
    return false;
}

void ToiminimiMaaritys::lataa()
{
    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        const ToiminimiModel* model = kp()->toiminimet();

        ui->poistaNappi->setEnabled( indeksi ); // Varsinaista toiminimeä ei voi poistaa

        ui->toimiLabel->setText( indeksi ? tr("Aputoiminimi") : tr("Organisaation nimi") );

        lataaLogo();

        const QString logonSijainti = model->tieto(ToiminimiModel::LogonSijainti, indeksi);
        const QString nimi = model->tieto(ToiminimiModel::Nimi, indeksi);
        const QString osoite = model->tieto(ToiminimiModel::Katuosoite, indeksi);
        const QString postinumero = model->tieto(ToiminimiModel::Postinumero, indeksi);
        const QString kaupunki = model->tieto(ToiminimiModel::Kaupunki, indeksi);
        const QString puhelin = model->tieto(ToiminimiModel::Puhelin, indeksi);
        const QString email = model->tieto(ToiminimiModel::Sahkoposti, indeksi);
        const QString kotisivu = model->tieto(ToiminimiModel::Kotisivu, indeksi);
        const int logonKorkus = model->tieto(ToiminimiModel::LogonKorkeus, indeksi, "20").toInt();
        const QString kehys = model->tieto(ToiminimiModel::VariKehys, indeksi, "128,128,128");
        const QString varjo = model->tieto(ToiminimiModel::VariVarjo, indeksi, "230,230,230");


        ui->logonSijaintiCombo->setCurrentIndex( ui->logonSijaintiCombo->findData( logonSijainti ) );
        ui->nimiLabel->setText( nimi );
        ui->osoiteEdit->setPlainText( osoite );
        ui->postinumeroEdit->setText( postinumero );
        ui->kaupunkiEdit->setText( kaupunki );
        ui->puhelinEdit->setText( puhelin );
        ui->emailEdit->setText( email );
        ui->kotisivuEdit->setText( kotisivu );
        ui->logoKorkeusSpin->setValue(logonKorkus);
        ui->kehysVari->setColor(kehys);
        ui->varjoVari->setColor(varjo);

    }
}

void ToiminimiMaaritys::lataaLogo()
{
    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        const ToiminimiModel* model = kp()->toiminimet();

        QImage logo = model->logo(indeksi);
        ui->logoLabel->setPixmap( QPixmap::fromImage(logo.scaled(128,128*3,Qt::KeepAspectRatio, Qt::SmoothTransformation)) );
        ui->eilogoButton->setEnabled(!logo.isNull());
    }
}

void ToiminimiMaaritys::uusiToiminimi()
{
    QString nimi = QInputDialog::getText(this,
                                         tr("Lisää aputoiminimi"),
                                         tr("Lisättävä aputoiminimi"));
    if( !nimi.isEmpty()) {
        ToiminimiModel* model = kp()->toiminimet();
        int indeksi = model->lisaaToiminimi(nimi);
        ui->combo->setCurrentIndex(proxy->mapFromSource(model->index(indeksi)).row());
        model->tallenna();
    }
}

void ToiminimiMaaritys::haeKaupunki()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty())
        ui->kaupunkiEdit->setText(toimipaikka);
}

void ToiminimiMaaritys::poistaAputoiminimi()
{
    if( ui->combo->currentIndex() > 0) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        ToiminimiModel* model = kp()->toiminimet();
        const QString nimi = model->tieto(ToiminimiModel::Nimi,indeksi);

        if( QMessageBox::question(this,
                                  tr("Aputoiminimen poistaminen"),
                                  tr("Haluatko todella poistaa aputoiminimen %1 ?").arg(nimi),
                                  QMessageBox::Yes | QMessageBox::Cancel,
                                  QMessageBox::Cancel) == QMessageBox::Yes) {
            ui->combo->setCurrentIndex(0);
            model->aseta(indeksi, ToiminimiModel::Piilossa, "X");
            model->tallenna();
        }
    }
}

void ToiminimiMaaritys::vaihdaLogo()
{
    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        ToiminimiModel* model = kp()->toiminimet();
        QString tiedostopolku = QFileDialog::getOpenFileName(this,"Valitse logo", QDir::homePath(),tr("Kuvatiedostot (*.png *.jpg *.jpeg)") );
        if( !tiedostopolku.isEmpty())
        {
            QImage uusilogo;
            uusilogo.load( tiedostopolku );
            model->asetaLogo(indeksi, uusilogo);
            lataaLogo();
        }
    }
}

void ToiminimiMaaritys::poistaLogo()
{
    if( ui->combo->currentIndex() > -1) {
        const int indeksi = ui->combo->currentData(ToiminimiModel::Indeksi).toInt();
        ToiminimiModel* model = kp()->toiminimet();
        if( QMessageBox::question(this,
                                  tr("Logon poistaminen"),
                                  tr("Haluatko todella poistaa tämän logon?"),
                                  QMessageBox::Yes | QMessageBox::Cancel,
                                  QMessageBox::Cancel) == QMessageBox::Yes) {
            model->asetaLogo(indeksi, QImage());
            lataaLogo();
        }
    }
}
