#include "toimistosivu.h"
#include "grouptreemodel.h"
#include "groupdata.h"
#include "uusitoimistodialog.h"
#include "bookdata.h"

#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QImage>

#include "ui_toimisto.h"
#include "uusikirjanpito/uusivelho.h"
#include "ryhmaoikeusdialog.h"

#include "authlogmodel.h"
#include "groupbooksmodel.h"
#include "groupmembersmodel.h"

#include "pikavalintadialogi.h"

#include <QAction>
#include <QMenu>

ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto()),
    groupTree_(new GroupTreeModel(this)),
    groupData_{new GroupData(this)},
    bookData_{new BookData(this)}
{
    ui->setupUi(this);    
    pikavalinnatAktio_ = new QAction(QIcon(":/pic/ratas.png"), tr("Pikavalinnat..."), this);

    muokkaaRyhmaAktio_ = new QAction(QIcon(":/pic/muokkaa.png"),tr("Muokaa..."), this);
    poistaRyhmaAktio_ = new QAction(QIcon(":/pic/roskis.png"), tr("Poista..."), this);


    vaihdaTilausAktio_ = new QAction(QIcon(":/freeicons/timantti.svg"),tr("Vaihda tuotetta..."), this);
    siirraKirjaAktio_ = new QAction(QIcon(":/pic/siirra.png"),tr("Siirrä..."), this);
    poistaKirjaAktio_ = new QAction(QIcon(":/pic/roskis.png"),tr("Poista..."), this);
    tukiKirjautumisAktio_ = new QAction(QIcon(":/freeicons/adminkey.png"),tr("Tukikirjautuminen..."), this);



    bookData_->setShortcuts(groupData_->shortcuts());

    QSortFilterProxyModel *treeSort = new QSortFilterProxyModel(this);
    treeSort->setSourceModel(groupTree_);
    ui->treeView->setModel(treeSort);
    ui->treeView->setSortingEnabled(true);

    QSortFilterProxyModel *bookSort = new QSortFilterProxyModel(this);
    bookSort->setSourceModel(groupData_->books());
    ui->groupBooksView->setModel(bookSort);
    ui->groupBooksView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    QSortFilterProxyModel *memberSort = new QSortFilterProxyModel(this);
    memberSort->setSourceModel(groupData_->members());
    ui->groupMembersView->setModel(memberSort);
    ui->groupMembersView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    QSortFilterProxyModel* duSort = new QSortFilterProxyModel(this);
    duSort->setSourceModel( bookData_->directUsers() );
    ui->bKayttajatView->setModel(duSort);
    ui->bKayttajatView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    QSortFilterProxyModel* guSort = new QSortFilterProxyModel(this);
    guSort->setSourceModel( bookData_->groupUsers() );
    ui->bRyhmaView->setModel(guSort);
    ui->bRyhmaView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->bLokiView->setModel( bookData_->authLog() );
    ui->bLokiView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect( ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &ToimistoSivu::nodeValittu);

    connect( ui->groupBooksView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &ToimistoSivu::kirjaValittu);

    connect( ui->groupMembersView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &ToimistoSivu::kayttajaValittu);

    connect( ui->bKayttajatView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &ToimistoSivu::kirjanKayttajaValittu);

    connect( groupData_, &GroupData::loaded, this, &ToimistoSivu::toimistoVaihtui);
    connect( groupTree_, &GroupTreeModel::modelReset, this, [treeSort] { treeSort->sort(0); });

    connect( bookData_, &BookData::loaded, this, &ToimistoSivu::kirjaVaihtui);
    connect( groupData_->books(), &GroupBooksModel::modelReset, this, [bookSort] { bookSort->sort(0);});

    connect( groupData_->members(), &GroupMembersModel::modelReset, this, [memberSort] { memberSort->sort(0);});

    connect( bookData_->directUsers(), &GroupMembersModel::modelReset, this, [duSort] { duSort->sort(0); });

    connect( bookData_->groupUsers(), &GroupMembersModel::modelReset, this, [guSort] { guSort->sort(0);});

    connect( ui->uusiRyhmaNappi, &QPushButton::clicked, this, &ToimistoSivu::lisaaRyhma);
    connect( ui->uusiToimistoNappi, &QPushButton::clicked, this, &ToimistoSivu::lisaaToimisto);

    connect( ui->uusiKayttajaNappi, &QPushButton::clicked, this, &ToimistoSivu::uusiKayttajaRyhmaan);
    connect( ui->uMuokkaaNappi, &QPushButton::clicked, this, &ToimistoSivu::muokkaaRyhmaOikeuksia);
    connect( ui->uPoistaNappi, &QPushButton::clicked, this, [this] { if(this->groupData_) this->groupData_->deleteMembership(this->userInfo_.userid()); });

    connect( ui->uusiKirjanpitoNappi, &QPushButton::clicked, this, &ToimistoSivu::uusiKirjanpito);

    connect( ui->bAvaaNappi, &QPushButton::clicked, bookData_, &BookData::openBook);
    connect( ui->pUusiNappi, &QPushButton::clicked, this, &ToimistoSivu::lisaaOikeus);
    connect( ui->pMuokkaaNappi, &QPushButton::clicked, this, &ToimistoSivu::muokkaaOikeus);
    connect( ui->pPoistaNappi, &QPushButton::clicked, this, &ToimistoSivu::poistaOikeus);


    toimistoVaihtui();

    connect( pikavalinnatAktio_, &QAction::triggered, this, &ToimistoSivu::pikavalinnat);

    QMenu* ryhmaMenu = new QMenu(this);
    ryhmaMenu->addAction(muokkaaRyhmaAktio_);
    ryhmaMenu->addAction(poistaRyhmaAktio_);
    ui->ryhmaMenuNappi->setMenu(ryhmaMenu);

    QMenu* kirjaMenu = new QMenu(this);
    kirjaMenu->addAction(vaihdaTilausAktio_);
    kirjaMenu->addAction(siirraKirjaAktio_);
    kirjaMenu->addAction(poistaKirjaAktio_);
    kirjaMenu->addAction(tukiKirjautumisAktio_);
    ui->bMenuNappi->setMenu(kirjaMenu);

}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}

