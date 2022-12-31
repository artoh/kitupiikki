#include "toimistokirjanpitodialogi.h"
#include "ui_toimistokirjanpitodialogi.h"

#include "validator/ytunnusvalidator.h"
#include "groupdata.h"
#include "db/kirjanpito.h"

#include <QPushButton>


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
}

ToimistoKirjanpitoDialogi::~ToimistoKirjanpitoDialogi()
{
    delete ui;
}

void ToimistoKirjanpitoDialogi::initUi()
{
    ui->toimistoLabel->setText( groupData_->officeName() );
    ui->ryhmaLabel->setText( groupData_->objectName());
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
