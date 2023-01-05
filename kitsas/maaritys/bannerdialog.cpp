#include "bannerdialog.h"
#include "ui_bannerdialog.h"

#include "model/bannermodel.h"

#include <QFileDialog>
#include <QMessageBox>

#include "db/kirjanpito.h"

BannerDialog::BannerDialog(QWidget *parent, BannerModel *model) :
    QDialog(parent),
    ui(new Ui::BannerDialog),
    model_(model)

{
    ui->setupUi(this);
    connect( ui->kuvaNappi, &QPushButton::clicked, this, &BannerDialog::vaihdaKuva );
    connect( ui->nimiEdit, &QLineEdit::textEdited, this, &BannerDialog::tarkasta);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/bannerit/");});
}

BannerDialog::~BannerDialog()
{
    delete ui;
}

void BannerDialog::muokkaa(const QModelIndex &indeksi)
{
    ui->nimiEdit->setText( indeksi.data(BannerModel::NimiRooli).toString());
    uuid_ = indeksi.data(BannerModel::IdRooli).toString();
    kuva_ = indeksi.data(BannerModel::KuvaRooli).value<QImage>();

    QPixmap pixmap = QPixmap::fromImage(kuva_.scaledToWidth( width() - 40, Qt::SmoothTransformation ));
    ui->kuvaLabel->setPixmap(pixmap);    
    ui->tunnisteText->setText(uuid_);

    tarkasta();
    exec();
}

void BannerDialog::uusi()
{
    ui->tunnisteLabel->hide();
    ui->tunnisteText->hide();
    tarkasta();

    exec();
}


void BannerDialog::vaihdaKuva()
{
    QString tiedosto = QFileDialog::getOpenFileName(this,
                                                    tr("Valitse lisättävä bannerikuva"),
                                                    QString(),
                                                    tr("Kuvatiedostot (*.png *.jpg *.jpeg)"));
    if( tiedosto.isEmpty()) return;

    QImage kuva;
    kuva.load(tiedosto);

    double skaala = kuva.size().width() / kuva.size().height();
    if( skaala < 3) {
        QMessageBox::critical(this, tr("Bannerikuvan muoto ei kelpaa"), tr("Bannerikuvan korkeus saa olla korkeintaan kolmasosa kuvan leveydestä."));
        return;
    }

    kuva_ = kuva;
    QPixmap pixmap = QPixmap::fromImage(kuva.scaledToWidth( width() - 40, Qt::SmoothTransformation ));
    ui->kuvaLabel->setPixmap(pixmap);

    tarkasta();
}

void BannerDialog::tarkasta()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( !kuva_.isNull() && !ui->nimiEdit->text().isEmpty() );
}

void BannerDialog::accept()
{
    if( uuid_.isEmpty() ) {
        model_->lisaa( ui->nimiEdit->text(), kuva_);
    } else {
        model_->muuta( uuid_, ui->nimiEdit->text(), kuva_);
    }

    QDialog::accept();
}