void ToimistoSivu::siirrySivulle()
{
    if( ui->treeView->selectionModel()->selectedIndexes().isEmpty() ) {
        if( groupTree_->nodes() < 50) {
            ui->treeView->expandAll();
        }
        ui->treeView->selectionModel()->select( groupTree_->index(0,0), QItemSelectionModel::SelectCurrent );
        nodeValittu( groupTree_->index(0,0) );
    }
}

void ToimistoSivu::nodeValittu(const QModelIndex &index)
{
    groupData_->load( index.data(GroupTreeModel::IdRole).toInt());
}

void ToimistoSivu::kirjaValittu(const QModelIndex &index)
{
    vaihdaLohko( KIRJANPITOLOHKO );
    kirjanKayttajaValittu(QModelIndex());
    bookData_->load(index.data(GroupBooksModel::IdRooli).toInt());
}

void ToimistoSivu::kayttajaValittu(const QModelIndex &index)
{    
    vaihdaLohko( KAYTTAJALOHKO );
    userInfo_ = groupData_->members()->getMember( index.data(GroupMembersModel::IdRooli).toInt() );
    // Näytettäisiinkö tässä käyttäjä nimi, yhteystiedot ja oikeudet
    // nykyisessä kirjanpidossa?
    ui->uNimi->setText( userInfo_.name());
    ui->uInfo->setText( QString("%1\n%2").arg(userInfo_.email(), userInfo_.phone()) );
    ui->uBrowser->setHtml( userInfo_.oikeusInfo() );

    ui->uMuokkaaNappi->setEnabled( userInfo_.userid() );
    ui->uPoistaNappi->setEnabled( userInfo_.userid());
}

void ToimistoSivu::kirjanKayttajaValittu(const QModelIndex &index)
{
    ui->pMuokkaaNappi->setEnabled( index.data(GroupMembersModel::IdRooli).toInt() );
    ui->pPoistaNappi->setEnabled(index.data(GroupMembersModel::IdRooli).toInt() );
}

void ToimistoSivu::vaihdaLohko(Lohko lohko)
{
    const QStringList oikeudet = groupData_->adminRights();

    ui->subTab->setTabVisible( RYHMATAB, lohko == RYHMALOHKO );
    ui->subTab->setTabVisible( KAYTTAJATAB, lohko == KAYTTAJALOHKO );
    ui->subTab->setTabVisible( KIRJANPITO_TIEDOT, lohko == KIRJANPITOLOHKO );
    ui->subTab->setTabVisible( KIRJANPITO_SUORAT, lohko == KIRJANPITOLOHKO && oikeudet.contains("OP"));
    ui->subTab->setTabVisible( KIRJANPITO_RYHMAT, lohko == KIRJANPITOLOHKO && oikeudet.contains("OM"));
    ui->subTab->setTabVisible( KIRJANPITO_LOKI, lohko == KIRJANPITOLOHKO && oikeudet.contains("OL"));
}

