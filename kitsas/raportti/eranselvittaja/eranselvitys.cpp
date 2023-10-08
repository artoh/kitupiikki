#include "eranselvitys.h"

#include "eranselvitystilimodel.h"
#include "eranselvityseramodel.h"
#include "eranselvitysviennit.h"
#include "eranselvityssortfilterproxymodel.h"

#include "lisaikkuna.h"
#include "qlineedit.h"
#include "qtoolbar.h"

#include <QTableView>
#include <QHeaderView>
#include <QSplitter>
#include <QSettings>

EranSelvitys::EranSelvitys(QDate date, QWidget *parent)
    : QMainWindow{parent},
    tiliModel_{ new EranSelvitysTiliModel(date, this) },
    eraModel_{ new EranSelvitysEraModel(this)},
    viennit_{ new EranSelvitysViennit(this)},
    proxyModel_{ new EranSelvitysSortFilterProxyModel(this)},
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

    proxyModel_->setSourceModel(eraModel_);
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);

    eraView_ = new QTableView();
    eraView_->setModel(proxyModel_);
    eraView_->horizontalHeader()->setSectionResizeMode( EranSelvitysEraModel::SELITE, QHeaderView::Stretch );
    eraView_->setSelectionBehavior( QTableView::SelectionBehavior::SelectRows );
    eraView_->setSelectionMode(QTableView::SelectionMode::SingleSelection);
    eraView_->setSortingEnabled(true);

    QTableView* viennitView = new QTableView();
    viennitView->setModel( viennit_ );
    viennitView->horizontalHeader()->setSectionResizeMode( EranSelvitysViennit::SELITE, QHeaderView::Stretch);
    viennitView->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
    viennitView->setSelectionMode( QTableView::SelectionMode::SingleSelection);
    viennitView->setSortingEnabled(true);

    QSplitter* splitter = new QSplitter();
    splitter->addWidget(tiliView);
    splitter->addWidget(eraView_);
    splitter->addWidget(viennitView);

    setCentralWidget( splitter );

    connect( tiliView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::tiliValittu);
    connect( eraView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::eraValittu);
    connect( viennitView, &QTableView::clicked, this, &EranSelvitys::naytaVienti);

    connect( eraModel_, &EranSelvitysEraModel::modelReset, this, &EranSelvitys::eratLadattu);

    initToolbar();
}

EranSelvitys::~EranSelvitys()
{
    kp()->settings()->setValue("EranSelvitysIkkuna", saveGeometry());
}

void EranSelvitys::initToolbar()
{
    QToolBar* toolbar = addToolBar(tr("Tase-erien selvittely"));
    toolbar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    QAction* nollatAktio = new QAction(QIcon(":/pic/kaytossa.png"), tr("Nollatut"));
    nollatAktio->setCheckable(true);
    connect( nollatAktio, &QAction::toggled, eraModel_, &EranSelvitysEraModel::naytaNollatut);
    toolbar->addAction(nollatAktio);

    QLineEdit* suodatus = new QLineEdit();
    suodatus->setPlaceholderText(tr("Etsi..."));
    connect(suodatus, &QLineEdit::textEdited, proxyModel_, &EranSelvitysSortFilterProxyModel::setFilterFixedString);
    toolbar->addWidget(suodatus);

    toolbar->addSeparator();
    QAction* paivitaAktio = new QAction(QIcon(":/pic/refresh.png"), tr("Päivitä"));
    connect( paivitaAktio, &QAction::triggered, tiliModel_, &EranSelvitysTiliModel::refresh);
    toolbar->addAction(paivitaAktio);

    QAction* ohjeAktio = new QAction(QIcon(":/pic/ohje.png"), tr("Ohje"));
    toolbar->addAction(ohjeAktio);
}

void EranSelvitys::tiliValittu(const QItemSelection &selected)
{
    if( !selected.empty()) {
        tili_ = selected.indexes().first().data(Qt::UserRole).toInt();
        viennit_->clear();
        eraModel_->load( tili_, date_ );        
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
