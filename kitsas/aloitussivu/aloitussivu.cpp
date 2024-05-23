/*
   Copyright (C) 2017 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QFile>
#include <QStringList>
#include <QSqlQuery>
#include <QSslSocket>
#include <QNetworkRequest>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QListWidget>
#include <QMessageBox>

#include <QRegularExpression>

#include <QDialog>
#include <QDebug>


#include <QSettings>
#include <QSysInfo>

#include "pilvi/paivitysinfo.h"
#include "ui_aboutdialog.h"
#include "ui_muistiinpanot.h"

#include "aloitussivu.h"
#include "db/kirjanpito.h"
#include "versio.h"
#include "pilvi/pilvimodel.h"
#include "sqlite/sqlitemodel.h"

#include "uusikirjanpito/uusivelho.h"
#include "kitupiikkituonti/vanhatuontidlg.h"
#include "pilvi/pilveensiirto.h"

#include "kirjanpitodelegaatti.h"

#include <QJsonDocument>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "tilaus/tilauswizard.h"
#include "versio.h"

#include "luotunnusdialogi.h"
#include "salasananvaihto.h"

#include "kieli/kielet.h"

#include "loginservice.h"
#include "toffeelogin.h"

#include <QSslError>
#include <QClipboard>
#include <QSize>

AloitusSivu::AloitusSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    login_{new LoginService(this)}
{

    ui = new Ui::Aloitus;
    ui->setupUi(this);

    initLoginService();    

    connect( ui->uusiNappi, &QPushButton::clicked, this, &AloitusSivu::uusiTietokanta);
    connect( ui->avaaNappi, &QPushButton::clicked, this, &AloitusSivu::avaaTietokanta);
    connect( ui->kitupiikkituontiNappi, &QPushButton::clicked, this, &AloitusSivu::tuoKitupiikista);
    connect( ui->tietojaNappi, SIGNAL(clicked(bool)), this, SLOT(abouttiarallaa()));
    connect(ui->varmistaNappi, &QPushButton::clicked, this, &AloitusSivu::varmuuskopioi);
    connect(ui->muistiinpanotNappi, &QPushButton::clicked, this, &AloitusSivu::muistiinpanot);
    connect(ui->poistaNappi, &QPushButton::clicked, this, &AloitusSivu::poistaListalta);

    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &AloitusSivu::haeSaldot);    
    connect( ui->selain, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkki(QUrl)));

    connect( kp(), SIGNAL(tietokantaVaihtui()), this, SLOT(kirjanpitoVaihtui()));
    connect( kp()->asetukset(), &AsetusModel::asetusMuuttui, this, &AloitusSivu::kirjanpitoVaihtui);
    connect( Kielet::instanssi(), &Kielet::kieliVaihtui, this, &AloitusSivu::haeSaldot );

    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &AloitusSivu::kirjauduttu);
    connect( ui->logoutButton, &QPushButton::clicked, this, &AloitusSivu::pilviLogout );
    connect( ui->rekisteroiButton, &QPushButton::clicked, this, [] {
        LuoTunnusDialogi dialogi;
        dialogi.exec();
    });

    connect( ui->viimeisetView, &QListView::clicked,
             [] (const QModelIndex& index) { kp()->sqlite()->avaaTiedosto( index.data(SQLiteModel::PolkuRooli).toString() );} );

    connect( ui->pilviView, &QListView::clicked,
             [](const QModelIndex& index) { kp()->pilvi()->avaaPilvesta( index.data(PilviModel::IdRooli).toInt() ); } );


    connect( ui->tilausButton, &QPushButton::clicked, this,
             [] () { TilausWizard *tilaus = new TilausWizard(); tilaus->nayta(); });

    connect( ui->kopioiPilveenNappi, &QPushButton::clicked, this, &AloitusSivu::siirraPilveen);
    connect( ui->pilviPoistaButton, &QPushButton::clicked, this, &AloitusSivu::poistaPilvesta);

    connect( kp(), &Kirjanpito::logoMuuttui, this, &AloitusSivu::logoMuuttui);

    connect( ui->vaihdaSalasanaButton, &QPushButton::clicked, this, &AloitusSivu::vaihdaSalasanaUuteen);


    QSortFilterProxyModel* sqliteproxy = new QSortFilterProxyModel(this);
    sqliteproxy->setSourceModel( kp()->sqlite());
    ui->viimeisetView->setModel( sqliteproxy );
    sqliteproxy->setSortRole(Qt::DisplayRole);
    sqliteproxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    pilviProxy_ = new QSortFilterProxyModel(this);
    pilviProxy_->setSortRole(Qt::DisplayRole);
    pilviProxy_->setSortCaseSensitivity(Qt::CaseInsensitive);
    pilviProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    pilviProxy_->setFilterRole(Qt::DisplayRole);
    pilviProxy_->setSourceModel( kp()->pilvi() );

    connect( ui->pilviSuodin, &QLineEdit::textChanged, pilviProxy_, &QSortFilterProxyModel::setFilterFixedString);

    ui->pilviView->setModel( pilviProxy_ );

    ui->pilviView->setItemDelegate(new KirjanpitoDelegaatti(this, true));
    ui->vaaraSalasana->setVisible(false);
    ui->palvelinvirheLabel->setVisible(false);

    const int viimeIndeksi = kp()->settings()->value("TietokonePilviValilehti").toInt();
    if( viimeIndeksi == TIETOKONE_TAB)
        ui->tkpilviTab->setCurrentIndex(TIETOKONE_TAB);
    else
        ui->tkpilviTab->setCurrentIndex(PILVI_TAB);

    ui->inboxFrame->setVisible(false);
    ui->inboxFrame->installEventFilter(this);
    ui->outboxFrame->setVisible(false);
    ui->outboxFrame->installEventFilter(this);
    ui->tilioimattaFrame->setVisible(false);
    ui->tilioimattaFrame->installEventFilter(this);

    if( kp()->pilvi()->kayttaja()) {
        kirjauduttu( kp()->pilvi()->kayttaja() );
    } else if( kp()->settings()->contains("AuthKey")) {
        ui->pilviPino->setCurrentIndex(SISAANTULO);
        qApp->processEvents();
        QTimer::singleShot(500, login_, &LoginService::keyLogin );
    }

}

AloitusSivu::~AloitusSivu()
{
    kp()->settings()->setValue("TietokonePilviValilehti", ui->tkpilviTab->currentIndex() );
    delete ui;
}


void AloitusSivu::siirrySivulle()
{

    if( ui->tilikausiCombo->currentIndex() < 0)
        ui->tilikausiCombo->setCurrentIndex( kp()->tilikaudet()->indeksiPaivalle( QDate::currentDate() ) );
    if( ui->tilikausiCombo->currentIndex() < 0)
        ui->tilikausiCombo->setCurrentIndex( ui->tilikausiCombo->count()-1 );

    if( !sivulla )
    {
        connect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::haeSaldot);
        sivulla = true;
    }    

    // Päivitetään aloitussivua
    if( kp()->yhteysModel() )
    {
        haeSaldot();
        haeKpInfo();
    } else {
        ui->selain->paivita();
    }
    ui->muistiinpanotNappi->setEnabled( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
}


bool AloitusSivu::poistuSivulta(int /* minne */)
{
    disconnect( kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &AloitusSivu::haeSaldot);
    sivulla = false;
    return true;
}

