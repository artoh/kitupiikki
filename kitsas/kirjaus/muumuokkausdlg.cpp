/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "muumuokkausdlg.h"
#include "db/yhteysmodel.h"
#include "ui_muumuokkausdlg.h"

#include "db/verotyyppimodel.h"
#include "alv/alvilmoitustenmodel.h"

#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>

MuuMuokkausDlg::MuuMuokkausDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MuuMuokkausDlg)
{
    ui->setupUi(this);

    QSortFilterProxyModel* veroproxy = new QSortFilterProxyModel;
    veroproxy->setSourceModel( kp()->alvTyypit());
    ui->alvlajiCombo->setModel( veroproxy );

    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KOHDENNUKSET_PROJEKTIT);

    ui->jaksoAlkaa->setNull();
    ui->jaksoLoppuu->setNull();

    ui->merkkausLabel->setVisible(kp()->kohdennukset()->merkkauksia());
    ui->merkkausCombo->setVisible(kp()->kohdennukset()->merkkauksia());

    ui->kantaCombo->addItems(QStringList() << "25,50 %" << "24,00 %" << "14,00 %" << "10,00 %");
    ui->kantaCombo->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{1,2}(,\\d{1,2})\\s?%?"),this));

    connect( ui->pvmEdit, &KpDateEdit::dateChanged, this, &MuuMuokkausDlg::pvmMuuttui);

    connect( ui->tiliLine, &TilinvalintaLine::textChanged, this, &MuuMuokkausDlg::tiliMuuttui);

    connect( ui->kumppani, &AsiakasToimittajaValinta::muuttui, this, &MuuMuokkausDlg::kumppaniMuuttui);
    connect( ui->eraCombo, &EraCombo::valittu, this, &MuuMuokkausDlg::eraMuuttui);
    connect( ui->alvlajiCombo, &QComboBox::currentTextChanged, this, &MuuMuokkausDlg::alvLajiMuuttui);
    connect( ui->perusteRadio, &QRadioButton::toggled, this, &MuuMuokkausDlg::kirjausLajiMuuttui);

    connect( ui->jaksoAlkaa, &KpDateEdit::dateChanged, this, &MuuMuokkausDlg::jaksoMuuttui);
    connect( ui->euroEdit, &KpEuroEdit::textChanged, this, &MuuMuokkausDlg::tarkasta );

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    if( kp()->settings()->contains("MuuMuokkausGeo"))
        restoreGeometry(kp()->settings()->value("MuuMuokkausGeo").toByteArray());

    ui->kumppani->clear();

    new QShortcut(QKeySequence(Qt::Key_F12), this, SLOT(accept()));
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("kirjaus/muut"); } );
}

MuuMuokkausDlg::~MuuMuokkausDlg()
{
    kp()->settings()->setValue("MuuMuokkausGeo", saveGeometry());
    delete ui;
}

void MuuMuokkausDlg::uusi(const TositeVienti &v)
{
    lataa(v);

    QString aalv = kp()->settings()->value("MuuMuokkausAalv").toString();
    ui->kirjaaVeroCheck->setChecked(aalv.contains("+"));
    ui->kirjaaVahennysCheck->setChecked(aalv.contains("-"));

    setWindowTitle(tr("Uusi vienti"));
}

void MuuMuokkausDlg::muokkaa(const TositeVienti &v)
{
    lataa(v);
    setWindowTitle(tr("Muokkaa vientiä"));
}

