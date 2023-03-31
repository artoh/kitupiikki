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
#include "aliaswidget.h"


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
    QVariantMap map = data->toMap();
    QVariantList lista = map.value("extras").toList();

    QLayoutItem *child;
    while ((child = extraLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    AliasWidget* alias = new AliasWidget(map.value("alias").toString());
    extraLayout->addWidget(alias);
    connect( alias, &AliasWidget::updateStatus, this, &LisaPalvelutMaaritys::updateExtras);

    for(const auto& item : lista) {
        LisaPalveluWidget* widget = new LisaPalveluWidget(item.toMap());
        extraLayout->addWidget(widget);
        connect( widget, &LisaPalveluWidget::updateStatus, this, &LisaPalvelutMaaritys::updateExtras);
    }
    extraLayout->setSizeConstraint(QLayout::SetMinimumSize);
    extraLayout->addStretch();

}
