#include "toimistosivu.h"
#include "grouptreemodel.h"
#include "groupdata.h"
#include "uusitoimistodialog.h"

#include <QSortFilterProxyModel>
#include <QInputDialog>

#include "ui_toimisto.h"


ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto()),
    groupTree_(new GroupTreeModel(this)),
    groupData_{new GroupData(this)}
{
    ui->setupUi(this);    

    QSortFilterProxyModel *treeSort = new QSortFilterProxyModel(this);
    treeSort->setSourceModel(groupTree_);
    ui->treeView->setModel(treeSort);
    ui->treeView->setSortingEnabled(true);

    ui->groupBooksView->setModel(groupData_->books());
    ui->groupBooksView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->groupMembersView->setModel(groupData_->members());
    ui->groupMembersView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect( ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &ToimistoSivu::nodeValittu);
    connect( groupData_, &GroupData::loaded, this, &ToimistoSivu::toimistoVaihtui);
    connect( groupTree_, &GroupTreeModel::modelReset, this, [treeSort] { treeSort->sort(0); });

    connect( ui->uusiRyhmaNappi, &QPushButton::clicked, this, &ToimistoSivu::lisaaRyhma);
    connect( ui->uusiToimistoNappi, &QPushButton::clicked, this, &ToimistoSivu::lisaaToimisto);

}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}

void ToimistoSivu::siirrySivulle()
{
    groupTree_->refresh();
}

void ToimistoSivu::nodeValittu(const QModelIndex &index)
{
    groupData_->load( index.data(GroupTreeModel::IdRole).toInt());
}

void ToimistoSivu::toimistoVaihtui()
{
    const QStringList oikeudet = groupData_->adminRights();

    ui->uusiRyhmaNappi->setVisible(oikeudet.contains("OG"));
    ui->uusiRyhmaNappi->setIcon( groupData_->isUnit() ? QIcon(":/pic/folder.png") : QIcon(":/pic/kansiot.png") );
    ui->uusiToimistoNappi->setVisible( groupData_->isUnit() && oikeudet.contains("SO"));
    ui->uusiKirjanpitoNappi->setVisible( !groupData_->isUnit() && oikeudet.contains("OB")  );
}

void ToimistoSivu::lisaaRyhma()
{
    const QString nimi = QInputDialog::getText(this, tr("Uusi ryhmä"), tr("Ryhmän nimi"));
    if( !nimi.isEmpty()) {
        QVariantMap payload;
        payload.insert("name", nimi);
        groupTree_->addGroup( ui->treeView->currentIndex().data(GroupTreeModel::IdRole).toInt(), payload );
    }
}

void ToimistoSivu::lisaaToimisto()
{
    UusiToimistoDialog dlg(this);
    dlg.newOffice(groupTree_, groupData_);
}
