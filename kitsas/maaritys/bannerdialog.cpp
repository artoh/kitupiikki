#include "bannerdialog.h"
#include "ui_bannerdialog.h"

#include "model/bannermodel.h"

#include <QFileDialog>
#include <QMessageBox>

BannerDialog::BannerDialog(QWidget *parent, BannerModel *model) :
    QDialog(parent),
    ui(new Ui::BannerDialog),
    model_(model)

{
    ui->setupUi(this);
    connect( ui->kuvaNappi, &QPushButton::clicked, this, &BannerDialog::vaihdaKuva );
    connect( ui->nimiEdit, &QLineEdit::textEdited, this, &BannerDialog::tarkasta);
}

BannerDialog::~BannerDialog()
{
    delete ui;
}

void BannerDialog::muokkaa(const QModelIndex indeksi)
{
    indeksi_ = indeksi.data(BannerModel::IndeksiRooli).toInt();
    ui->nimiEdit->setText( indeksi.data(BannerModel::NimiRooli).toString());
    const QString uuid = indeksi.data(BannerModel::IdRooli).toString();
    kuva_ = model_->kuva(uuid);
    QPixmap pixmap = QPixmap::fromImage(kuva_.scaledToWidth( width() - 40, Qt::SmoothTransformation ));
    ui->kuvaLabel->setPixmap(pixmap);
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
    if( indeksi_ < 0 ) {
        model_->lisaa( ui->nimiEdit->text(), kuva_);
    } else {
        model_->muuta(indeksi_, ui->nimiEdit->text(), kuva_);
    }

    QDialog::accept();
}
