#include "eranselvitys.h"

#include "eranselvitystilimodel.h"
#include "eranselvityseramodel.h"
#include "eranselvitysviennit.h"
#include "eranselvityssortfilterproxymodel.h"

#include "kirjaus/kirjauswg.h"
#include "model/tositeviennit.h"
#include "model/tositevienti.h"
#include "lisaikkuna.h"

#include <QActionGroup>
#include <QBoxLayout>
#include <QLineEdit>
#include <QToolBar>


#include "db/yhteysmodel.h"
#include "eranselvityseranvalintadialog.h"
#include "raportti/raporttivalinnat.h"
#include "naytin/naytinikkuna.h"

#include <QTableView>
#include <QHeaderView>
#include <QSplitter>
#include <QSettings>
#include <QTimer>

EranSelvitys::EranSelvitys(QDate startDate, QDate endDate, QWidget *parent)
    : QMainWindow{parent},
    tiliModel_{ new EranSelvitysTiliModel(startDate, endDate, this) },
    eraModel_{ new EranSelvitysEraModel(this)},
    viennit_{ new EranSelvitysViennit(startDate, endDate, this)},
    proxyModel_{ new EranSelvitysSortFilterProxyModel(this)},
    startDate_{startDate},
    endDate_{endDate}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Tase-erien selvittely %1-%2").arg(startDate.toString("dd.MM.yyyy"), endDate.toString("dd.MM.yyyy")));
    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(close()));
    restoreGeometry(  kp()->settings()->value("EranSelvitysIkkuna").toByteArray() );

    tiliView_ = new QTableView();
    tiliView_->setModel( tiliModel_ );
    tiliView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tiliView_->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
    tiliView_->setSelectionMode(QTableView::SelectionMode::SingleSelection);

    proxyModel_->setSourceModel(eraModel_);
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel_->setSortRole(Qt::EditRole);

    eraView_ = new QTableView();
    eraView_->setModel(proxyModel_);
    eraView_->horizontalHeader()->setSectionResizeMode( EranSelvitysEraModel::SELITE, QHeaderView::Stretch );
    eraView_->setSelectionBehavior( QTableView::SelectionBehavior::SelectRows );
    eraView_->setSelectionMode(QTableView::SelectionMode::SingleSelection);
    eraView_->setSortingEnabled(true);    
    eraView_->sortByColumn(0, Qt::AscendingOrder);

    QSortFilterProxyModel* viennitProxy = new QSortFilterProxyModel(this);
    viennitProxy->setSourceModel(viennit_);
    viennitProxy->setSortRole(Qt::EditRole);
    viennitView_ = new QTableView();
    viennitView_->setModel( viennitProxy );

    viennitView_->horizontalHeader()->setSectionResizeMode( EranSelvitysViennit::SELITE, QHeaderView::Stretch);
    viennitView_->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
    viennitView_->setSelectionMode( QTableView::SelectionMode::MultiSelection);
    viennitView_->setSortingEnabled(true);
    viennitView_->sortByColumn(0, Qt::AscendingOrder);

    QSplitter* splitter = new QSplitter();
    splitter->addWidget(tiliView_);
    splitter->addWidget(eraView_);
    splitter->addWidget(viennitView_);


    connect( tiliView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::tiliValittu);
    connect( eraView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::eraValittu);

    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setCollapsible(2, false);

    setCentralWidget( splitter );
    initToolbar();
    initActions();
    initActionBar();


    connect( eraView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::paivitaNapit);
    connect( viennitView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EranSelvitys::paivitaNapit);

    connect( tiliModel_, &EranSelvitysTiliModel::modelReset, this, &EranSelvitys::tiliListaPaivitetty);
    connect( eraModel_, &EranSelvitysEraModel::modelReset, this, &EranSelvitys::eraListaPaivitetty);
    connect( viennit_, &EranSelvitysViennit::modelReset, this, &EranSelvitys::vientiListaPaivitetty);

    paivitaNapit();

    QTimer::singleShot(100, this, [this] { this->tiliView_->selectRow(0);}  );

}

EranSelvitys::~EranSelvitys()
{
    kp()->settings()->setValue("EranSelvitysIkkuna", saveGeometry());
}

