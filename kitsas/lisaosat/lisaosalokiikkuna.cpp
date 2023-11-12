#include "lisaosalokiikkuna.h"
#include "lisaosalokimodel.h"
#include "db/kirjanpito.h"
#include <QSettings>
#include <QHeaderView>
#include <QTableView>

LisaosaLokiIkkuna::LisaosaLokiIkkuna(QWidget *parent)
    : QMainWindow{parent},
    model_{new LisaosaLokiModel{this}},
    view_{new QTableView{this}}

{
    view_->setModel(model_);
    setCentralWidget(view_);

    view_->horizontalHeader()->setStretchLastSection(true);
    view_->setSelectionMode(QTableView::SelectionMode::NoSelection);
    view_->setAlternatingRowColors(true);
    view_->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);

    setAttribute(Qt::WA_DeleteOnClose);
    connect( model_, &LisaosaLokiModel::modelReset, this, &LisaosaLokiIkkuna::refresh);

    resize(800,600);
    restoreGeometry( kp()->settings()->value("LisaosaLoki").toByteArray());
    setWindowIcon(QIcon(":/pic/palat.png"));
}

LisaosaLokiIkkuna::~LisaosaLokiIkkuna()
{
    kp()->settings()->setValue("LisaosaLoki", saveGeometry());
}

void LisaosaLokiIkkuna::lataaLoki(const QString &lisaosaId, const QString &lisaosanimi)
{
    model_->lataa(lisaosaId);
    setWindowTitle(lisaosanimi);
}

void LisaosaLokiIkkuna::refresh()
{
    view_->resizeColumnToContents(0);

}
