#include "aliaswidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTableView>
#include <QMainWindow>
#include <QHeaderView>

#include "aliasdialog.h"

AliasWidget::AliasWidget(const QString alias, QWidget *parent)
    : QGroupBox(parent), alias_(alias)
{
    updateUi();
}

void AliasWidget::updateUi()
{
        QHBoxLayout* mainLayout = new QHBoxLayout();
        QLabel *icon = new QLabel();
        icon->setPixmap(QPixmap(":/pic/link.png"));
        mainLayout->addWidget(icon);

        QVBoxLayout* textLayout = new QVBoxLayout();

        QLabel* nameLabel = new QLabel( tr("Alias"));
        nameLabel->setStyleSheet("font-weight: bold");
        textLayout->addWidget(nameLabel);

        QLabel* infoLabel = new QLabel( alias_.isEmpty() ? tr("Voit määrittää kirjanpidolle nimen tai lyhenteen, jolla kirjanpitoon viitataan lisäpalveluissa esimerkiksi aineistoa tuotaessa.") : alias_ );
        infoLabel->setWordWrap(true);
        textLayout->addWidget(infoLabel);

        mainLayout->addLayout(textLayout);
        mainLayout->addStretch();

        QVBoxLayout* buttonLayout = new QVBoxLayout();
        buttonLayout->setAlignment(Qt::AlignTop);

        QPushButton* aliasButton = new QPushButton(QIcon(alias_.isEmpty() ? ":/pic/lisaa.png" : ":/pic/ratas.png"),
                                                   alias_.isEmpty() ? tr("Ota käyttöön") : tr("Määritä"));
        connect( aliasButton, &QPushButton::clicked, this, &AliasWidget::setup);
        buttonLayout->addWidget(aliasButton);

        mainLayout->addLayout(buttonLayout);
        setLayout(mainLayout);
}

void AliasWidget::setup()
{
    AliasDialog dlg(this);
    dlg.asetaAlias(alias_);
    if( dlg.exec() == QDialog::Accepted)
        emit updateStatus();
}
