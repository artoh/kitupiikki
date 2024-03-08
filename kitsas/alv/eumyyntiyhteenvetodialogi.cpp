#include "eumyyntiyhteenvetodialogi.h"
#include "ui_alvilmoitusdialog.h"
#include "db/kirjanpito.h"
#include "naytin/naytinikkuna.h"

#include "alvilmoitustenmodel.h"
#include "alvkaudet.h"
#include "model/tosite.h"
#include "liite/liitteetmodel.h"
#include "db/tositetyyppimodel.h"
#include "pilvi/pilvimodel.h"

#include <QRegularExpressionValidator>

#include <QDate>
#include <QPushButton>
#include <QMessageBox>

EuMyyntiYhteenvetoDialogi::EuMyyntiYhteenvetoDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)

{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("alv/ilmoitus/");});
    ui->puhelinEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\+\\d+")));
    ui->korjausLabel->hide();
    ui->korjausCombo->hide();
    ui->alarajaGroup->hide();

    QPushButton* avaa = ui->buttonBox->addButton(tr("Tulosta"), QDialogButtonBox::ApplyRole);
    avaa->setIcon(QIcon(":/pic/print.png"));
    connect( avaa, &QPushButton::clicked, [this] {NaytinIkkuna::naytaRaportti(this->rk);});

    connect( ui->yhteysEdit, &QLineEdit::textEdited, this, &EuMyyntiYhteenvetoDialogi::tarkastaKelpo);
    connect( ui->puhelinEdit, &QLineEdit::textEdited, this, &EuMyyntiYhteenvetoDialogi::tarkastaKelpo);
    connect( ui->ilmoitaGroup, &QGroupBox::toggled, this, &EuMyyntiYhteenvetoDialogi::tarkastaKelpo);
}

EuMyyntiYhteenvetoDialogi::~EuMyyntiYhteenvetoDialogi()
{
    delete ui;
}

void EuMyyntiYhteenvetoDialogi::tarkastaKelpo()
{
    bool kelpo =
        !ui->ilmoitaGroup->isChecked() ||
        ( ui->yhteysEdit->text().length() > 4 &&
         ui->puhelinEdit->text().length() > 8 );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpo);
}

void EuMyyntiYhteenvetoDialogi::accept()
{
    ui->buttonBox->setEnabled(false);
    kp()->odotusKursori(true);

    tosite_ = new Tosite(this);

    if( ui->ilmoitaGroup->isChecked())
    {
        kp()->asetukset()->aseta(AsetusModel::VeroYhteysHenkilo, ui->yhteysEdit->text());
        kp()->asetukset()->aseta(AsetusModel::VeroYhteysPuhelin, ui->puhelinEdit->text());

        ilmoita();
    } else {
        tallennaTosite();
    }

}

void EuMyyntiYhteenvetoDialogi::ilmoita()
{
    QVariantMap payload(data_);

    payload.insert("person", kp()->asetukset()->asetus(AsetusModel::VeroYhteysHenkilo));
    payload.insert("phone", kp()->asetukset()->asetus(AsetusModel::VeroYhteysPuhelin));

    QString url = QString("%1/vateu").arg( kp()->pilvi()->service("vero") );
    KpKysely* kysymys = kpk(url, KpKysely::POST);
    connect( kysymys, &KpKysely::vastaus, this, &EuMyyntiYhteenvetoDialogi::ilmoitettu);
    connect( kysymys, &KpKysely::virhe, this, [this] (int koodi, const QString& selite) {
        this->ilmoitusVirhe(QString::number(koodi), selite);
    });

    kysymys->kysy(payload);
}

void EuMyyntiYhteenvetoDialogi::ilmoitettu(QVariant *data)
{
    // Tässä tallennetaan ilmoituksen tunniste jne.
    const QVariantMap map = data->toMap();
    const QString status = map.value("status").toString();

    if( status == "OK") {
        QVariantMap alvdata;
        alvdata.insert("apiid", map.value("id").toString());
        alvdata.insert("apidate", map.value("timestamp").toString());
        tosite_->setData(Tosite::ALV, alvdata);

        rk.lisaaTyhjaRivi();
        RaporttiRivi apiInfo;
        apiInfo.lisaa(tr("Ilmoitettu rajapinnan kautta"));
        apiInfo.lisaa(map.value("timestamp").toString(), 3);
        rk.lisaaRivi(apiInfo);
        RaporttiRivi apiId;
        apiId.lisaa("");
        apiId.lisaa(map.value("id").toString(),3);
        rk.lisaaRivi(apiId);

        tallennaTosite();
    } else {
        ilmoitusVirhe(map.value("ErrorCode").toString(), map.value("ErrorText").toString());
    }
}

