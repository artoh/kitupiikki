#include "toimistokirjanpitodialogi.h"
#include "ui_toimistokirjanpitodialogi.h"

#include "validator/ytunnusvalidator.h"
#include "groupdata.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"


#include <QPushButton>
#include <QMessageBox>

#include <QNetworkReply>
#include <QJsonDocument>

ToimistoKirjanpitoDialogi::ToimistoKirjanpitoDialogi(QWidget *parent, GroupData* group) :
    QDialog(parent),
    ui(new Ui::ToimistoKirjanpitoDialogi),
    groupData_{group}
{
    ui->setupUi(this);
    initUi();

    connect( ui->ytunnusEdit, &QLineEdit::textEdited, this, &ToimistoKirjanpitoDialogi::haeTunnarilla);
    connect( ui->nimiEdit, &QLineEdit::textEdited, this, &ToimistoKirjanpitoDialogi::tarkastaKelpo);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] {kp()->ohje("toimisto/#uuden-kirjanpidon-perustaminen");});
}

ToimistoKirjanpitoDialogi::~ToimistoKirjanpitoDialogi()
{
    delete ui;
}

void ToimistoKirjanpitoDialogi::accept()
{
    ui->buttonBox->setEnabled(false);
    kp()->odotusKursori(true);

    // Luo uuden kirjanpidon
    // groups/:id/books -rajapinnalla
    // ja lisää sen listaukseen.

    // Tietokanta alustetaan vasta, kun
    // siihen kirjaudutaan ensimmäistä kertaa
    QVariantMap payload;
    payload.insert("name", ui->nimiEdit->text());
    payload.insert("businessid", ui->ytunnusEdit->text());
    payload.insert("product", ui->tuoteList->currentItem()->data(Qt::UserRole).toInt());
    payload.insert("trial", ui->harjoitusCheck->isChecked());


    KpKysely* kysely = kp()->pilvi()->loginKysely(
        QString("/groups/%1/book").arg(groupData_->id()),
        KpKysely::POST
    );
    connect( kysely, &KpKysely::vastaus, this, &ToimistoKirjanpitoDialogi::created);
    kysely->kysy(payload);
}

void ToimistoKirjanpitoDialogi::initUi()
{
    ui->toimistoLabel->setText( groupData_->officeName() );
    ui->ryhmaLabel->setText( groupData_->name()  );
    ui->ytunnusEdit->setValidator(new YTunnusValidator());

    for(const auto& item : groupData_->products()) {
        const QVariantMap map = item.toMap();
        QListWidgetItem *lItem = new QListWidgetItem( map.value("name").toString(), ui->tuoteList);
        lItem->setData(Qt::UserRole, map.value("id").toString());
    }
    ui->tuoteList->setCurrentRow(0);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void ToimistoKirjanpitoDialogi::haeTunnarilla()
{
    if( ui->ytunnusEdit->hasAcceptableInput() ) {
        QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/" + ui->ytunnusEdit->text()));
        QNetworkReply *reply = kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &ToimistoKirjanpitoDialogi::hakuSaapuu);
    } else {
        tarkastaKelpo();
    }
}

void ToimistoKirjanpitoDialogi::hakuSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();
    if( var.toMap().value("results").toList().isEmpty())
        return;

    QVariantMap tieto = var.toMap().value("results").toList().value(0).toMap();
    ui->nimiEdit->setText( tieto.value("name").toString() );

    tarkastaKelpo();

}

void ToimistoKirjanpitoDialogi::tarkastaKelpo()
{
    bool kelpo =
            ui->ytunnusEdit->hasAcceptableInput() &&
            ui->nimiEdit->text().length() > 2;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpo);
}

void ToimistoKirjanpitoDialogi::created()
{
    kp()->odotusKursori(false);
    groupData_->reload();
    kp()->pilvi()->paivitaLista();
    QDialog::accept();
}

void ToimistoKirjanpitoDialogi::error()
{
    QMessageBox::critical(this, tr("Kirjanpidon luominen"),
                          tr("Kirjanpidon luominen epäonnistui virhetilanteen takia.\nYritä uudelleen myöhemmin."));
    kp()->odotusKursori(false);
    ui->buttonBox->setEnabled(true);
}
