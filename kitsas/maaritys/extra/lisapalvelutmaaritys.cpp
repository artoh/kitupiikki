#include "lisapalvelutmaaritys.h"

#include <QBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "lisapalveluwidget.h"


LisaPalvelutMaaritys::LisaPalvelutMaaritys()
{
    setupUi();
    updateExtras();
}

void LisaPalvelutMaaritys::setupUi()
{
    QLabel* otsikko = new QLabel("Lisäpalvelut");
    otsikko->setStyleSheet("font-weight: bold");
    QScrollArea* skrolli = new QScrollArea;

    extraLayout = new QVBoxLayout;

    QWidget* sisalto = new QWidget();
    sisalto->setLayout(extraLayout);
    skrolli->setWidget(sisalto);
    skrolli->setWidgetResizable(true);

    QVBoxLayout* leiska = new QVBoxLayout();
    leiska->addWidget(otsikko);
    leiska->addWidget(skrolli);
    setLayout(leiska);
}

void LisaPalvelutMaaritys::updateExtras()
{
    KpKysely* kysely = kp()->pilvi()->loginKysely("/extras");
    connect( kysely, &KpKysely::vastaus, this, &LisaPalvelutMaaritys::showExtras);
    kysely->kysy();
}

void LisaPalvelutMaaritys::showExtras(QVariant *data)
{
    QVariantList lista = data->toList();

    QLayoutItem *child;
    while ((child = extraLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for(const auto& item : lista) {
        LisaPalveluWidget* widget = new LisaPalveluWidget(item.toMap());
        extraLayout->addWidget(widget);
    }
    extraLayout->setSizeConstraint(QLayout::SetMinimumSize);
    extraLayout->addStretch();

}
