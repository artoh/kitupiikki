#include "uusimaksudialog.h"
#include "ui_uusimaksudialog.h"
#include "validator/viitevalidator.h"

#include <QPushButton>

UusiMaksuDialog::UusiMaksuDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UusiMaksuDlg)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    ui->pvmEdit->setDateRange( QDate::currentDate(), QDate::currentDate().addDays(365) );


    connect( ui->ibanEdit, &KpIbanEdit::textChanged, this, &UusiMaksuDialog::updateBankName );
    connect( ui->ibanEdit, &KpIbanEdit::textChanged, this, &UusiMaksuDialog::validate);
    connect( ui->saajaEdit, &QLineEdit::textChanged, this, &UusiMaksuDialog::validate);
    connect( ui->pvmEdit, &KpDateEdit::textChanged, this, &UusiMaksuDialog::validate);
    connect( ui->euroEdit, &KpEuroEdit::euroMuuttui, this, &UusiMaksuDialog::validate);
    connect( ui->viiteRadio, &QRadioButton::clicked, this, &UusiMaksuDialog::validate);
    connect( ui->viestiRadio, &QRadioButton::clicked, this, &UusiMaksuDialog::validate);
    connect( ui->viiteEdit, &QLineEdit::textChanged, this, &UusiMaksuDialog::validate);
    connect( ui->viestiEdit, &QLineEdit::textChanged, this, &UusiMaksuDialog::validate);
}

UusiMaksuDialog::~UusiMaksuDialog()
{
    delete ui;
}

void UusiMaksuDialog::init(const QString &saaja, const QString &iban, const QString &viite, const Euro &summa, const QDate& pvm, const QString& laskuNumero)
{
    ui->saajaEdit->setText( saaja );
    Iban sIban(iban);
    if( sIban.isValid())
        ui->ibanEdit->setIban(sIban);
    if( !viite.isEmpty())
        ui->viiteEdit->setText(viite);
    if( summa > Euro::Zero )
        ui->euroEdit->setEuro(summa);
    if( pvm.isValid() && pvm >= QDate::currentDate() && pvm <= QDate::currentDate().addDays(365))
        ui->pvmEdit->setDate(pvm);
    ui->laskuNumeroEdit->setText(laskuNumero);
}

QVariant UusiMaksuDialog::data() const
{
    QVariantMap map;
    map.insert("iban", ui->ibanEdit->iban().valeitta());
    map.insert("nimi", ui->saajaEdit->text());
    map.insert("pvm", ui->pvmEdit->date().toString("yyyy-MM-dd"));
    map.insert("euro", ui->euroEdit->euro().toString());
    if( ui->viiteRadio->isChecked() )
        map.insert("viite", ui->viiteEdit->text().remove(empty));
    else
        map.insert("viesti", ui->viestiEdit->text().trimmed());
    if( !ui->laskuNumeroEdit->text().isEmpty())
        map.insert("laskunumero", ui->laskuNumeroEdit->text());
    return map;
}

void UusiMaksuDialog::updateBankName()
{
    ui->pankkiLabel->setText( ui->ibanEdit->iban().pankki() );
}

void UusiMaksuDialog::validate()
{
    const bool isValid =
        ui->ibanEdit->iban().isValid() &&
        !ui->saajaEdit->text().isEmpty() &&
        ui->pvmEdit->date().isValid() &&
        ui->euroEdit->euro().cents() > 0 && (
            (ui->viiteRadio->isChecked() && ViiteValidator::kelpaako(ui->viiteEdit->text())) ||
            (ui->viestiRadio->isChecked() && ui->viestiEdit->text().length() > 0)
    );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

QRegularExpression UusiMaksuDialog::empty = QRegularExpression("\\s");
