#include "aliasdialog.h"
#include "ui_aliasdialog.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QRegularExpressionValidator>
#include <QPushButton>

AliasDialog::AliasDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AliasDialog)
{
    ui->setupUi(this);
    ui->kaytossaLabel->setVisible(false);

    ui->nimiEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z][A-Za-z0-9]{2,31}")));
    connect( ui->nimiEdit, &QLineEdit::textEdited, this, &AliasDialog::tarkasta);
}

AliasDialog::~AliasDialog()
{
    delete ui;
}

void AliasDialog::asetaAlias(const QString &alias)
{
    ui->nimiEdit->setText(alias);
    tarkasta();
}

void AliasDialog::tarkasta()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->kaytossaLabel->setVisible(false);
    ui->okLabel->setVisible(false);
    if( ui->nimiEdit->text().length() > 2) {
        KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/alias/%1").arg(ui->nimiEdit->text()));
        connect( kysely, &KpKysely::vastaus, this, &AliasDialog::tieto);
        kysely->kysy();
    }
}

void AliasDialog::tieto(QVariant *data)
{
    const bool kaytossa = !data->toMap().isEmpty();

    ui->kaytossaLabel->setVisible(kaytossa);
    ui->okLabel->setVisible(!kaytossa);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!kaytossa);
}

void AliasDialog::tallennettu()
{
    QDialog::accept();
}

void AliasDialog::accept()
{
    QVariantMap map;
    map.insert("alias", ui->nimiEdit->text());
    KpKysely* kysely = kp()->pilvi()->loginKysely("/extras/", KpKysely::PUT);
    connect( kysely, &KpKysely::vastaus, this, &AliasDialog::tallennettu);
    kysely->kysy(map);

}
