#include "toffeelogin.h"
#include "ui_toffeelogin.h"
#include "loginservice.h"
#include "kieli/kielet.h"

ToffeeLogin::ToffeeLogin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ToffeeLogin),
    loginService{new LoginService(this)}
{
    ui->setupUi(this);

    ui->versioLabel->setText( QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion() ));

    if( Kielet::instanssi()->uiKieli() == "sv")
        ui->kieliCombo->setCurrentIndex(1);
    else
        ui->kieliCombo->setCurrentIndex(0);

    connect( ui->kieliCombo, &QComboBox::currentTextChanged, this, &ToffeeLogin::vaihdaKieli);

    connect( ui->emailEdit, &QLineEdit::textEdited, loginService, &LoginService::setEmail);
    connect( ui->salaEdit, &QLineEdit::textEdited, loginService, &LoginService::setPassword);
    connect( loginService, &LoginService::passwdAllowed, ui->salaEdit, &QLineEdit::setEnabled);
    connect( loginService, &LoginService::passwdAllowed, ui->unohdusNappi, &QPushButton::setEnabled);
    connect( loginService, &LoginService::loginAllowed, ui->loginNappi, &QPushButton::setEnabled);

    connect( loginService, &LoginService::networkError, this, [this] (const QString& viesti)
        { ui->verkkoVirhe->setText(QString("<b>%1</b>").arg(viesti)); ui->verkkoVirhe->setVisible( !viesti.isEmpty() ); });
    connect( loginService, &LoginService::incorrectPassword, this, [this]
        { ui->salasanaVirhe->setVisible(true); ui->salaEdit->clear(); ui->loginNappi->setEnabled(false); });
    connect( ui->unohdusNappi, &QPushButton::clicked, loginService, &LoginService::forgetPassword);

    connect( ui->loginNappi, &QPushButton::clicked, this, [this] { this->loginService->login(ui->muistaBox->isChecked()); } );
    connect( loginService, &LoginService::logged, this, &ToffeeLogin::logged);
}

ToffeeLogin::~ToffeeLogin()
{
    delete ui;
}

int ToffeeLogin::exec()
{
    ui->unohdusNappi->setEnabled(false);
    ui->loginNappi->setEnabled(false);
    ui->salaEdit->setEnabled(false);
    ui->salasanaVirhe->hide();
    ui->verkkoVirhe->hide();

    ui->emailEdit->clear();
    ui->salaEdit->clear();

    return QDialog::exec();
}

void ToffeeLogin::vaihdaKieli()
{
    Kielet::instanssi()->valitseUiKieli( ui->kieliCombo->currentIndex() ? "sv" : "fi" );
    ui->retranslateUi(this);
    ui->versioLabel->setText( QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion() ));
}

void ToffeeLogin::logged()
{
    QDialog::accept();
}


