#include "ryhmaoikeusdialog.h"
#include "ui_ryhmaoikeusdialog.h"

#include "ui_oikeuswidget.h"
#include "ui_toimistooikeudet.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "groupdata.h"
#include "shortcutmodel.h"

#include <QPushButton>
#include <QMessageBox>

RyhmaOikeusDialog::RyhmaOikeusDialog(QWidget *parent, GroupData *groupData) :
    QDialog(parent),
    ui(new Ui::RyhmaOikeusDialog),
    oikeusUi{new Ui::OikeusWidget},
    toimistoUi{new Ui::ToimistoOikeudet},
    group_{groupData}
{
    ui->setupUi(this);

    oikeusUi->setupUi(ui->oikeudet);
    toimistoUi->setupUi(ui->toimisto);

    ui->oikeudet->alusta();
    ui->toimisto->alusta();

    ui->alkuPvm->setNull();
    ui->loppuPvm->setNull();


    ui->ryhmaLabel->setText( group_->name() );
    ui->toimisto->nakyviin("OO", group_->isUnit());
    ui->pikaCombo->setModel(group_->shortcuts());


    connect( ui->emailEdit, &QLineEdit::textEdited, this, &RyhmaOikeusDialog::emailMuokattu);
    connect( ui->oikeudet, &OikeusWidget::muokattu, this, &RyhmaOikeusDialog::oikeusMuutos);
    connect( ui->oikeudet, &OikeusWidget::muokattu, this, &RyhmaOikeusDialog::oikeusMuutos);

    connect( ui->pikaCombo, &QComboBox::currentTextChanged, this, &RyhmaOikeusDialog::pikaMuutos);
}

RyhmaOikeusDialog::~RyhmaOikeusDialog()
{
    delete ui;
}

void RyhmaOikeusDialog::lisaaRyhmaan()
{
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

    if( bookId_ ) {
        payload.insert("book", bookId_);
    } else {        
        payload.insert("group", group_->id());
        if( !hallinta.isEmpty() ) {
            payload.insert("admin", hallinta);
        }
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

void RyhmaOikeusDialog::oikeusMuutos()
{

    int indeksi = group_->shortcuts()->indexFor( ui->oikeudet->oikeuslista(), ui->toimisto->oikeuslista() );
    ui->pikaCombo->setCurrentIndex(indeksi);

    tarkasta();
}

void RyhmaOikeusDialog::tarkasta()
{
    const bool kelpaa =
            ( userId_ || emailRe.match( ui->emailEdit->text() ).hasMatch() ) &&
            (!ui->oikeudet->oikeudet().isEmpty() || !ui->toimisto->oikeudet().isEmpty());

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpaa);
}

void RyhmaOikeusDialog::pikaMuutos()
{
    if( ui->pikaCombo->currentIndex() > 0) {
        ui->oikeudet->aseta( ui->pikaCombo->currentData(ShortcutModel::RightsRole).toStringList() );
        ui->toimisto->aseta( ui->pikaCombo->currentData(ShortcutModel::AdminRole).toStringList() );
    }
}

QRegularExpression RyhmaOikeusDialog::emailRe{R"(^.*@.*\.\w+$)"};