void MuuMuokkausDlg::lataa(const TositeVienti &v)
{
    ladataan_ = true;    

    if( v.contains("pvm"))
        ui->pvmEdit->setDate( v.pvm() );

    if(v.kumppaniId() || !v.kumppaniNimi().isEmpty())
        ui->kumppani->valitse(v.kumppaniMap());


    Tili tili;
    tili = kp()->tilit()->tiliNumerolla(v.tili());

    ui->tiliLine->valitseTili(tili);

    if( v.kredit() > 1e-5 ) {
        ui->kreditRadio->setChecked(true);
        ui->euroEdit->setValue(v.kredit());
    } else {
        ui->debetRadio->setChecked(true);
        ui->euroEdit->setValue(v.debet());
    }

    bool naytavero = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);

    ui->line->setVisible(naytavero);
    ui->alvLabel->setVisible(naytavero);
    ui->alvlajiCombo->setVisible(naytavero);
    ui->alvGroup->setVisible(naytavero);

    ui->kohdennusCombo->valitseKohdennus(v.kohdennus());
    ui->merkkausCombo->setSelectedItems( v.merkkaukset() );


    if(!naytavero) {
        setAlvKoodi(0);
    } else {
        setAlvKoodi(v.alvKoodi());
        setAlvProssa( v.alvProsentti());
    }

    ui->kirjaaVeroCheck->setChecked( v.data(TositeVienti::AALV).toString().contains("+") );
    ui->kirjaaVahennysCheck->setChecked( v.data(TositeVienti::AALV).toString().contains("-"));

    ui->ostoReskontraRadio->setChecked( v.tyyppi() == TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS );
    ui->myyntiReskontraRadio->setChecked( v.tyyppi() == TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);

    ui->seliteEdit->setPlainText( v.selite());

    ui->jaksoAlkaa->setDate(v.jaksoalkaa());
    ui->jaksoLoppuu->setDate(v.jaksoloppuu());

    ui->poistoSpin->setValue( v.tasaerapoisto() / 12 );

    if(!tili.onkoValidi())
        tiliMuuttui();

    ui->eraCombo->valitse( v.era() );

    alvLajiMuuttui();

    ladataan_ = false;

}

TositeVienti MuuMuokkausDlg::vienti() const
{
    return vienti_;
}

void MuuMuokkausDlg::accept()
{

   if(!ui->tiliLine->valittuTilinumero() ||
      !ui->euroEdit->asCents() ||
      ( ui->alvlajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt() &&
       kp()->alvIlmoitukset()->onkoIlmoitettu(ui->pvmEdit->date()) &&
         !( kp()->asetukset()->onko(AsetusModel::OhitaAlvLukko) && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ALV_ILMOITUS))
       )
      )
       return;

    vienti_.setPvm( ui->pvmEdit->date());
    vienti_.setTili( ui->tiliLine->valittuTilinumero());

    double euro = ui->euroEdit->value();
    QString aalv;
    if( ui->kirjaaVeroCheck->isChecked())
        aalv = "+";
    if( ui->kirjaaVahennysCheck->isChecked())
        aalv += "-";

    if( ui->sisAlvCheck->isVisible() && ui->sisAlvCheck->isChecked()) {
        euro = euro / (( 100.0 + alvProsentti()) / 100.0);

        // Hankalien pyöristysten takia voidaan pyöristystä ohjata
        // ylös- tai alaspäin

        Euro pohja = Euro::fromDouble(euro);
        Euro vero = Euro::fromDouble( pohja.toDouble() * alvProsentti() / 100 );
        if( pohja + vero > ui->euroEdit->euro())
            aalv += "d";
        else if( pohja + vero < ui->euroEdit->euro())
            aalv += "u";
    }

    if( ui->debetRadio->isChecked())
        vienti_.setDebet( euro);
    else
        vienti_.setKredit( euro);

    if( ui->kohdennusCombo->isVisible())
        vienti_.setKohdennus( ui->kohdennusCombo->kohdennus());
    if( ui->merkkausCombo->isVisible())
        vienti_.setMerkkaukset(ui->merkkausCombo->selectedDatas());
    if(ui->eraCombo->isVisible())
        vienti_.setEra(ui->eraCombo->eraMap());
    if(ui->jaksoAlkaa->date().isValid())
        vienti_.setJaksoalkaa(ui->jaksoAlkaa->date());
    if(ui->jaksoLoppuu->date().isValid())
        vienti_.setJaksoloppuu(ui->jaksoLoppuu->date());
    if(ui->kumppani->id() || !ui->kumppani->nimi().isEmpty())
        vienti_.setKumppani(ui->kumppani->map());    
    vienti_.setSelite(ui->seliteEdit->toPlainText());

    if( ui->alvlajiCombo->isVisible()) {
        int koodi = ui->alvlajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();
        if( koodi >= AlvKoodi::MAKSETTAVAALV)
            ;
        else if( ui->veroRadio->isChecked())
            koodi += AlvKoodi::ALVKIRJAUS;
        else if( ui->vahennysRadio->isChecked())
            koodi += AlvKoodi::ALVVAHENNYS;
        else if( ui->kohdentamatonRadio->isChecked())
            koodi += AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON;

        vienti_.setAlvKoodi( koodi );
        vienti_.setAlvProsentti(alvProsentti());

    }

    if( ui->poistoSpin->isVisible())
        vienti_.setTasaerapoisto(ui->poistoSpin->value() * 12);

    if( !aalv.isEmpty())
        vienti_.set(TositeVienti::AALV, aalv);

    if( ui->reskontraGroup->isVisible()) {
        if( ui->myyntiReskontraRadio->isChecked())
            vienti_.setTyyppi(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);
        else if(ui->ostoReskontraRadio->isChecked())
            vienti_.setTyyppi(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS);
    }

    kp()->settings()->setValue("MuuMuokkausAalv", aalv);
    kp()->settings()->setValue("MuuMuokkausGeo", saveGeometry());

    QDialog::accept();
}

