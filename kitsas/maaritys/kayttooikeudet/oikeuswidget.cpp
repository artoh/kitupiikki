#include "oikeuswidget.h"

#include <QSet>
#include <QCheckBox>
#include <QVariant>

#include <algorithm>

OikeusWidget::OikeusWidget(QWidget *parent)
    : QWidget{parent}
{
}

void OikeusWidget::alusta()
{
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        connect( box, &QCheckBox::clicked, this, &OikeusWidget::tarkasta );
    }

}

void OikeusWidget::aseta(const QStringList &oikeus)
{
    omistaja_ = oikeus.contains("Om");

    bool onX = oikeus.contains("R");


    for( QCheckBox* box: findChildren<QCheckBox*>()) {

        QString boxoikeus = box->property("Oikeus").toString();
        box->setChecked( oikeus.contains(boxoikeus) || (onX && boxoikeus != "Ko") );
        connect( box, &QCheckBox::clicked, this, &OikeusWidget::tarkasta );

    }
    alussa_ = oikeudet();
    asetaKaytossaOlevatJaKasitteleAlioikeudet();

}

QSet<QString> OikeusWidget::oikeudet() const
{
    QSet<QString> set;
    for(const QCheckBox* box : findChildren<QCheckBox*>()) {
        if( box->isChecked() && (box->isEnabled() || box->property("Oikeus").toString() == "Ko"  )) {
            set.insert( box->property("Oikeus").toString() );
        }
    }
    return set;
}

QStringList OikeusWidget::oikeuslista() const
{
    QStringList lista = oikeudet().values();
    std::sort(lista.begin(), lista.end());

    return lista;
}



void OikeusWidget::nakyviin(const QString &oikeus, bool onkoNakyvissa)
{
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        if( box->property("Oikeus").toString() == oikeus) {
            box->setVisible(onkoNakyvissa);
        }
    }
}

void OikeusWidget::kaikki()
{
    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        box->setChecked(true);
    }
    tarkasta();

}

void OikeusWidget::eimitaan()
{
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(false);
    tarkasta();
}

bool OikeusWidget::onkoMuokattu()
{
    return oikeudet() != alussa_;
}

void OikeusWidget::tarkasta()
{
    asetaKaytossaOlevatJaKasitteleAlioikeudet();
    emit muokattu( onkoMuokattu() );
}

void OikeusWidget::asetaKaytossaOlevatJaKasitteleAlioikeudet()
{
    bool onKaikkiKO = false;
    bool onTositeoikeudet = false;
    bool onLaskuOikeudet = false;
    bool onKiertoOikeudet = false;
    bool onTyokaluOikeudet = false;
    bool onAsetusOikeudet = false;
    bool onLisaosaOikeudet = false;


    for( QCheckBox* box: findChildren<QCheckBox*>()) {

        QString boxoikeus = box->property("Oikeus").toString();

        if (box->isChecked()) {
            if (boxoikeus == "R")
                onKaikkiKO = true;
            else if (boxoikeus == "RT")
                onTositeoikeudet = true;
            else if (boxoikeus == "RL")
                onLaskuOikeudet = true;
            else if (boxoikeus == "RK")
                onKiertoOikeudet = true;
            else if (boxoikeus == "RO")
                onTyokaluOikeudet = true;
            else if (boxoikeus == "As")
                onAsetusOikeudet = true;
            else if (boxoikeus == "Ao")
                onLisaosaOikeudet = true;
        }
    }

    for( QCheckBox* box: findChildren<QCheckBox*>()) {

        QString boxoikeus = box->property("Oikeus").toString();


        if (boxoikeus == "Ko") {
            box->setDisabled(omistaja_);
        } else if (onKaikkiKO) {
            box->setEnabled(boxoikeus == "R");
            box->setChecked(true);
        } else if (boxoikeus == "Ts" || boxoikeus == "Tl" || boxoikeus == "Tt" || boxoikeus == "Tk") {
            box->setDisabled(onTositeoikeudet);
            if (onTositeoikeudet)
                box->setChecked(true);
        } else if (boxoikeus == "Ls" || boxoikeus == "Ll" || boxoikeus == "Lt" || boxoikeus == "Xt" || boxoikeus == "Xr") {
            box->setDisabled(onLaskuOikeudet);
            if (onLaskuOikeudet)
                box->setChecked(true);
        } else if (boxoikeus == "Kl" || boxoikeus == "Kt" || boxoikeus == "Kh" || boxoikeus=="Ks" || boxoikeus == "Pm" || boxoikeus == "Pl") {
            box->setDisabled(onKiertoOikeudet);
            if (onKiertoOikeudet)
                box->setChecked(true);
        } else if (boxoikeus == "Av" || boxoikeus == "Bm" || boxoikeus == "Tp") {
            box->setDisabled(onTyokaluOikeudet);
            if (onTyokaluOikeudet)
               box->setChecked(true);
        } else if (boxoikeus == "Ab" || boxoikeus == "Ao") {
            box->setDisabled(onAsetusOikeudet);
            if (onAsetusOikeudet)
                box->setChecked(true);

        } else if (boxoikeus == "Ad") {
            box->setDisabled(onAsetusOikeudet || onLisaosaOikeudet);
            if (onAsetusOikeudet || onLisaosaOikeudet)
                box->setChecked(true);
        }


        else box->setEnabled(true);

    }
}
