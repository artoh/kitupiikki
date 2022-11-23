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

ToimistoSivu::ToimistoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::Toimisto()),
    groupTree_(new GroupTreeModel(this)),
    groupData_{new GroupData(this)},
    bookData_{new BookData(this)}
{
    ui->setupUi(this);    

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

    connect( ui->groupBooksView->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &ToimistoSivu::kirjaValittu);

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

    connect( ui->uusiKirjanpitoNappi, &QPushButton::clicked, this, &ToimistoSivu::uusiKirjanpito);

    connect( ui->bAvaaNappi, &QPushButton::clicked, bookData_, &BookData::openBook);

    toimistoVaihtui();
}

ToimistoSivu::~ToimistoSivu()
{
    delete ui;
}

void ToimistoSivu::siirrySivulle()
{

}

void ToimistoSivu::nodeValittu(const QModelIndex &index)
{
    groupData_->load( index.data(GroupTreeModel::IdRole).toInt());
}

void ToimistoSivu::kirjaValittu(const QModelIndex &index)
{
    bookData_->load(index.data(GroupBooksModel::IdRooli).toInt());
}

void ToimistoSivu::toimistoVaihtui()
{
    const QStringList oikeudet = groupData_->adminRights();

    ui->uusiRyhmaNappi->setVisible(oikeudet.contains("OG"));
    ui->uusiRyhmaNappi->setIcon( groupData_->isUnit() ? QIcon(":/pic/folder.png") : QIcon(":/pic/kansiot.png") );
    ui->uusiToimistoNappi->setVisible( groupData_->isUnit() && oikeudet.contains("SO"));
    ui->uusiKirjanpitoNappi->setVisible( !groupData_->isUnit() && oikeudet.contains("OB")  );


    ui->stackedWidget->setCurrentIndex( RYHMATAB );
    ui->toimistoNimiLabel->setText(  groupData_->name() );
    ui->toimistoYtunnusLabel->setText( groupData_->isOffice() ? groupData_->businessId() : groupData_->officeName() );
    ui->talousverkkoLabel->setVisible( groupData_->officeType() == "Talousverkko" );

    ui->muokkaaToimistoNappi->setVisible( (groupData_->isOffice() && oikeudet.contains("SO")) ||
                                          (groupData_->isGroup() && oikeudet.contains("OG")) ||
                                          (groupData_->isUnit() && oikeudet.contains("SU")));

}

void ToimistoSivu::kirjaVaihtui()
{
    ui->stackedWidget->setCurrentIndex( KIRJANPITOTAB );
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

void ToimistoSivu::uusiKayttajaRyhmaan()
{
    RyhmaOikeusDialog dlg(this);
    dlg.lisaaRyhmaan( groupData_ );
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