void EuMyyntiYhteenvetoDialogi::ilmoitusVirhe(const QString &koodi, const QString &viesti)
{
    kp()->odotusKursori(false);
    QMessageBox::critical(this, tr("Virhe ilmoittamisessa"),
                          tr("Alv-ilmoituksen toimittaminen verottajalle epäonnistui.\n") +
                          QString("%1\n(%2)").arg( viesti, koodi ) );
    ui->buttonBox->setEnabled(true);
}

void EuMyyntiYhteenvetoDialogi::tallennaTosite()
{
    tosite_->asetaPvm( loppupvm_ );
    tosite_->asetaTyyppi( TositeTyyppi::YHTEENVETOILMOITUS );
    tosite_->asetaOtsikko( tr("Yhteisömyynnin yhteenvetoilmoitus %1/%2").arg(loppupvm_.month()).arg(loppupvm_.year()) );

    tosite_->liitteet()->lisaa( rk.pdf(false, true),
                               QString("eumyynti%2_%1.pdf").arg(loppupvm_.year()).arg(loppupvm_.month()),
                               "yhteisomyynti");

    connect( tosite_, &Tosite::talletettu, this, &EuMyyntiYhteenvetoDialogi::valmis);
    tosite_->tallenna();

}

void EuMyyntiYhteenvetoDialogi::valmis()
{
    kp()->odotusKursori(false);
    emit tallennettu();
    QDialog::accept();
}

void EuMyyntiYhteenvetoDialogi::naytaIlmoitus(const QVariantMap &data)
{
    data_ = data;
    alusta();

    ui->ilmoitusBrowser->setHtml( rk.html() );

    bool ilmoitusKaytossa = kp()->alvIlmoitukset()->kaudet()->alvIlmoitusKaytossa() &&
                            !kp()->onkoHarjoitus();

    ui->ilmoitaGroup->setVisible( ilmoitusKaytossa );
    ui->ilmoitaGroup->setChecked( ilmoitusKaytossa );
    ui->ilmoitaItseLabel->setVisible( !ilmoitusKaytossa );

    ui->yhteysEdit->setText(  kp()->asetukset()->asetus(AsetusModel::VeroYhteysHenkilo) );
    ui->puhelinEdit->setText( kp()->asetukset()->asetus(AsetusModel::VeroYhteysPuhelin));
    if( ui->puhelinEdit->text().isEmpty())
        ui->puhelinEdit->setText("+358");

    tarkastaKelpo();
    show();

}

void EuMyyntiYhteenvetoDialogi::alusta()
{
    loppupvm_ = data_.value("pvm").toDate();

    ui->ylaLabel->setText(tr("Yhteisömyynti-ilmoitus annetaan kirjanpidossa olevien tietojen mukaisesti, asiakkaan tietoihin syötettyjen alv-tunnusten mukaisesti"));

    rk.asetaKausiteksti( QString("%1 / %2").arg(loppupvm_.month()).arg(loppupvm_.year()) );
    rk.asetaOtsikko(tr("YHTEISÖMYYNNIN YHTEENVETOILMOITUS"));

    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("DK12345678901234567890");
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa(tr("Asiakas"));
    otsikko.lisaa(tr("Alv-tunnus"));
    otsikko.lisaa(tr("Tavaramyynti"));
    otsikko.lisaa(tr("Palvelumyynti"));
    rk.lisaaOtsake(otsikko);

    Euro tavarasumma;
    Euro palvelusumma;

    QVariantList lista = data_.value("ilmoitus").toList();
    for(const auto& item : lista) {
        const QVariantMap map = item.toMap();
        RaporttiRivi rivi;
        rivi.lisaa(map.value("nimi").toString());
        rivi.lisaa(map.value("alvtunnus").toString());

        Euro tavara = Euro::fromString( map.value("tavara").toString() );
        Euro palvelu = Euro::fromString( map.value("palvelu").toString() );

        rivi.lisaa( tavara );
        rivi.lisaa( palvelu );

        tavarasumma += tavara;
        palvelusumma += palvelu;

        rk.lisaaRivi(rivi);
    }

    rk.lisaaTyhjaRivi();
    RaporttiRivi summaRivi;

    summaRivi.lisaa(tr("Yhteensä"), 2);
    summaRivi.lisaa( tavarasumma, true);
    summaRivi.lisaa( palvelusumma,true);
    summaRivi.viivaYlle(true);

    rk.lisaaRivi( summaRivi );

}
