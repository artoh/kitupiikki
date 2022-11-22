#include "oikeuswidget.h"

#include <QSet>
#include <QCheckBox>
#include <QVariant>

OikeusWidget::OikeusWidget(QWidget *parent)
    : QWidget{parent}
{
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        connect( box, &QCheckBox::toggled, this, &OikeusWidget::tarkasta );
    }
}

void OikeusWidget::aseta(const QStringList &oikeus)
{
    for( QCheckBox* box: findChildren<QCheckBox*>()) {
        box->setChecked( oikeus.contains( box->property("Oikeus").toString() ) );
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

void OikeusWidget::kaikki()
{
    for(QCheckBox* box : findChildren<QCheckBox*>())
        box->setChecked(true);

}

void OikeusWidget::tarkasta()
{
    bool onkoMuokattu = ( oikeudet() != alussa_);
    if( onkoMuokattu != muokkausIlmoitus_ ) {
        emit muokattu(onkoMuokattu);
        muokkausIlmoitus_ = onkoMuokattu;
    }
}
