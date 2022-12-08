#include "laskutustietodialog.h"

#include "ui_tilausyhteys.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "rekisteri/postinumerot.h"
#include "validator/ytunnusvalidator.h"

LaskutustietoDialog::LaskutustietoDialog(QWidget *parent) :
    QDialog{parent},
    ui{new Ui::TilausYhteys}
{
    QWidget* widget = new QWidget;
    ui->setupUi(widget);
    box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QVBoxLayout* leiska = new QVBoxLayout;
    leiska->addWidget(widget);
    leiska->addStretch();
    leiska->addWidget(box);

    setLayout(leiska);

    ui->ytunnusEdit->setValidator(new YTunnusValidator(false, this));


    connect( ui->emailButton, &QPushButton::toggled, this, &LaskutustietoDialog::maksutapaVaihtui);
    connect( ui->verkkolaskuButton, &QPushButton::toggled, this, &LaskutustietoDialog::maksutapaVaihtui);
    connect( ui->postinumero, &QLineEdit::textChanged, this, [this] (const QString& numero)
           { this->ui->postitoimipaikka->setText(Postinumerot::toimipaikka(numero)); } );
    connect( box, &QDialogButtonBox::accepted, this, &LaskutustietoDialog::accept );
    connect( box, &QDialogButtonBox::rejected, this, &LaskutustietoDialog::close);
    haeTiedot();
}

LaskutustietoDialog::~LaskutustietoDialog()
{
    delete ui;
}

void LaskutustietoDialog::haeTiedot()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely("/subscription");
    connect( kysymys, &KpKysely::vastaus, this, &LaskutustietoDialog::tiedotSaapuu);
    kysymys->kysy();
}

void LaskutustietoDialog::tiedotSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    QVariantMap payer = map.value("payer").toMap();

    const QString payerName = payer.value("name").toString();
    const QString email = payer.value("email").toString();

    ui->nimiEdit->setText( payerName.isEmpty() ? kp()->pilvi()->kayttaja().nimi() : payerName );
    ui->emailEdit->setText( email.isEmpty() ? kp()->pilvi()->kayttaja().email() : email);
    ui->ovtEdit->setText( payer.value("ovt").toString());
    ui->operaattoriEdit->setText( payer.value("ovt").toString());

    ui->osoiteEdit->setPlainText(payer.value("address").toString());
    ui->postinumero->setText(payer.value("postcode").toString());
    ui->postitoimipaikka->setText(payer.value("town").toString());
    ui->ytunnusEdit->setText( AsiakasToimittajaDlg::alvToY( payer.value("vatnumber").toString() ) );
    ui->puhelinEdit->setText( payer.value("phone").toString());
    ui->asviite->setText(payer.value("customref").toString());

    const QString operaattori = payer.value("operator").toString();
    ui->verkkolaskuButton->setChecked( !operaattori.isEmpty() );
    ui->emailButton->setChecked( operaattori.isEmpty());

    maksutapaVaihtui();


}

void LaskutustietoDialog::maksutapaVaihtui()
{
    ui->ovtLabel->setVisible( ui->verkkolaskuButton->isChecked()  );
    ui->ovtEdit->setVisible( ui->verkkolaskuButton->isChecked() );
    ui->operatorLabel->setVisible(ui->verkkolaskuButton->isChecked() );
    ui->operaattoriEdit->setVisible( ui->verkkolaskuButton->isChecked());
    ui->emailLabel->setVisible(ui->emailButton->isChecked() );
    ui->emailEdit->setVisible( ui->emailButton->isChecked() );
}
