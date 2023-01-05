#include "varmennedialog.h"
#include "ui_varmennedialog.h"

#include <QRegularExpressionValidator>
#include <QPushButton>

#include "toimisto/groupdata.h"
#include "db/kirjanpito.h"
#include "db/asetusmodel.h"
#include "pilvi/pilvimodel.h"

VarmenneDialog::VarmenneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VarmenneDialog)
{
    ui->setupUi(this);

    ui->tunnusEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d+")));
    ui->salaEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\w+")));

    connect( ui->tunnusEdit, &QLineEdit::textChanged, this, &VarmenneDialog::check);
    connect( ui->salaEdit, &QLineEdit::textChanged, this, &VarmenneDialog::check);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

VarmenneDialog::~VarmenneDialog()
{
    delete ui;
}

bool VarmenneDialog::toimistoVarmenne(GroupData *group)
{
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("toimisto/varmenne/"); });
    if( exec() == QDialog::Accepted) {
        group->lisaaVarmenne(ui->tunnusEdit->text(), ui->salaEdit->text());
        return true;
    }
    return false;
}

int VarmenneDialog::pilviVarmenne()
{
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("alv/varmenne/"); });
    return exec();
}

QString VarmenneDialog::tunnus() const
{
    return ui->tunnusEdit->text();
}

QString VarmenneDialog::salasana() const
{
    return ui->salaEdit->text();
}

void VarmenneDialog::check()
{
    bool kelpo =
            ui->tunnusEdit->text().length() > 8 &&
            ui->salaEdit->text().length() > 8;

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpo);
}
