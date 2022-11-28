#include "kirjansiirtodialogi.h"
#include "toimisto/groupdata.h"
#include "ui_kirjansiirtodialogi.h"

#include "groupdata.h"
#include "grouptreemodel.h"
#include "groupnode.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QPushButton>

KirjanSiirtoDialogi::KirjanSiirtoDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KirjanSiirtoDialogi)
{
    ui->setupUi(this);
}

KirjanSiirtoDialogi::~KirjanSiirtoDialogi()
{
    delete ui;
}

void KirjanSiirtoDialogi::siirra(int bookId, GroupTreeModel *tree, GroupData *data)
{
    ui->treeView->setModel(tree);
    if( tree->nodes() < 40)
        ui->treeView->expandAll();

    updateButton();
    connect( ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &KirjanSiirtoDialogi::updateButton);

    if( exec() == QDialog::Accepted) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        int group = ui->treeView->currentIndex().data(GroupTreeModel::IdRole).toInt();
        if( group ) {
            // Tehdään siirto
            KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/books/%1").arg(bookId), KpKysely::PATCH);
            QVariantMap payload;
            payload.insert("group", group);
            connect( kysymys, &KpKysely::vastaus, this, [this, data] {
                data->reload();
                this->accept();
            });
            kysymys->kysy(payload);
        }
    }
}

void KirjanSiirtoDialogi::updateButton()
{
    const int type = ui->treeView->currentIndex().data(GroupTreeModel::TypeRole).toInt();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                type == GroupNode::GROUP || type == GroupNode::OFFICE );
}