void EranSelvitys::initToolbar()
{
    QToolBar* toolbar = addToolBar(tr("Tase-erien selvittely"));
    toolbar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    toolbar->setMovable(false);

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
    connect( paivitaAktio, &QAction::triggered, this, &EranSelvitys::paivita);
    toolbar->addAction(paivitaAktio);

    QAction* naytaAktio = new QAction(QIcon(":/pic/print.png"), tr("Tase-erittely"));
    connect( naytaAktio, &QAction::triggered, this, &EranSelvitys::naytaTaseErittely);
    toolbar->addAction(naytaAktio);


    connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &EranSelvitys::paivita);
}

void EranSelvitys::initActions()
{
    avaaAktio_ = new QAction(QIcon(":/pixaby/naytto.png"), tr("Näytä tosite"));
    connect( avaaAktio_, &QAction::triggered, this, &EranSelvitys::naytaVienti);

    uusiAktio_ = new QAction(QIcon(":/pic/lisaa.png"), tr("Uusi erä"));
    connect( uusiAktio_, &QAction::triggered, this, &EranSelvitys::uusiEra);

    siirtoAktio_ = new QAction(QIcon(":/pic/siirra.png"), tr("Siirrä erään"));
    connect(siirtoAktio_, &QAction::triggered, this, &EranSelvitys::siirra);

    erittelematonAktio_ = new QAction(QIcon(":/pic/keltainen.png"), tr("Erittelemättömiin"));
    connect( erittelematonAktio_, &QAction::triggered, this, &EranSelvitys::erittelemattomiin);

    nollausAktio_ = new QAction(QIcon(":/pic/sulje.png"), tr("Nollaustosite"));
    connect(nollausAktio_, &QAction::triggered, this, &EranSelvitys::nollausTosite);

    nimeaAktio_ = new QAction(QIcon(":/pixaby/rename.svg"), tr("Uudelleennimeä"));
    connect( nimeaAktio_, &QAction::triggered, this, &EranSelvitys::uudelleennimea);

}

void EranSelvitys::initActionBar()
{
    const bool muokkausOikeus = kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_MUOKKAUS);

    QToolBar* aBar = new QToolBar(tr("Toiminnot"));
    aBar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
    aBar->setIconSize(QSize(32,32));
    aBar->setMovable(false);

    aBar->addAction(avaaAktio_);

    aBar->addSeparator();

    if( muokkausOikeus ) {
        aBar->addAction(uusiAktio_);
        aBar->addAction(siirtoAktio_);
        aBar->addAction(nollausAktio_);
        aBar->addAction(erittelematonAktio_);

        aBar->addSeparator();

        aBar->addAction( nimeaAktio_);

        aBar->addSeparator();
    }

    QAction* ohjeAktio = new QAction(QIcon(":/pic/ohje.png"), tr("Ohje"));
    aBar->addAction(ohjeAktio);
    connect( ohjeAktio, &QAction::triggered, [] { kp()->ohje("raportit/taseerittely/selvittely/"); });

    addToolBar(Qt::ToolBarArea::RightToolBarArea, aBar);
}

void EranSelvitys::tiliValittu(const QItemSelection &selected)
{
    if( !selected.empty()  ) {
        const int tili = selected.indexes().first().data(Qt::UserRole).toInt();
        if( tili == tili_) return;

        tili_ = tili;
        viennit_->clear();
        eraModel_->load( tili_, endDate_ );
    }
}

void EranSelvitys::eraValittu()
{    
    const int eraId = eraView_->selectionModel()->selectedIndexes().value(0).data(Qt::UserRole).toInt();
    eraId_ = eraId;
    viennit_->load(tili_, eraId_);
}

void EranSelvitys::naytaVienti()
{
    QModelIndexList selected = viennitView_->selectionModel()->selectedRows();
    if( !selected.empty()) {
        const int tositeId = selected.first().data(Qt::UserRole).toInt();
        LisaIkkuna *ikkuna = new LisaIkkuna();
        ikkuna->naytaTosite( tositeId );
    }
}

