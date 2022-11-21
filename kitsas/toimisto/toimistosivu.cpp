#include "toimistosivu.h"
#include "grouptreemodel.h"
#include "groupdata.h"

#include "ui_toimisto.h"


ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto()),
    groupTree_(new GroupTreeModel(this)),
    groupData_{new GroupData(this)}
{
    ui->setupUi(this);

    ui->treeView->setModel(groupTree_);
    ui->groupBooksView->setModel(groupData_->books());
    ui->groupBooksView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->groupMembersView->setModel(groupData_->members());
    ui->groupBooksView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect( ui->treeView, &QTreeView::clicked, this,
             [this] (const QModelIndex& index ) {
        this->groupData_->load( index.data(GroupTreeModel::IdRole).toInt() );
    });
}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}

void ToimistoSivu::siirrySivulle()
{
    groupTree_->refresh();
}
