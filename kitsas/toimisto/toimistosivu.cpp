#include "toimistosivu.h"
#include "grouptreemodel.h"

#include "ui_toimisto.h"


ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto()),
    groupTree_(new GroupTreeModel(this))
{
    ui->setupUi(this);

    ui->treeView->setModel(groupTree_);
}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}

void ToimistoSivu::siirrySivulle()
{
    groupTree_->refresh();
}