void EranSelvitys::naytaTaseErittely()
{
    RaporttiValinnat valinnat;
    valinnat.aseta(RaporttiValinnat::AlkuPvm, startDate_);
    valinnat.aseta(RaporttiValinnat::LoppuPvm, endDate_);
    valinnat.aseta(RaporttiValinnat::ErittelemattomienViennit, true);
    valinnat.aseta(RaporttiValinnat::Tyyppi, "taseerittely");

    NaytinIkkuna::naytaRaportti(valinnat);
}

Euro EranSelvitys::nollausSumma() const
{
    QModelIndexList selected = eraView_->selectionModel()->selectedRows();
    if( selected.isEmpty()) return Euro::Zero;

    EraMap eraMap(selected.first().data(EranSelvitysEraModel::EraMapRooli).toMap());
    Euro saldo = eraMap.saldo();

    if( QString::number(tili_).startsWith("1")) saldo = Euro::Zero - saldo;

    QModelIndexList entries = viennitView_->selectionModel()->selectedRows();
    if( !entries.isEmpty()) {
        saldo = Euro::Zero;
        for(const auto& entry : entries) {
            TositeVienti vienti(entry.data(EranSelvitysViennit::VientiMapRooli).toMap());
            saldo += vienti.kreditEuro();
            saldo -= vienti.debetEuro();
        }
    }

    return saldo;
}



void EranSelvitys::nollausTosite()
{
    const Euro saldo = nollausSumma();
    if( !saldo ) return;

    LisaIkkuna* ikkuna = new LisaIkkuna();
    KirjausSivu* sivu = ikkuna->kirjaa(-1, TositeTyyppi::MUU);
    TositeViennit* viennit = sivu->kirjausWg()->tosite()->viennit();

    QModelIndexList selected = eraView_->selectionModel()->selectedRows();
    EraMap eraMap(selected.first().data(EranSelvitysEraModel::EraMapRooli).toMap());

    TositeVienti poisVienti;
    poisVienti.setPvm(endDate_);
    poisVienti.setTili( tili_ );
    poisVienti.setEra(eraMap);
    poisVienti.setDebet(saldo);
    poisVienti.setSelite( eraMap.nimi());
    poisVienti.setKumppani(eraMap.value("kumppani").toMap());
    viennit->lisaa(poisVienti);

    TositeVienti toinenVienti;
    toinenVienti.setPvm(endDate_);
    toinenVienti.setKredit(saldo);
    toinenVienti.setKumppani(eraMap.value("kumppani").toMap());
    toinenVienti.set(TositeVienti::AALV, "+-");
    viennit->lisaa(toinenVienti);

    sivu->kirjausWg()->gui()->viennitView->setCurrentIndex( viennit->index(1,1) );


}

void EranSelvitys::siirra()
{
    QModelIndexList entries = viennitView_->selectionModel()->selectedRows();
    if( entries.isEmpty()) return;

    TositeVienti eka(entries.first().data(EranSelvitysEraModel::EraMapRooli).toMap());
    const int nykyinenEra = eka.eraId();

    EranSelvitysEranValintaDialog dlg(eraModel_, this);
    const int uusiera = dlg.valitseEra(nykyinenEra);

    if( uusiera == nykyinenEra) return;

    QVariantList lista;
    for( const auto& entry : entries ) {
        TositeVienti vienti( entry.data(EranSelvitysEraModel::EraMapRooli).toMap());
        QVariantMap map;

        map.insert("vientiId", vienti.id());
        map.insert("eraId", uusiera);
        lista.append( map );
    }

    KpKysely* kysely = kpk("/erat", KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitys::paivita);
    kysely->kysy(lista);

}

void EranSelvitys::uusiEra()
{
    QModelIndexList entries = viennitView_->selectionModel()->selectedRows();
    if( entries.isEmpty()) return;

    TositeVienti eka(entries.first().data(EranSelvitysEraModel::EraMapRooli).toMap());
    const int uusiEra = eka.id();

    QVariantList lista;
    for( const auto& entry : entries ) {
        TositeVienti vienti( entry.data(EranSelvitysEraModel::EraMapRooli).toMap());
        QVariantMap map;

        map.insert("vientiId", vienti.id());
        map.insert("eraId", uusiEra);
        lista.append( map );
    }

    KpKysely* kysely = kpk("/erat", KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitys::paivita);
    kysely->kysy(lista);

}

