#include "pikavalintadialogi.h"
#include "ui_pikavalintadialogi.h"

#include "ui_oikeuswidget.h"
#include "ui_toimistooikeudet.h"

#include "groupdata.h"
#include "shortcutmodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QPushButton>
#include <QMessageBox>

PikavalintaDialogi::PikavalintaDialogi(QWidget *parent, GroupData *groupData) :
    QDialog(parent),
    ui(new Ui::PikavalintaDialogi),
    oikeusUi{new Ui::OikeusWidget},
    toimistoUi{new Ui::ToimistoOikeudet},
    group_{groupData}
{
    ui->setupUi(this);

    oikeusUi->setupUi(ui->oikeudet);
    toimistoUi->setupUi(ui->toimisto);

    ui->oikeudet->alusta();
    ui->toimisto->alusta();

    ui->listView->setModel( group_->shortcuts() );

    ui->buttonBox->button(QDialogButtonBox::Discard)->setText( tr("Poista pikavalinnat"));

    ui->toimisto->nakyviin("OO", group_->isUnit());

    connect( ui->nimiEdit, &QLineEdit::editingFinished, this, &PikavalintaDialogi::paivitaMuokkaus);
    connect( ui->oikeudet, &OikeusWidget::muokattu, this, &PikavalintaDialogi::paivitaMuokkaus);
    connect( ui->toimisto, &OikeusWidget::muokattu, this, &PikavalintaDialogi::paivitaMuokkaus);
    connect( ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &PikavalintaDialogi::lataa);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &PikavalintaDialogi::lisaa);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &PikavalintaDialogi::poista);

    connect( ui->buttonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, this, &PikavalintaDialogi::poistaPikavalinnat);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("toimisto/pikavalinnat"); });
}

PikavalintaDialogi::~PikavalintaDialogi()
{
    delete ui;
}

void PikavalintaDialogi::lisaa()
{
    group_->shortcuts()->set(tr("Uusi pikavalinta"), QStringList(), QStringList(), -1);
}

void PikavalintaDialogi::paivitaMuokkaus()
{
    if( ui->listView->currentIndex().row() > 0) {
        group_->shortcuts()->set( ui->nimiEdit->text(), ui->oikeudet->oikeuslista(), ui->toimisto->oikeuslista(), ui->listView->currentIndex().row() );
    }
}

void PikavalintaDialogi::poista()
{
    if( ui->listView->currentIndex().row() > 0)
        group_->shortcuts()->poista(ui->listView->currentIndex().row());
}

void PikavalintaDialogi::lataa()
{
    ui->oikeudet->aseta( ui->listView->currentIndex().data(ShortcutModel::RightsRole).toStringList() );
    ui->toimisto->aseta(ui->listView->currentIndex().data(ShortcutModel::AdminRole).toStringList());
    ui->nimiEdit->setText( ui->listView->currentIndex().data(Qt::DisplayRole).toString() );

    // Muokatut oikeudet ei voi muokata
    for(QGroupBox* box : findChildren<QGroupBox*>()) {
        box->setEnabled( ui->listView->currentIndex().row() );
    }
    ui->nimiEdit->setEnabled( ui->listView->currentIndex().row());
}

void PikavalintaDialogi::poistaPikavalinnat()
{
    if( QMessageBox::question(this, tr("Pikavalinnat"), tr("Haluatko poistaa kokonaan ryhmälle määritellyt pikavalinnat?")) == QMessageBox::Yes) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely(QString("/groups/%1/shortcuts").arg(group_->id()), KpKysely::DELETE);
        connect( kysymys, &KpKysely::vastaus, this, &PikavalintaDialogi::tallennettu);
        kysymys->kysy();
    }

}

void PikavalintaDialogi::accept()
{
    ShortcutModel* model = group_->shortcuts();

    QVariantList payload;
    for(int i=1; i < model->rowCount(); i++) {
        QVariantMap scut;
        const QModelIndex& index = model->index(i,0);
        scut.insert("name", index.data(Qt::DisplayRole).toString());
        scut.insert("rights", index.data(ShortcutModel::RightsRole).toStringList());
        scut.insert("admin", index.data(ShortcutModel::AdminRole).toStringList());
        payload.append(scut);
    }

    KpKysely* kysymys = kp()->pilvi()->loginKysely(QString("/groups/%1/shortcuts").arg(group_->id()), KpKysely::PUT);
    connect( kysymys, &KpKysely::vastaus, this, &PikavalintaDialogi::tallennettu);

    kysymys->kysy(payload);

}

void PikavalintaDialogi::reject()
{
    group_->reload();
    QDialog::reject();
}

void PikavalintaDialogi::tallennettu()
{
    group_->reload();
    QDialog::accept();
}
