#include "kaksivaihedialog.h"
#include "ui_kaksivaihedialog.h"

#include <QRegularExpressionValidator>
#include <QPushButton>

KaksivaiheDialog::KaksivaiheDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KaksivaiheDialog)
{
    ui->setupUi(this);

    ui->codeLine->setValidator(new QRegularExpressionValidator(NumeroRE__, this));
    connect( ui->codeLine, &QLineEdit::textEdited, this, &KaksivaiheDialog::edited);
}

KaksivaiheDialog::~KaksivaiheDialog()
{
    delete ui;
}


QString KaksivaiheDialog::askCode(const QString &name)
{
    ui->nimiLabel->setText(name);
    if( exec() == QDialog::Accepted) {
        return ui->codeLine->text();
    } else {
        return QString();
    }
}

QString KaksivaiheDialog::askEmailCode(const QString &email)
{
    ui->otsikkoLabel->setText( tr("Sähköpostiosoitteen vahvistaminen") );
    ui->nimiLabel->setText( email );
    ui->ohjeLabel->setText(tr("Syötä uuteen sähköpostiosoitteeseesi lähetetty kuusinumeroinen vahvistuskoodi"));
    adjustSize();

    if( exec() == QDialog::Accepted) {
        return ui->codeLine->text();
    } else {
        return QString();
    }

}

int KaksivaiheDialog::exec()
{
    return QDialog::exec();
}

void KaksivaiheDialog::edited()
{
    if( ui->codeLine->text().length() == 6) {
        accept();
    }
}

QRegularExpression KaksivaiheDialog::NumeroRE__ = QRegularExpression("[0-9]{6}");
