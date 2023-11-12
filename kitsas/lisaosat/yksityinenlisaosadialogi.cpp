#include "yksityinenlisaosadialogi.h"
#include "ui_yksityinenlisaosadialogi.h"

#include <QRegularExpressionValidator>
#include <QDialogButtonBox>
#include <QPushButton>

YksityinenLisaosaDialogi::YksityinenLisaosaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::YksityinenLisaosaDialogi)
{
    ui->setupUi(this);
    ui->lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9A-F]{8}-[0-9A-F]{4}-[4][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}$", QRegularExpression::CaseInsensitiveOption)));

    connect( ui->lineEdit, &QLineEdit::textEdited, this, &YksityinenLisaosaDialogi::refreshOk);
    refreshOk();
}

YksityinenLisaosaDialogi::~YksityinenLisaosaDialogi()
{
    delete ui;
}

QString YksityinenLisaosaDialogi::getId(QWidget* parent)
{
    YksityinenLisaosaDialogi dlg(parent);
    if (dlg.exec() == QDialog::Accepted)
        return dlg.ui->lineEdit->text();
    else
        return QString();
}

void YksityinenLisaosaDialogi::refreshOk()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->lineEdit->hasAcceptableInput());
}
