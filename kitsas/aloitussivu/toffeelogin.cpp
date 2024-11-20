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
    connect( loginService, &LoginService::logged, this, &ToffeeLogin::logged);

    loginService->registerWidgets( ui->emailEdit,
                                   ui->salaEdit,
                                   ui->infoLabel,
                                   ui->muistaBox,
                                   ui->loginNappi,
                                   ui->unohdusNappi,
                                   ui->salaLabel);

}

ToffeeLogin::~ToffeeLogin()
{
    delete ui;
}

int ToffeeLogin::exec()
{
    return QDialog::exec();
}

void ToffeeLogin::reject()
{
    qApp->exit(0);
}

int ToffeeLogin::keyExec()
{
    loginService->keyLogin();
    return exec();
}

void ToffeeLogin::vaihdaKieli()
{
    if( ui->kieliCombo->currentIndex() == SV)
        Kielet::instanssi()->valitseUiKieli("sv");
    else if( ui->kieliCombo->currentIndex() == EN)
        Kielet::instanssi()->valitseUiKieli("en");
    else
        Kielet::instanssi()->valitseUiKieli("fi");
    ui->retranslateUi(this);
    ui->versioLabel->setText( QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion() ));
}

void ToffeeLogin::logged(PilviKayttaja kayttaja)
{
    if( kayttaja ) {
        QDialog::accept();
    }
}