void AloitusSivu::kirjanpitoVaihtui()
{

    bool avoinna = kp()->yhteysModel();

    ui->nimiLabel->setVisible(avoinna && !kp()->asetukset()->onko("LogossaNimi") );
    ui->tilikausiCombo->setVisible(avoinna );
    ui->logoLabel->setVisible( avoinna && !kp()->logo().isNull());
    ui->varmistaNappi->setEnabled(avoinna && qobject_cast<SQLiteModel*>(kp()->yhteysModel()) );
    ui->muistiinpanotNappi->setEnabled(avoinna);
    ui->poistaNappi->setEnabled( avoinna && qobject_cast<SQLiteModel*>(kp()->yhteysModel()));
    ui->pilviPoistaButton->setVisible( avoinna &&
                                       qobject_cast<PilviModel*>(kp()->yhteysModel()) &&
                                       kp()->pilvi()->onkoOikeutta(PilviModel::OMISTAJA) );

    if( avoinna )
    {
        // Kirjanpito avattu
        ui->nimiLabel->setText( kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi));

        ui->tilikausiCombo->setModel( kp()->tilikaudet() );
        ui->tilikausiCombo->setModelColumn( 0 );

        // Valitaan nykyinen tilikausi
        // Pohjalle kuitenkin viimeinen tilikausi, jotta joku on aina valittuna
        ui->tilikausiCombo->setCurrentIndex( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

        for(int i=0; i < kp()->tilikaudet()->rowCount(QModelIndex());i++)
        {
            Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(i);
            if( kausi.alkaa() <= kp()->paivamaara() && kausi.paattyy() >= kp()->paivamaara())
            {
                ui->tilikausiCombo->setCurrentIndex(i);
                break;
            }
        }        
    } else {
        ui->inboxFrame->hide();
        ui->outboxFrame->hide();
        ui->tilioimattaFrame->hide();        
    }

    if( !kp()->asetukset()->asetus(AsetusModel::Tilikartta).isEmpty() )
        kp()->settings()->setValue("Tilikartta", kp()->asetukset()->asetus(AsetusModel::Tilikartta));

    bool pilvessa = qobject_cast<PilviModel*>( kp()->yhteysModel() );
    bool paikallinen = qobject_cast<SQLiteModel*>(kp()->yhteysModel());
    bool procloud = pilvessa && kp()->pilvi()->pilvi().planId() >= AvattuPilvi::PROPILVI ;

    ui->paikallinenKuva->setVisible(paikallinen);
    ui->pilviKuva->setVisible( pilvessa && !procloud);
    ui->procloud->setVisible( procloud );
    ui->kopioiPilveenNappi->setVisible(paikallinen && kp()->pilvi()->kayttaja() && kp()->pilvi()->kayttaja().moodi() == PilviKayttaja::NORMAALI);    
    ui->selain->paivita();
}

