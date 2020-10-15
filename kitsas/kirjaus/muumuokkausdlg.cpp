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
#include "ui_muumuokkausdlg.h"

#include "db/verotyyppimodel.h"

#include <QSortFilterProxyModel>

MuuMuokkausDlg::MuuMuokkausDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MuuMuokkausDlg)
{
    ui->setupUi(this);

    QSortFilterProxyModel* veroproxy = new QSortFilterProxyModel;
    veroproxy->setSourceModel( kp()->alvTyypit());
    veroproxy->setFilterRole(VerotyyppiModel::KoodiRooli);
    veroproxy->setFilterRegularExpression(QRegularExpression("^(900|932|[0-8].*)"));
    ui->alvlajiCombo->setModel( veroproxy );

    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KOHDENNUKSET_PROJEKTIT);

    ui->jaksoAlkaa->setNull();
    ui->jaksoLoppuu->setNull();

    ui->merkkausLabel->setVisible(kp()->kohdennukset()->merkkauksia());
    ui->merkkausCombo->setVisible(kp()->kohdennukset()->merkkauksia());

    ui->kantaCombo->addItems(QStringList() << " 24,00 %" << "14,00 %" << "10,00 %");
    ui->kantaCombo->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{1,2}(,\\d{1,2}).*"),this));

    connect( ui->pvmEdit, &KpDateEdit::dateChanged, this, &MuuMuokkausDlg::pvmMuuttui);

    connect( ui->tiliLine, &TilinvalintaLine::textChanged, this, &MuuMuokkausDlg::tiliMuuttui);

    connect( ui->kumppani, &AsiakasToimittajaValinta::valittu, this, &MuuMuokkausDlg::kumppaniMuuttui);
    connect( ui->eraCombo, &EraCombo::valittu, this, &MuuMuokkausDlg::eraMuuttui);
    connect( ui->alvlajiCombo, &QComboBox::currentTextChanged, this, &MuuMuokkausDlg::alvLajiMuuttui);
    connect( ui->perusteRadio, &QRadioButton::toggled, this, &MuuMuokkausDlg::kirjausLajiMuuttui);

    connect( ui->jaksoAlkaa, &KpDateEdit::dateChanged, this, &MuuMuokkausDlg::jaksoMuuttui);
}

MuuMuokkausDlg::~MuuMuokkausDlg()
{
    delete ui;
}

void MuuMuokkausDlg::lataa(const TositeVienti &v)
{
    ladataan_ = true;
    setWindowTitle(tr("Muokkaa vientiä"));

    if( v.contains("pvm"))
        ui->pvmEdit->setDate( v.pvm() );

    ui->kumppani->set(v.kumppaniId(), v.kumppaniNimi());

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

    bool naytavero = kp()->asetukset()->onko(AsetusModel::ALV);

    ui->line->setVisible(naytavero);
    ui->alvLabel->setVisible(naytavero);
    ui->alvlajiCombo->setVisible(naytavero);
    ui->alvGroup->setVisible(naytavero);

    if(!naytavero) {
        setAlvKoodi(0);
    } else {
        setAlvKoodi(v.alvKoodi());
        setAlvProssa( v.alvProsentti());
    }

    ui->seliteEdit->setPlainText( v.selite());

    if(!tili.onkoValidi())
        tiliMuuttui();

    alvLajiMuuttui();

    ladataan_ = false;

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
}

void MuuMuokkausDlg::tiliMuuttui()
{
    Tili tili = ui->tiliLine->valittuTili();

    bool kohdennuksia = kp()->kohdennukset()->kohdennuksia();
    ui->kohdennusLabel->setVisible( kohdennuksia && (tili.onko(TiliLaji::TULOS) || tili.onko(TiliLaji::POISTETTAVA)));
    ui->kohdennusCombo->setVisible( kohdennuksia && (tili.onko(TiliLaji::TULOS) || tili.onko(TiliLaji::POISTETTAVA)));

    ui->eraLabel->setVisible( tili.eritellaankoTase() );
    ui->eraCombo->setVisible( tili.eritellaankoTase() );

    ui->poistoLabel->setVisible( tili.onko(TiliLaji::TASAERAPOISTO));
    ui->poistoSpin->setVisible( tili.onko(TiliLaji::TASAERAPOISTO));

    ui->jaksoLabel->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoAlkaa->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoViiva->setVisible(tili.onko(TiliLaji::TULOS));
    ui->jaksoLoppuu->setVisible(tili.onko(TiliLaji::TULOS));

    ui->eraCombo->lataa( tili.numero(), ui->kumppani->id() );

    if( !ladataan_) {
        ui->poistoSpin->setValue( tili.luku("tasaerapoisto") / 12);
        setAlvKoodi( tili.luku("alvlaji") );
        setAlvProssa( tili.str("alvprosentti").toDouble());
        if( tili.luku("kohdennus"))
            ui->kohdennusCombo->valitseKohdennus(tili.luku("kohdennus"));
    }

}

void MuuMuokkausDlg::kumppaniMuuttui()
{
    if( ui->eraCombo->isVisible())
        ui->eraCombo->lataa(ui->tiliLine->valittuTilinumero(), ui->kumppani->id());
}

void MuuMuokkausDlg::eraMuuttui(int /*eraid*/, double avoinna, const QString& selite, int kumppani)
{
    if( !ui->euroEdit->asCents()) {
        ui->euroEdit->setValue(qAbs(avoinna));
        if( avoinna > 0) {
            ui->debetRadio->setChecked(true);
        } else {
            ui->kreditRadio->setChecked(true);
        }
    }
    if( ui->seliteEdit->toPlainText().isEmpty())
        ui->seliteEdit->setPlainText(selite);
    if( !ui->kumppani->id())
        ui->kumppani->set(kumppani);
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

    kirjausLajiMuuttui();


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
                                       koodi==AlvKoodi::RAKENNUSPALVELU_OSTO));
    ui->kirjaaVahennysCheck->setVisible( ui->perusteRadio->isCheckable() &&
                                        ( koodi == AlvKoodi::OSTOT_NETTO ||
                                          koodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
                                          koodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
                                          koodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                                          koodi == AlvKoodi::MAAHANTUONTI ||
                                          koodi == AlvKoodi::RAKENNUSPALVELU_OSTO));
}