void MuuMuokkausDlg::setAlvProssa(double prosentti)
{
    ui->kantaCombo->setCurrentText(QString("%L1 %").arg(prosentti,0,'f',2));
}

void MuuMuokkausDlg::setAlvKoodi(int koodi)
{
    if( koodi > AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON)
        ui->kohdentamatonRadio->setChecked(true);
    else if(koodi > AlvKoodi::ALVVAHENNYS)
        ui->vahennysRadio->setChecked(true);
    else if(koodi > AlvKoodi::ALVKIRJAUS)
        ui->veroRadio->setChecked(true);
    else
        ui->perusteRadio->setChecked(true);

    ui->alvlajiCombo->setCurrentIndex( ui->alvlajiCombo->findData(koodi % 100, VerotyyppiModel::KoodiRooli) );
}

void MuuMuokkausDlg::pvmMuuttui()
{
    ui->kohdennusCombo->suodataPaivalla( ui->pvmEdit->date() );
    ui->merkkausCombo->haeMerkkaukset(ui->pvmEdit->date());

    bool alvlukko = kp()->alvIlmoitukset()->onkoIlmoitettu(ui->pvmEdit->date());
    ui->alvVaro->setVisible(alvlukko);
    ui->alvVaroLabel->setVisible(alvlukko);
    tarkasta();
}

void MuuMuokkausDlg::tiliMuuttui()
{
    Tili tili = ui->tiliLine->valittuTili();
    if( !tili.onkoValidi()) return;

    bool kohdennuksia = kp()->kohdennukset()->kohdennuksia();
    ui->kohdennusLabel->setVisible( kohdennuksia && (tili.onko(TiliLaji::TULOS) || tili.onko(TiliLaji::POISTETTAVA)));
    ui->kohdennusCombo->setVisible( kohdennuksia && (tili.onko(TiliLaji::TULOS) || tili.onko(TiliLaji::POISTETTAVA)));

    ui->eraLabel->setVisible( tili.onko(TiliLaji::TASE) );
    ui->eraCombo->setVisible( tili.onko(TiliLaji::TASE) );

    ui->poistoLabel->setVisible( tili.onko(TiliLaji::TASAERAPOISTO));
    ui->poistoSpin->setVisible( tili.onko(TiliLaji::TASAERAPOISTO));

    ui->jaksoLabel->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoAlkaa->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoViiva->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoLoppuu->setVisible(tili.onko(TiliLaji::TULOS));

    ui->eraCombo->asetaTili( tili.numero(), ui->kumppani->id() );

    ui->reskontraGroup->setVisible( tili.onko(TiliLaji::TASE));

    if( !ladataan_) {
        if( !kp()->onkoAlvVelvollinen(ui->pvmEdit->date()) ) {
            setAlvKoodi( tili.alvlaji() );
            const double tilinProssa = tili.alvprosentti() == 24 ? yleinenAlv( ui->pvmEdit->date() ) / 100.0 : tili.alvprosentti();
            setAlvProssa( tilinProssa );
        } else {
            setAlvKoodi( AlvKoodi::EIALV );
        }

        ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12);

        if( tili.luku("kohdennus"))
            ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));

        if( tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA))
            ui->kreditRadio->setChecked(true);
        else
            ui->debetRadio->setChecked(true);

        if( tili.onko(TiliLaji::OSTOVELKA))
            ui->ostoReskontraRadio->setChecked(true);
        else if(tili.onko(TiliLaji::MYYNTISAATAVA))
            ui->myyntiReskontraRadio->setChecked(true);
    }
    tarkasta();

}

