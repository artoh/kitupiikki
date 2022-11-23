#include "oikeuswidget.h"

#include <QSet>
#include <QCheckBox>
#include <QVariant>

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
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        box->setChecked( oikeus.contains( box->property("Oikeus").toString() ) );
        connect( box, &QCheckBox::clicked, this, &OikeusWidget::tarkasta );
    }
    alussa_ = oikeudet();
}

QSet<QString> OikeusWidget::oikeudet() const
{
    QSet<QString> set;
    for(const QCheckBox* box : findChildren<QCheckBox*>()) {
        if( box->isChecked()) {
            set.insert( box->property("Oikeus").toString() );
        }
    }
    return set;
}

QStringList OikeusWidget::oikeuslista() const
{
    return oikeudet().values();
}

void OikeusWidget::kayttoon(const QString &oikeus, bool onkoKaytossa)
{
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        if( box->property("Oikeus").toString() == oikeus) {
            box->setEnabled(onkoKaytossa);
        }
    }
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
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(true);
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
    emit muokattu( onkoMuokattu() );
}
