#include "lisapalveluwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>

LisaPalveluWidget::LisaPalveluWidget(const QVariantMap &data, QWidget *parent)
    : QGroupBox{parent}, data_{data}
{
    updateUi();
}

void LisaPalveluWidget::updateUi()
{
    qDeleteAll(children());

    QHBoxLayout* mainLayout = new QHBoxLayout();
    QVBoxLayout* textLayout = new QVBoxLayout();

    QLabel* nameLabel = new QLabel( data_.title() );
    nameLabel->setStyleSheet("font-weight: bold");
    textLayout->addWidget(nameLabel);

    QLabel* infoLabel = new QLabel( data_.active() ? data_.statusinfo() : data_.description() );
    textLayout->addWidget(infoLabel);

    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();

    QVBoxLayout* buttonLayout = new QVBoxLayout();

    if( data_.active()) {
        QPushButton* removeButton = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista käytöstä"));
        buttonLayout->addWidget(removeButton);
    } else {
        QPushButton* addButton = new QPushButton(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön"));
        buttonLayout->addWidget(addButton);
    }
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