void MuuMuokkausDlg::kumppaniMuuttui()
{
    if( ui->eraCombo->isVisible())
        ui->eraCombo->asetaTili(ui->tiliLine->valittuTilinumero(), ui->kumppani->id());
}

void MuuMuokkausDlg::eraMuuttui(EraMap era)
{
    if( !ui->euroEdit->asCents()) {
        ui->euroEdit->setCents(qAbs(era.saldo().cents()));
        if( era.saldo().cents() > 0) {
            ui->debetRadio->setChecked(true);
        } else {
            ui->kreditRadio->setChecked(true);
        }
    }
    if( ui->seliteEdit->toPlainText().isEmpty())
        ui->seliteEdit->setPlainText(era.nimi());
    if( !ui->kumppani->id() && era.kumppaniId())
        ui->kumppani->valitse(era.kumppani());
}

void MuuMuokkausDlg::jaksoMuuttui()
{
    if( ui->jaksoAlkaa->date().isValid()) {
        ui->jaksoLoppuu->setEnabled(true);
    } else {
        ui->jaksoLoppuu->setEnabled(false);
        ui->jaksoLoppuu->setNull();
    }

}

void MuuMuokkausDlg::alvLajiMuuttui()
{
    int alvlaji = ui->alvlajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();
    bool nollalaji = ui->alvlajiCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool();

    ui->kohdentamatonRadio->setEnabled( alvlaji == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
                                        alvlaji == AlvKoodi::MAKSUPERUSTEINEN_OSTO);

    ui->alvGroup->setVisible(!nollalaji);
    ui->kantaLabel->setVisible(!nollalaji);
    ui->kantaCombo->setVisible(!nollalaji);

    if( !nollalaji && alvProsentti() < 1e-5 )
        ui->kantaCombo->setCurrentText("24,00 %");

    ui->sisAlvCheck->setVisible(!nollalaji &&
                                alvlaji != AlvKoodi::MYYNNIT_BRUTTO &&
                                alvlaji != AlvKoodi::OSTOT_BRUTTO);

    kirjausLajiMuuttui();
    tarkasta();

}

void MuuMuokkausDlg::kirjausLajiMuuttui()
{
    int koodi = ui->alvlajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();
    ui->kirjaaVeroCheck->setVisible( ui->perusteRadio->isChecked() &&
                                     ( koodi==AlvKoodi::MYYNNIT_NETTO ||
                                       koodi==AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
                                       koodi==AlvKoodi::ENNAKKOLASKU_MYYNTI ||
                                       koodi==AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                                       koodi==AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                                       koodi==AlvKoodi::MAAHANTUONTI ||
                                       koodi==AlvKoodi::RAKENNUSPALVELU_OSTO ||
                                       koodi==AlvKoodi::MAAHANTUONTI_PALVELUT));
    ui->kirjaaVahennysCheck->setVisible( ui->perusteRadio->isCheckable() &&
                                        ( koodi == AlvKoodi::OSTOT_NETTO ||
                                          koodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
                                          koodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                                          koodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                                          koodi == AlvKoodi::MAAHANTUONTI ||
                                          koodi == AlvKoodi::RAKENNUSPALVELU_OSTO ||
                                          koodi == AlvKoodi::MAAHANTUONTI_PALVELUT));
}

void MuuMuokkausDlg::tarkasta()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                ui->tiliLine->valittuTilinumero() &&
                ui->euroEdit->asCents() &&
                ( ui->alvlajiCombo->currentData(VerotyyppiModel::KoodiRooli).toInt() == 0 ||
                  !kp()->alvIlmoitukset()->onkoIlmoitettu(ui->pvmEdit->date()) ||
                  ( kp()->asetukset()->onko(AsetusModel::OhitaAlvLukko) && kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ALV_ILMOITUS) )
                )
            );
}

double MuuMuokkausDlg::alvProsentti() const
{
    bool nollalaji = ui->alvlajiCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool();
    if(nollalaji)
        return 0.0;

    QString txt = ui->kantaCombo->currentText();
    txt.replace(",",".");
    int vali = txt.indexOf(QRegularExpression("[^\\d\\.]"));
    if( vali > 0)
        txt = txt.left(vali);
    return txt.toDouble();
}