void AloitusSivu::linkki(const QUrl &linkki)
{
    if( linkki.scheme() == "ohje")
    {
        kp()->ohje( linkki.path() );
    }
    else if( linkki.scheme() == "selaa")
    {
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla( ui->tilikausiCombo->currentIndex() );
        QString tiliteksti = linkki.fileName();
        emit selaus( tiliteksti.toInt(), kausi );
    }
    else if( linkki.scheme() == "ktp")
    {
        QString toiminto = linkki.path().mid(1);

        if( toiminto == "uusi")
            uusiTietokanta();
        else
            emit ktpkasky(toiminto);
    }
    else if( linkki.scheme().startsWith("http"))
    {        
        Kirjanpito::avaaUrl( linkki );
    }
    else if( linkki.scheme() == "close") {
        const QString id = linkki.path();
        if( !id.isEmpty() ) {
            kp()->pilvi()->poistaNotify(id);
            KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/notifications/%1").arg(id), KpKysely::DELETE);
            kysely->kysy();
            ui->selain->paivita();
        }
    }
}

void AloitusSivu::uusiTietokanta()
{
    UusiVelho velho( parentWidget() );
    if( velho.exec() ) {
        if( velho.field("pilveen").toBool())
            kp()->pilvi()->uusiPilvi(velho.data());
        else {
            if(kp()->sqlite()->uusiKirjanpito(velho.polku(), velho.data())) {
                kp()->sqlite()->avaaTiedosto(velho.polku());
            }
        }
    }
}

void AloitusSivu::avaaTietokanta()
{
    QString polku = QFileDialog::getOpenFileName(this, "Avaa kirjanpito",
                                                 QDir::homePath(),"Kirjanpito (*.kitsas)");
    if( !polku.isEmpty())
        kp()->sqlite()->avaaTiedosto( polku );

}

void AloitusSivu::tuoKitupiikista()
{
    VanhatuontiDlg dlg(this);
    dlg.exec();
}

void AloitusSivu::abouttiarallaa()
{
    Ui::AboutDlg aboutUi;
    QDialog aboutDlg;
    aboutUi.setupUi( &aboutDlg);
    connect( aboutUi.aboutQtNappi, &QPushButton::clicked, qApp, &QApplication::aboutQt);

    QString versioteksti = QString("<b>Versio %1</b><br>Käännetty %2")
            .arg( qApp->applicationVersion(), PaivitysInfo::buildDate().toString("dd.MM.yyyy"));

    QString kooste(KITSAS_BUILD);
    if( !kooste.isEmpty())
        versioteksti.append("<br>Kooste " + kooste);

    aboutUi.versioLabel->setText(versioteksti);    

    aboutDlg.exec();
}