void ToimistoSivu::toimistoVaihtui()
{
    vaihdaLohko( RYHMALOHKO );
    const QStringList oikeudet = groupData_->adminRights();
    ui->mainTab->setTabVisible( PAA_JASENET, oikeudet.contains("OM") );

    ui->uusiRyhmaNappi->setVisible(oikeudet.contains("OG"));
    ui->uusiRyhmaNappi->setIcon( groupData_->isUnit() ? QIcon(":/pic/folder.png") : QIcon(":/pic/kansiot.png") );
    ui->uusiToimistoNappi->setVisible( groupData_->isUnit() && oikeudet.contains("SO"));
    ui->uusiKirjanpitoNappi->setVisible( !groupData_->isUnit() && oikeudet.contains("OB")  );

    ui->toimistoNimiLabel->setText(  groupData_->name() );
    ui->toimistoYtunnusLabel->setText( groupData_->isOffice() ? groupData_->businessId() : groupData_->officeName() );
    ui->talousverkkoLabel->setVisible( groupData_->officeType() == "Talousverkko" );

    ui->ryhmaMenuNappi->setVisible( oikeudet.contains("OG"));

    poistaRyhmaAktio_->setEnabled( groupData_->books()->rowCount() == 0 &&
                                   !groupTree_->hasChildren(ui->treeView->currentIndex())  );

    kirjaVaihtui();
}

void ToimistoSivu::kirjaVaihtui()
{    
    ui->bNimi->setText( bookData_->companyName() );
    ui->bYtunnus->setText( bookData_->businessId());

    const QImage scaled = bookData_->logo().scaledToHeight(64, Qt::SmoothTransformation);
    ui->logo->setVisible(!scaled.isNull());
    ui->logo->setPixmap(QPixmap::fromImage(scaled));

    ui->bHarjoitus->setVisible( bookData_->trial() );

    ui->bLuotu->setText( bookData_->created().toString("dd.MM.yyyy") );
    ui->bMuokattu->setText( bookData_->modified().toString("dd.MM.yyyy"));
    ui->bTositteita->setText( QString("%1").arg( bookData_->documents() ) );
    ui->bKoko->setText( bookData_->prettySize() );

    ui->bTuote->setText( bookData_->planName() );

    ui->bAvaaNappi->setVisible( bookData_->loginAvailable() );

    const bool luontiOikeus = groupData_->adminRights().contains("OB");
    const bool muokkausOikeus = groupData_->adminRights().contains("OD");
    const bool tukiKirjautumisOikeus = groupData_->adminRights().contains("OS");

    ui->bMenuNappi->setVisible( bookData_->id() && (luontiOikeus || muokkausOikeus || tukiKirjautumisOikeus) );
    vaihdaTilausAktio_->setEnabled( muokkausOikeus );
    siirraKirjaAktio_->setEnabled( muokkausOikeus );
    poistaKirjaAktio_->setEnabled( muokkausOikeus );
    tukiKirjautumisAktio_->setEnabled( tukiKirjautumisOikeus );

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

void ToimistoSivu::lisaaOikeus()
{
    RyhmaOikeusDialog dlg(this, groupData_);
    dlg.lisaa(bookData_);
    bookData_->reload();
}

void ToimistoSivu::muokkaaOikeus()
{
    int userid = ui->bKayttajatView->currentIndex().data(GroupMembersModel::IdRooli).toInt();
    if( userid ) {
        const GroupMember member = bookData_->directUsers()->getMember(userid);
        RyhmaOikeusDialog dlg(this, groupData_);
        dlg.muokkaa(member, bookData_);
        bookData_->reload();
    }
}

void ToimistoSivu::poistaOikeus()
{
    int userid = ui->bKayttajatView->currentIndex().data(GroupMembersModel::IdRooli).toInt();
    if( userid) {
        bookData_->removeRights(userid);
        bookData_->reload();
    }
}

void ToimistoSivu::muokkaaRyhmaOikeuksia()
{
    if( userInfo_.userid() ) {
        RyhmaOikeusDialog dlg(this, groupData_);
        dlg.muokkaa(userInfo_);
        groupData_->reload();
    }
}

void ToimistoSivu::uusiKayttajaRyhmaan()
{
    RyhmaOikeusDialog dlg(this, groupData_);
    dlg.lisaa();
    groupData_->reload();

}

void ToimistoSivu::uusiKirjanpito()
{
    UusiVelho velho(this);
    if( velho.toimistoVelho(groupData_)) {
        QVariantMap map = velho.data();
        groupData_->addBook(map);
    }
}

void ToimistoSivu::pikavalinnat()
{
    PikavalintaDialogi dlg(this, groupData_);
    dlg.exec();
}


