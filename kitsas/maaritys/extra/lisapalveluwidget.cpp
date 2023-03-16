#include "lisapalveluwidget.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "extradialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

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

    if( data_.active()) {
        QVBoxLayout* actionLayout = new QVBoxLayout();
        const QVariantList actionList = data_.info().value("actions").toList();
        for(const auto& item : actionList) {
            const QVariantMap map = item.toMap();
            Monikielinen buttonText(map.value("label").toMap());
            const QString& actionName = map.value("name").toString();
            QPushButton* actionButton = new QPushButton(buttonText.teksti());
            connect( actionButton, &QPushButton::clicked, this, [this, actionName] { this->action(actionName);});
            actionLayout->addWidget(actionButton);
        }
        mainLayout->addLayout(actionLayout);
    }
    QVBoxLayout* buttonLayout = new QVBoxLayout();

    const QString url = data_.info().value("doc").toString();
    QPushButton* helpButton = new QPushButton(QIcon(":/pic/ohje.png"), tr("Ohje"));
    connect( helpButton, &QPushButton::clicked, this, [url] { kp()->ohje(url); });
    buttonLayout->addWidget(helpButton);

    if( data_.active()) {
        QPushButton* removeButton = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista käytöstä"));
        connect( removeButton, &QPushButton::clicked, this, &LisaPalveluWidget::passivate);
        buttonLayout->addWidget(removeButton);
    } else {
        QPushButton* addButton = new QPushButton(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön"));
        connect( addButton, &QPushButton::clicked, this, &LisaPalveluWidget::activate);
        buttonLayout->addWidget(addButton);
    }
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

void LisaPalveluWidget::activate()
{
    Monikielinen getStartedText( data_.info().value("getstarted").toMap() );
    const QString text = tr("Otatko käyttöön lisäosan %1?").arg(data_.title()) + "\n\n" +
            getStartedText.teksti();


    QMessageBox mbox( QMessageBox::NoIcon, tr("Vahvista lisäosan käyttöönotto"),
                      text, QMessageBox::Yes | QMessageBox::No | QMessageBox::Help, this );
    int answer = mbox.exec();
    if( answer == QMessageBox::Help) {
        kp()->ohje( data_.info().value("doc").toString() );
    } else if( answer == QMessageBox::Yes) {
        setOnOff(true);
    }
}

void LisaPalveluWidget::passivate()
{
    const QString text = tr("Poistatko käytöstä lisäosan %1?").arg(data_.title());
    QMessageBox mbox( QMessageBox::NoIcon, tr("Vahvista lisäosan käyttöönotto"),
                      text, QMessageBox::Yes | QMessageBox::No | QMessageBox::Help, this );
    int answer = mbox.exec();
    if( answer == QMessageBox::Help) {
        kp()->ohje( data_.info().value("doc").toString() );
    } else if( answer == QMessageBox::Yes) {
        setOnOff(false);
    }
}

void LisaPalveluWidget::setOnOff(bool on)
{
    KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1").arg(data_.id()), KpKysely::PUT);
    QVariantMap payload;
    payload.insert("active", on);
    payload.insert("settings", data_.settings());
    connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::update);
    kysely->kysy(payload);
}

void LisaPalveluWidget::action(const QString &action)
{
    KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1/%2").arg(data_.id()).arg(action), KpKysely::GET);
    connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::actionData);
    kysely->kysy();
}

void LisaPalveluWidget::actionData(QVariant *data)
{
    const QVariantMap& map = data->toMap();
    const QString& type = map.value("show").toString();
    if( type == "form") {
        actionDialog(map);
    } else if( type == "message") {
        actionMessage(map);
    }
}

void LisaPalveluWidget::actionDialog(const QVariantMap &dialogData)
{
    ExtraDialog dlg(this);
    dlg.init(data_.title(), dialogData);

    if( dlg.exec() == QDialog::Accepted) {
        // Lähetetään tämä palvelimelle
        const QString& action = dialogData.value("action").toString();
        KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1/%2").arg(data_.id()).arg(action), KpKysely::POST);
        connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::actionData);
        kysely->kysy(dlg.values());
    }
}

void LisaPalveluWidget::actionMessage(const QVariantMap &data)
{
    Monikielinen text(data.value("text").toMap());
    const QString& icon = data.value("icon").toString();

    QMessageBox box(this);
    box.setWindowTitle(data_.title());
    box.setText(text.teksti());
    box.setStandardButtons(QMessageBox::Ok);

    if( icon == "info")
        box.setIcon(QMessageBox::Information);
    else if(icon == "warning")
        box.setIcon(QMessageBox::Warning);
    else if(icon == "critical")
        box.setIcon(QMessageBox::Critical);

    box.exec();
}

