#include "yksityistilienpaattaja.h"
#include "db/tositetyyppimodel.h"
#include "ui_yksityistilienpaattaja.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "liite/liitteetmodel.h"
#include <QSettings>


YksityistilienPaattaja::YksityistilienPaattaja(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::YksityistilienPaattaja)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    restoreGeometry( kp()->settings()->value("YksityistilienPaattajaDialog").toByteArray() );
}

YksityistilienPaattaja::~YksityistilienPaattaja()
{
    kp()->settings()->setValue("YksityistilienPaattajaDialog", saveGeometry());
    delete ui;
}

void YksityistilienPaattaja::alusta(const Tilikausi &kausi, const QVariantMap &data)
{
    kausi_ = kausi;
    data_ = data;

    ui->browser->setHtml( selvitys().html() );
    ui->tiliCombo->suodataTyypilla("B");

    int oletusTili = kp()->asetukset()->luku("Yksityistilit/Peruspaaoma");
    if(oletusTili) ui->tiliCombo->valitseTili(oletusTili);
}

void YksityistilienPaattaja::accept()
{
    kp()->asetukset()->aseta("Yksityistilit/Peruspaaoma", ui->tiliCombo->valittuTilinumero());

    Tosite* tosite = new Tosite(this);
    tosite->asetaTyyppi(TositeTyyppi::YKSITYISTILIEN_PAATTAMINEN);
    tosite->asetaPvm( kausi_.paattyy().addDays(1) );
    tosite->asetaOtsikko( tr("Yksityistilien %1 päättäminen").arg(kausi_.kausivaliTekstina()));
    tosite->asetaSarja(kp()->tositeTyypit()->sarja( TositeTyyppi::YKSITYISTILIEN_PAATTAMINEN ) );;

    if( ui->paataTilit) {
        Euro summa;

        QMapIterator<QString, QVariant> i(data_.value("tilit").toMap());
        while( i.hasNext()) {
            i.next();
            TositeVienti tiliVienti;
            tiliVienti.setPvm( tosite->pvm() );
            tiliVienti.setTili( i.key().toInt());
            tiliVienti.setSelite( tosite->otsikko());
            Euro saldo = Euro::fromString(i.value().toString());
            tiliVienti.setDebet(saldo);
            summa += saldo;

            tosite->viennit()->lisaa(tiliVienti);
        }
        TositeVienti vastaVienti;
        vastaVienti.setPvm( tosite->pvm());
        vastaVienti.setTili( ui->tiliCombo->valittuTilinumero());
        vastaVienti.setSelite( tosite->otsikko());
        vastaVienti.setKredit(summa);
        tosite->viennit()->lisaa(vastaVienti);
    }

    if( ui->paataTulos) {
        QString selite = tr("Tilikauden %1 tuloksen päättäminen peruspääomaan").arg(kausi_.kausivaliTekstina());
        Euro tulos = data_.value("tulos").toString();

        TositeVienti tulosVienti;
        tulosVienti.setPvm( tosite->pvm() );
        tulosVienti.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::EDELLISTENTULOS).numero() );
        tulosVienti.setSelite( selite );
        tulosVienti.setDebet( tulos );
        tosite->viennit()->lisaa(tulosVienti);

        TositeVienti vastaVienti;
        vastaVienti.setPvm( tosite->pvm());
        vastaVienti.setTili( ui->tiliCombo->valittuTilinumero());
        vastaVienti.setSelite( selite);
        vastaVienti.setKredit(tulos);
        tosite->viennit()->lisaa(vastaVienti);
    }

    tosite->liitteet()->lisaa( selvitys().pdf(false, true), "yksityistilit.pdf", "yksityistilit");
    connect( tosite, &Tosite::talletettu, this, &YksityistilienPaattaja::kirjattu);
    tosite->tallenna();
}

RaportinKirjoittaja YksityistilienPaattaja::selvitys()
{
    RaportinKirjoittaja rk;
    rk.asetaKausiteksti( kausi_.kausivaliTekstina());
    rk.asetaOtsikko(tr("Yksityistilien päättäminen"));

    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();

    rk.lisaaTyhjaRivi();

    RaporttiRivi orivi;
    orivi.lihavoi(true);
    orivi.lisaa(tr("Yksityistilit tilikaudella"), 2);
    Euro summa;
    rk.lisaaRivi(orivi);

    QMapIterator<QString, QVariant> i(data_.value("tilit").toMap());
    while( i.hasNext()) {
        i.next();
        RaporttiRivi rivi;
        Tili* tili = kp()->tilit()->tili(i.key().toInt());
        rivi.lisaa(tili ? tili->nimiNumero() : i.key());
        Euro saldo = Euro::fromString(i.value().toString());
        rivi.lisaa(saldo);
        summa += saldo;
        rk.lisaaRivi(rivi);
    }

    RaporttiRivi summaRivi;
    summaRivi.viivaYlle(true);
    summaRivi.lisaa(tr("Yhteensä"), 1);
    summaRivi.lisaa(summa);
    rk.lisaaRivi(summaRivi);

    rk.lisaaTyhjaRivi();
    RaporttiRivi tulosRivi;
    tulosRivi.lihavoi();
    tulosRivi.lisaa(tr("Tilikauden tulos"));
    tulosRivi.lisaa(Euro::fromString(data_.value("tulos").toString()));
    rk.lisaaRivi(tulosRivi);

    return rk;
}

void YksityistilienPaattaja::kirjattu()
{
    QDialog::accept();
    emit tallennettu();
}
