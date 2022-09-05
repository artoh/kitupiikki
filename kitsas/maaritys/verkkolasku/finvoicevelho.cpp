#include "finvoicevelho.h"
#include <QVariant>

#include "ui_finvoicevelhoalku.h"
#include "ui_finvoicevelhotiedot.h"
#include "ui_finvoicevelhoemail.h"
#include "ui_finvoicevelhovalmis.h"

#include "db/kirjanpito.h"
#include "model/toiminimimodel.h"

#include <QTimer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

FinvoiceVelho::FinvoiceVelho(QWidget *parent) :
    QWizard(parent),
    alkusivu(new Alkusivu)
{
    addPage(alkusivu);
    addPage(new TiedotSivu);
    addPage(new EmailSivu);
    addPage(new ValmisSivu);

    setPixmap(QWizard::LogoPixmap, QPixmap(":/pic/e64.png"));

    setOption(HaveHelpButton, true);
    connect(this, &FinvoiceVelho::helpRequested, [] { kp()->ohje("asetukset/verkkolasku"); });

}

void FinvoiceVelho::kitsasKaytossa(bool onko)
{
    alkusivu->kitsasKaytossa(onko);
}

FinvoiceVelho::Alkusivu::Alkusivu() :
    ui( new Ui::FinvoiceVelhoAlku)
{
    ui->setupUi(this);
    setTitle(FinvoiceVelho::tr("Ota verkkolaskutus käyttöön"));
    setSubTitle(FinvoiceVelho::tr("Verkkolaskutilin valinta"));

    registerField("kitsas", ui->kitsasRadio);
    registerField("maventa", ui->maventaRadio);
    registerField("paikallinen", ui->tiedostoRadio);
}

FinvoiceVelho::Alkusivu::~Alkusivu()
{
    delete ui;
}

void FinvoiceVelho::Alkusivu::kitsasKaytossa(bool onko)
{
    kitsasKaytossa_ = onko;
}

void FinvoiceVelho::Alkusivu::initializePage()
{
    ui->kitsasRadio->setEnabled( kitsasKaytossa_);
    ui->uusiLabel->setVisible(kitsasKaytossa_);
    ui->onjoLabel->setVisible( !kitsasKaytossa_);
    ui->kitsasRadio->setChecked( kitsasKaytossa_);
    ui->maventaRadio->setChecked( !kitsasKaytossa_ );
}

bool FinvoiceVelho::Alkusivu::validatePage()
{
    if( ui->kitsasRadio->isChecked()) {
        return true;
    }

    QWizard* velho = wizard();
    QTimer::singleShot(0, [velho] { velho->accept(); } );
    return true;
}


FinvoiceVelho::TiedotSivu::TiedotSivu() :
    ui( new Ui::FinvoiceVelhoTiedot)
{
    setTitle(FinvoiceVelho::tr("Tarkasta tiedot"));
    setSubTitle(FinvoiceVelho::tr("Jos tiedot ovat puutteelliset, korjaa oikeat tiedot ohjelman asetuksiin ennen jatkamista."));
    ui->setupUi(this);

    const ToiminimiModel* tnimi = kp()->toiminimet();
    ui->nimiLabel->setText( tnimi->tieto() );
    ui->ytnnusLabel->setText( kp()->asetukset()->asetus(AsetusModel::Ytunnus) );
    ui->osoiteLabel->setText( tnimi->tieto(ToiminimiModel::Katuosoite) + "\n" +
                              tnimi->tieto(ToiminimiModel::Postinumero) + " " +
                              tnimi->tieto(ToiminimiModel::Kaupunki));
    ui->kotipaikkaLabel->setText( kp()->asetukset()->asetus(AsetusModel::Kotipaikka) );
}

FinvoiceVelho::TiedotSivu::~TiedotSivu()
{
    delete ui;
}

FinvoiceVelho::EmailSivu::EmailSivu() :
    ui(new Ui::FinvoiceVelhoEmail)
{
    ui->setupUi(this);

    setTitle( FinvoiceVelho::tr("Sähköpostiosoitteet"));
    setSubTitle(FinvoiceVelho::tr("Anna sähköpostiosoitteet yhteydenottoa ja sähköistä allekirjoitusta varten"));

    registerField("yhteys*", ui->emailEdit);
    registerField("auth*", ui->authemail);

    ui->emailEdit->setText( kp()->toiminimet()->tieto(ToiminimiModel::Sahkoposti) );

    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
    ui->emailEdit->setValidator( new QRegularExpressionValidator(emailRe, this) );
    ui->authemail->setValidator(new QRegularExpressionValidator(emailRe, this));

}

FinvoiceVelho::EmailSivu::~EmailSivu()
{
    delete ui;
}

FinvoiceVelho::ValmisSivu::ValmisSivu() :
    ui( new Ui::FinvoiceVelhoValmis)
{
    ui->setupUi(this);

    setTitle(FinvoiceVelho::tr("Viimeistele käyttöönotto"));
    setSubTitle(FinvoiceVelho::tr("Verkkolaskutus valmis käyttöön otettavaksi"));

    registerField("postitus", ui->postiCheck);
}

FinvoiceVelho::ValmisSivu::~ValmisSivu()
{
    delete ui;
}