void AloitusSivu::varmuuskopioi()
{
    QString tiedosto = kp()->sqlite()->tiedostopolku();
    bool onnistui = false;

    // Suljetaan tiedostoyhteys, jotta saadaan varmasti varmuuskopioitua kaikki
    const QString& asetusAvain = kp()->asetukset()->asetus(AsetusModel::UID) + "/varmistuspolku";
    const QString varmuushakemisto = kp()->settings()->value( asetusAvain, QDir::homePath()).toString();

    kp()->sqlite()->sulje();

    QFileInfo info(tiedosto);

    QString polku = QString("%1/%2-%3.kitsas")
            .arg(varmuushakemisto)
            .arg(info.baseName())
            .arg( QDate::currentDate().toString("yyMMdd"));

    QString tiedostoon = QFileDialog::getSaveFileName(this, tr("Varmuuskopioi kirjanpito"), polku, tr("Kirjanpito (*.kitsas)") );


    if( tiedostoon == tiedosto)
    {
        QMessageBox::critical(this, tr("Virhe"), tr("Tiedostoa ei saa kopioida itsensä päälle!"));
        tiedostoon.clear();
    }

    if( QFile::exists(tiedostoon))
        QFile::remove(tiedostoon);

    if( !tiedostoon.isEmpty() )
    {
        QFile kirjanpito( tiedosto);
        if( kirjanpito.copy(tiedostoon) ) {
            QMessageBox::information(this, kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi), tr("Kirjanpidon varmuuskopiointi onnistui."));
            onnistui = true;
        } else {
            QMessageBox::critical(this, tr("Virhe"), tr("Tiedoston varmuuskopiointi epäonnistui."));
        }
    }
    // Avataan tiedosto uudestaan
    kp()->sqlite()->avaaTiedosto(tiedosto);
    if( onnistui ) {
        QFileInfo varmuusinfo(tiedostoon);
        kp()->settings()->setValue( asetusAvain, varmuusinfo.absolutePath() );
        kp()->asetukset()->aseta("Varmuuskopioitu", QDateTime::currentDateTime().toString(Qt::ISODate));
    }
}

void AloitusSivu::muistiinpanot()
{
    QDialog dlg(this);
    Ui::Muistiinpanot mui;
    mui.setupUi(&dlg);

    mui.editori->setPlainText( kp()->asetukset()->asetus(AsetusModel::Muistiinpanot) );
    if( dlg.exec() == QDialog::Accepted )
        kp()->asetukset()->aseta(AsetusModel::Muistiinpanot, mui.editori->toPlainText());

    ui->selain->paivita();
}

