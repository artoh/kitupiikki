#include "ryhmaoikeusdialog.h"
#include "ui_ryhmaoikeusdialog.h"

#include "ui_oikeuswidget.h"
#include "ui_toimistooikeudet.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "groupdata.h"

#include <QPushButton>
#include <QMessageBox>

RyhmaOikeusDialog::RyhmaOikeusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RyhmaOikeusDialog),
    oikeusUi{new Ui::OikeusWidget},
    toimistoUi{new Ui::ToimistoOikeudet}
{
    ui->setupUi(this);

    oikeusUi->setupUi(ui->oikeudet);
    toimistoUi->setupUi(ui->toimisto);

    ui->oikeudet->alusta();
    ui->toimisto->alusta();

    ui->alkuPvm->setNull();
    ui->loppuPvm->setNull();

    connect( ui->emailEdit, &QLineEdit::textEdited, this, &RyhmaOikeusDialog::emailMuokattu);
    connect( ui->oikeudet, &OikeusWidget::muokattu, this, &RyhmaOikeusDialog::tarkasta);
    connect( ui->oikeudet, &OikeusWidget::muokattu, this, &RyhmaOikeusDialog::tarkasta);
}

RyhmaOikeusDialog::~RyhmaOikeusDialog()
{
    delete ui;
}

void RyhmaOikeusDialog::lisaaRyhmaan(GroupData *group)
{
    groupId_ = group->id();
    ui->ryhmaLabel->setText( group->name() );
    ui->toimisto->nakyviin("OO", group->isUnit());
    setWindowTitle( tr("Lisää käyttäjä") );
    exec();
}

void RyhmaOikeusDialog::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    kp()->odotusKursori(true);

    QVariantMap payload;
    if( userId_ ) {
        payload.insert("id", userId_);
    } else {
        payload.insert("email", ui->emailEdit->text());
    }

    const QStringList oikeudet = ui->oikeudet->oikeuslista();
    const QStringList hallinta = ui->toimisto->oikeuslista();

    if( !oikeudet.isEmpty()) {
        payload.insert("rights", oikeudet);
    }

    if( groupId_ ) {
        payload.insert("group", groupId_);
        if( !hallinta.isEmpty() ) {
            payload.insert("admin", hallinta);
        }
    } else {
        payload.insert("book", bookId_);
    }

    if( ui->alkuPvm->date().isValid())
        payload.insert("startdate", ui->alkuPvm->date().toString("yyyy-MM-dd"));
    if( ui->loppuPvm->date().isValid())
        payload.insert("enddate", ui->loppuPvm->date().toString("yyyy-MM-dd"));

    KpKysely* kysymys = kp()->pilvi()->loginKysely("/groups/users/", KpKysely::POST);
    connect( kysymys, &KpKysely::vastaus, this, &RyhmaOikeusDialog::tallennettu);
    connect( kysymys, &KpKysely::virhe, this, &RyhmaOikeusDialog::virhe);

    kysymys->kysy(payload);

}

void RyhmaOikeusDialog::tallennettu()
{
    kp()->odotusKursori(false);
    QDialog::accept();
}

void RyhmaOikeusDialog::virhe(int koodi)
{
    kp()->odotusKursori(false);
    QMessageBox::critical(this, tr("Oikeuksien asettaminen epäonnistui"),
                          tr("Palvelinyhteydessä tapahtui virhe %1").arg(koodi));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void RyhmaOikeusDialog::emailMuokattu()
{
    const QString osoite = ui->emailEdit->text();
    if( emailRe.match( osoite ).hasMatch()) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely(QString("/users/%1").arg( osoite ) );
        connect( kysymys, &KpKysely::vastaus, this, &RyhmaOikeusDialog::emailLoytyy);
        connect( kysymys, &KpKysely::virhe, this, &RyhmaOikeusDialog::emailEiLoydy);
        kysymys->kysy();
    }
    tarkasta();
}

void RyhmaOikeusDialog::emailLoytyy(QVariant *data)
{
    const QVariantMap map = data->toMap();
    ui->nimiEdit->setText( map.value("name").toString() );    
    ui->nimiEdit->setEnabled( false );    
}

void RyhmaOikeusDialog::emailEiLoydy(int virhe)
{
    if( virhe == 203) {
        ui->nimiEdit->setEnabled(true);
        userId_ = 0;
    }
}

void RyhmaOikeusDialog::tarkasta()
{
    const bool kelpaa =
            ( userId_ || emailRe.match( ui->emailEdit->text() ).hasMatch() ) &&
            (!ui->oikeudet->oikeudet().isEmpty() || !ui->toimisto->oikeudet().isEmpty());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpaa);
}

QRegularExpression RyhmaOikeusDialog::emailRe{R"(^.*@.*\.\w+$)"};
