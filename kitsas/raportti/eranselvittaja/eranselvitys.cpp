#include "eranselvitys.h"

#include "eranselvitystilimodel.h"
#include "eranselvityseramodel.h"
#include "eranselvitysviennit.h"

#include "lisaikkuna.h"

#include <QTableView>
#include <QHeaderView>
#include <QSplitter>
#include <QSettings>

EranSelvitys::EranSelvitys(QDate date, QWidget *parent)
    : QMainWindow{parent},
    tiliModel_{ new EranSelvitysTiliModel(date, this) },
    eraModel_{ new EranSelvitysEraModel(this)},
    viennit_{ new EranSelvitysViennit(this)},
    date_{date}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Tase-erien selvittely %1").arg(date.toString("dd.MM.yyyy")));
    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(close()));
    restoreGeometry(  kp()->settings()->value("EranSelvitysIkkuna").toByteArray() );

    QTableView* tiliView = new QTableView();
    tiliView->setModel( tiliModel_ );
    tiliView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tiliView->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
    tiliView->setSelectionMode(QTableView::SelectionMode::SingleSelection);

    eraView_ = new QTableView();
    eraView_->setModel(eraModel_);
    eraView_->horizontalHeader()->setSectionResizeMode( EranSelvitysEraModel::SELITE, QHeaderView::Stretch );
    eraView_->setSelectionBehavior( QTableView::SelectionBehavior::SelectRows );
    eraView_->setSelectionMode(QTableView::SelectionMode::SingleSelection);

    QTableView* viennitView = new QTableView();
    viennitView->setModel( viennit_ );
    viennitView->horizontalHeader()->setSectionResizeMode( EranSelvitysViennit::SELITE, QHeaderView::Stretch);
    viennitView->setSelectionMode( QTableView::SelectionMode::NoSelection);

    QSplitter* splitter = new QSplitter();
    splitter->addWidget(tiliView);
    splitter->addWidget(eraView_);
    splitter->addWidget(viennitView);

    setCentralWidget( splitter );

    connect( tiliView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::tiliValittu);
    connect( eraView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::eraValittu);
    connect( viennitView, &QTableView::clicked, this, &EranSelvitys::naytaVienti);

    connect( eraModel_, &EranSelvitysEraModel::modelReset, this, &EranSelvitys::eratLadattu);
}

EranSelvitys::~EranSelvitys()
{
    kp()->settings()->setValue("EranSelvitysIkkuna", saveGeometry());
}

void EranSelvitys::tiliValittu(const QItemSelection &selected)
{
    if( !selected.empty()) {
        tili_ = selected.indexes().first().data(Qt::UserRole).toInt();
        eraModel_->load( tili_, date_ );
        viennit_->clear();
    }
}

void EranSelvitys::eraValittu(const QItemSelection &selected)
{
    if( !selected.empty()) {
        const int eraid = selected.indexes().first().data(Qt::UserRole).toInt();
        viennit_->load(tili_, eraid);
    }
}

void EranSelvitys::naytaVienti(const QModelIndex &index)
{
    if( index.isValid()) {
        const int tosite = index.data(Qt::UserRole).toInt();
        LisaIkkuna *ikkuna = new LisaIkkuna();
        ikkuna->naytaTosite( tosite );
    }
}

void EranSelvitys::eratLadattu()
{
    if( eraModel_->rowCount()) {
        eraView_->selectRow(0);
    }
}