void AloitusSivu::poistaListalta()
{

    if( QMessageBox::question(this, tr("Poista kirjanpito luettelosta"),
                              tr("Haluatko poistaa tämän kirjanpidon viimeisten kirjanpitojen luettelosta?\n"
                                 "Kirjanpitoa ei poisteta levyltä."),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    kp()->sqlite()->poistaListalta( kp()->sqlite()->tiedostopolku() );
}

void AloitusSivu::poistaPilvesta()
{
    if( !kp()->onkoHarjoitus()) {
        QMessageBox::critical(this, tr("Kirjanpidon poistaminen"),
                              tr("Turvallisuussyistä voit poistaa vain harjoittelutilassa olevan kirjanpidon.\n\n"
                                 "Poistaaksesi tämän kirjanpidon sinun pitää ensin asettaa Asetukset/Perusvalinnat-sivulla "
                                 "määritys Harjoituskirjanpito."));
        return;
    }

    if( QMessageBox::question(this, tr("Kirjanpidon poistaminen"),
                              tr("Haluatko todella poistaa tämän kirjanpidon %1 pysyvästi?").arg(kp()->asetukset()->nimi()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    kp()->pilvi()->poistaNykyinenPilvi();
}

void AloitusSivu::kpInfoSaapuu(QVariant *data)
{
    QVariantMap map = data ? data->toMap() : QVariantMap();

    ui->inboxFrame->setVisible( map.value("tyolista").toInt() );
    ui->inboxCount->setText( map.value("tyolista").toString() );

    ui->outboxFrame->setVisible( map.value("lahetettavia").toInt());
    ui->outboxCount->setText( map.value("lahetettavia").toString());

    ui->tilioimattaFrame->setVisible( map.value("tilioimatta").toInt() );
    ui->tilioimattaCount->setText( map.value("tilioimatta").toString());

    ui->selain->asetaTilioimatta( map.value("tilioimatta").toInt() );

}


bool AloitusSivu::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && target == ui->inboxFrame) {
        emit ktpkasky("inbox");
        return true;
    } else if (event->type() == QEvent::MouseButtonPress && target == ui->outboxFrame) {
        emit ktpkasky("outbox");
        return true;
    } else if( event->type() == QEvent::MouseButtonPress && target == ui->tilioimattaFrame ) {
        emit ktpkasky("huomio");
        return true;
    }
    return false;
}


void AloitusSivu::kirjauduttu(const PilviKayttaja& kayttaja)
{    
    ui->salaEdit->clear();

    if( !kayttaja.id() && kayttaja.moodi() == PilviKayttaja::NORMAALI) {
        ui->pilviPino->setCurrentIndex(KIRJAUDU);        
        return;
    }

    ui->pilviPino->setCurrentIndex(LISTA);
    ui->kayttajaLabel->setText( kayttaja.nimi() );

    ui->planLabel->setStyleSheet("");

    if( kayttaja.suljettu() ) {
        switch (kayttaja.suljettu()) {
            case PilviKayttaja::MAKSAMATON:
                ui->planLabel->setText(tr("Käyttö estetty maksamattomien tilausmaksujen takia"));
                break;
            case PilviKayttaja::EHTOJEN_VASTAINEN:
                ui->planLabel->setText(tr("Käyttö estetty käyttösääntöjen vastaisen toiminnan takia"));
                break;
            case PilviKayttaja::KAYTTAJAN_PYYNNOSTA:
                ui->planLabel->setText(tr("Käyttäjätunnus suljettu tilapäisesti käyttäjän pyynnöstä"));
                break;
        default:
            ui->planLabel->setText(tr("Käyttäjätunnus suljettu"));
        }
        ui->planLabel->setStyleSheet("color: red; font-weight: bold;");
    } else if( kayttaja.planId()) {
        ui->planLabel->setText( kayttaja.planName() );
    } else if( kayttaja.trialPeriod().isValid()) {
        ui->planLabel->setText(tr("Kokeilujakso %1 saakka").arg( kayttaja.trialPeriod().toString("dd.MM.yyyy") ));
    } else {
        ui->planLabel->clear();
    }


    bool naytaNormaalit = kayttaja.moodi() == PilviKayttaja::NORMAALI || kayttaja.planId();

    ui->tilausButton->setVisible( naytaNormaalit );
    ui->uusiNappi->setVisible( naytaNormaalit );
    ui->tkpilviTab->setTabVisible(TIETOKONE_TAB, naytaNormaalit || kp()->sqlite()->rowCount());

    ui->tilausButton->setText( kp()->pilvi()->kayttaja().planId() ? tr("Tilaukseni") : tr("Tee tilaus") );

    ui->pilviSuodin->clear();
    ui->etsiIcon->setVisible( kp()->pilvi()->rowCount() > 10 );
    ui->pilviSuodin->setVisible( kp()->pilvi()->rowCount() > 10 );

}

void AloitusSivu::loginVirhe()
{
    ui->pilviPino->setCurrentIndex(KIRJAUDU);
    ui->vaaraSalasana->setVisible(!ui->salaEdit->text().isEmpty());
    ui->salaEdit->clear();
}


void AloitusSivu::pilviLogout()
{
    ui->salaEdit->clear();
    kp()->pilvi()->kirjauduUlos();
    ui->vaaraSalasana->hide();

    // Toffee-moodissa ulos kirjauduttaessa annetaan heti uusi kirjautumisikkuna
    if( kp()->pilvi()->kayttaja().moodi() == PilviKayttaja::PRO) {
        ToffeeLogin dlg(this);
        dlg.exec();
    } else {
        ui->pilviPino->setCurrentIndex(KIRJAUDU);
    }
}

void AloitusSivu::logoMuuttui()
{
    if( kp()->logo().isNull() )
        ui->logoLabel->hide();
    else {
        double skaala = (1.0 * kp()->logo().width() ) / kp()->logo().height();
        ui->logoLabel->setPixmap( QPixmap::fromImage( kp()->logo().scaled( qRound( 64 * skaala),64,Qt::KeepAspectRatio) ) );
        ui->logoLabel->show();
    }
}

void AloitusSivu::haeSaldot()
{
    ui->selain->haSaldot( ui->tilikausiCombo->currentData(TilikausiModel::PaattyyRooli).toDate() );
}

void AloitusSivu::haeKpInfo()
{
    KpKysely* kysely = kpk("/info");
    if( kysely ) {
        connect( kysely, &KpKysely::vastaus, this, &AloitusSivu::kpInfoSaapuu);
        kysely->kysy();
    }
}

void AloitusSivu::siirraPilveen()
{
    PilveenSiirto *siirtoDlg = new PilveenSiirto();
    siirtoDlg->exec();
}


void AloitusSivu::vaihdaSalasanaUuteen()
{
    Salasananvaihto dlg(this);
    dlg.exec();
}

void AloitusSivu::initLoginService()
{
    login_->registerWidgets( ui->emailEdit, ui->salaEdit,
                             ui->palvelinvirheLabel, ui->muistaCheck,
                            ui->loginButton, ui->salasanaButton, ui->salaLabel
                            );

}