void EranSelvitys::erittelemattomiin()
{
    QModelIndexList entries = viennitView_->selectionModel()->selectedRows();
    if( entries.isEmpty()) return;

    QVariantList lista;
    for( const auto& entry : entries ) {
        TositeVienti vienti( entry.data(EranSelvitysEraModel::EraMapRooli).toMap());
        QVariantMap map;

        map.insert("vientiId", vienti.id());
        map.insert("eraId", 0);
        lista.append( map );
    }

    KpKysely* kysely = kpk("/erat", KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitys::paivita);
    kysely->kysy(lista);
}

void EranSelvitys::uudelleennimea()
{
    QModelIndexList entries = viennitView_->selectionModel()->selectedRows();
    if( entries.isEmpty()) return;
    TositeVienti vienti( entries.first().data(EranSelvitysEraModel::EraMapRooli).toMap());

    bool ok;
    const QString selite = QInputDialog::getText(this, tr("Vaihda selite"),
                                                 tr("Viennin selite") + "\n\n" + tr("Korvaa viennin selitteen myös kirjanpitotositteella.") , QLineEdit::Normal, vienti.selite(), &ok);
    if( !ok ) return;

    QVariantList lista;
    QVariantMap map;
    map.insert("vientiId", vienti.id());
    map.insert("selite", selite);
    lista.append(map);

    KpKysely* kysely = kpk("/erat", KpKysely::PATCH);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitys::paivita);
    kysely->kysy(lista);

    tiliView_->selectRow(0);


}

void EranSelvitys::paivita()
{
    tiliModel_->refresh();
    eraModel_->refresh();
    viennit_->load(tili_, eraId_);
    paivitaNapit();
}

void EranSelvitys::paivitaNapit()
{

    QModelIndexList viennit = viennitView_->selectionModel()->selectedRows();

    valitutViennit_.clear();
    for(const auto& vienti : viennit) {
        valitutViennit_.append(vienti.data(EranSelvitysViennit::VientiIdRooli).toInt() );
    }


    avaaAktio_->setEnabled(viennit.count() == 1);

    bool kelpoViennit = !viennit.isEmpty() && tili_;
    for(const auto& vienti : viennit) {
        const QDate& vientiPvm = vienti.data(EranSelvitysViennit::PvmRooli).toDate();
        if( vientiPvm <= kp()->tilitpaatetty()) {
            kelpoViennit = false;
            break;
        }
    }   

    nollausAktio_->setEnabled(nollausSumma() && tili_);
    uusiAktio_->setEnabled(kelpoViennit);
    siirtoAktio_->setEnabled(kelpoViennit);
    erittelematonAktio_->setEnabled(kelpoViennit && eraId_);
    nimeaAktio_->setEnabled(kelpoViennit && viennit.count() == 1);
}

void EranSelvitys::tiliListaPaivitetty()
{
    for(int i=0; i < tiliView_->model()->rowCount() ; i++) {
        const QModelIndex& index = tiliView_->model()->index(i,0);        
        if( index.data(EranSelvitysTiliModel::TiliNumeroRooli).toInt() == tili_  ) {
            tiliView_->selectRow(i);
            break;
        }
    }
}

void EranSelvitys::eraListaPaivitetty()
{

    for(int i=0; i < eraView_->model()->rowCount(); i++) {
        const QModelIndex& index = eraView_->model()->index(i,0);
        const int eraId = index.data(EranSelvitysEraModel::IdRooli).toInt();
        if( eraId == eraId_) {
            eraView_->selectRow(i);
            return;
        }
    }
    if( eraModel_->rowCount()) {
        viennitView_->selectRow(0);
        eraId_ = viennitView_->model()->index(0,0).data(Qt::UserRole).toInt();
        viennit_->load(tili_, eraId_);
    }
}

void EranSelvitys::vientiListaPaivitetty()
{
    QList<int> valitut = valitutViennit_;
    for(int i=0; i < viennitView_->model()->rowCount(); i++) {
        const QModelIndex& index = viennitView_->model()->index(i,0);
        const int vientiId = index.data(EranSelvitysViennit::VientiIdRooli).toInt();
        if( valitut.contains(vientiId)) {
            viennitView_->selectRow(i);
        }
    }
    paivitaNapit();
}
