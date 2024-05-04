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
    asetaKaytossaOlevat();

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
    bool onX = false;
    for(const QCheckBox* box : findChildren<QCheckBox*>()) {
        if( box->property("Oikeus").toString() == "R" && box->isChecked()  ) {
          onX = true;
        }
    }

    if (onX) {
    for(QCheckBox* box : findChildren<QCheckBox*>()) {
        if(box->property("Oikeus").toString() != "Ko"  ) {
            box->setChecked(true) ;
        }
    }
    }
    asetaKaytossaOlevat();
    emit muokattu( onkoMuokattu() );
}

void OikeusWidget::asetaKaytossaOlevat()
{
    bool onX = false;

    for( QCheckBox* box: findChildren<QCheckBox*>()) {

        QString boxoikeus = box->property("Oikeus").toString();
        if (boxoikeus == "R" && box->isChecked() ) {
            onX = true;
        }
    }

    for( QCheckBox* box: findChildren<QCheckBox*>()) {

        QString boxoikeus = box->property("Oikeus").toString();

        if (onX) {
            box->setEnabled(boxoikeus == "R" || (boxoikeus == "Ko" && !omistaja_ ));
        } else {
            box->setEnabled(boxoikeus != "Ko" || !omistaja_);
        }

    }
}
