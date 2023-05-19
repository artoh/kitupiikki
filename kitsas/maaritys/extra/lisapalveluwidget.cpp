#include "lisapalveluwidget.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "extradialog.h"
#include "extralogmodel.h"

#include <QSvgWidget>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTableView>
#include <QMainWindow>
#include <QHeaderView>
#include <QDesktopServices>

LisaPalveluWidget::LisaPalveluWidget(const QVariantMap &data, QWidget *parent)
    : QGroupBox{parent}, data_{data}
{
    updateUi();
}

void LisaPalveluWidget::updateUi()
{
    qDeleteAll(children());

    QHBoxLayout* mainLayout = new QHBoxLayout();

    const QString& status = data_.status().value("status").toString();
    QLabel *icon = new QLabel();
    if( !data_.active()) {
        icon->setPixmap(QPixmap(":/pic/palat-harmaa.png"));
    } else if(status == "OK") {
        icon->setPixmap(QPixmap(":/pic/ok64.png"));
    } else if(status == "ERROR") {
        icon->setPixmap(QPixmap(":/pic/virhe64.png"));
    } else if(status == "WARNING") {
        icon->setPixmap(QPixmap(":/pic/varoitus64.png"));
    } else {
        icon->setPixmap(QPixmap(":/pic/palat.png"));
    }
    mainLayout->addWidget(icon);

    QVBoxLayout* textLayout = new QVBoxLayout();
    QHBoxLayout* nameLayout = new QHBoxLayout();

    QLabel* nameLabel = new QLabel( data_.title() );
    nameLabel->setStyleSheet("font-weight: bold");
    nameLayout->addWidget(nameLabel);

    if( data_.inTesting()) {
        QLabel *testingLabel = new QLabel(tr(" Vain testikäyttäjille "));
        testingLabel->setStyleSheet("background-color: orange; color: black;");
        nameLayout->addWidget(testingLabel);

    }

    if( !data_.price().isEmpty()) {
        QLabel *priceLabel = new QLabel( " " + data_.price() + " ");
        priceLabel->setStyleSheet("background-color: yellow; color: black;");
        nameLayout->addWidget(priceLabel);
    }

    nameLayout->addStretch();
    textLayout->addLayout(nameLayout);

    QLabel* infoLabel = new QLabel( data_.active() ? data_.statusinfo() : data_.description() );
    textLayout->addWidget(infoLabel);

    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();

    if( data_.active()) {
        QVBoxLayout* actionLayout = new QVBoxLayout();
        actionLayout->setAlignment(Qt::AlignTop);
        for(const auto& item : data_.actions()) {
            const QVariantMap map = item.toMap();
            Monikielinen buttonText(map.value("label").toMap());            
            QPushButton* actionButton = new QPushButton(buttonText.teksti());
            if( map.contains("icon"))
                actionButton->setIcon(QIcon(":/pic/" + map.value("icon").toString()));

            if( map.contains("name")) {
                const QString& actionName = map.value("name").toString();
                connect( actionButton, &QPushButton::clicked, this, [this, actionName] { this->action(actionName);});
            } else if (map.contains("url"))  {
                const QString& actionUrl = map.value("url").toString();
                connect( actionButton, &QPushButton::clicked, this, [this, actionUrl] { this->actionLink(actionUrl);});
            }   else {
                delete actionButton;
                continue;
            }
            actionLayout->addWidget(actionButton);
        }
        mainLayout->addLayout(actionLayout);
    }
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->setAlignment(Qt::AlignTop);

    const QString url = data_.info().value("doc").toString();
    QPushButton* helpButton = new QPushButton(QIcon(":/pic/ohje.png"), tr("Ohje"));
    connect( helpButton, &QPushButton::clicked, this, [url] { kp()->ohje(url); });
    buttonLayout->addWidget(helpButton);

    if( data_.active()) {
        QPushButton* logButton = new QPushButton(QIcon(":/pic/Paivakirja64.png"), tr("Tapahtumat"));
        connect( logButton, &QPushButton::clicked, this, &LisaPalveluWidget::loki);
        buttonLayout->addWidget(logButton);

        QPushButton* removeButton = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista käytöstä"));
        connect( removeButton, &QPushButton::clicked, this, &LisaPalveluWidget::passivate);
        buttonLayout->addWidget(removeButton);
    } else {
        QPushButton* addButton = new QPushButton(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön"));
        connect( addButton, &QPushButton::clicked, this, [this] { this->action("enable"); });
        buttonLayout->addWidget(addButton);
    }
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

void LisaPalveluWidget::passivate()
{
    const QString text = tr("Poistatko käytöstä lisäosan %1?").arg(data_.title());
    QMessageBox mbox( QMessageBox::NoIcon, data_.title(),
                      text, QMessageBox::Yes | QMessageBox::No | QMessageBox::Help, this );
    int answer = mbox.exec();
    if( answer == QMessageBox::Help) {
        kp()->ohje( data_.info().value("doc").toString() );
    } else if( answer == QMessageBox::Yes) {
        KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1/disable").arg(data_.id()), KpKysely::POST);
        connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::actionData);
        kp()->odotusKursori(true);
        kysely->kysy();
    }
}


void LisaPalveluWidget::action(const QString &action)
{
    kp()->odotusKursori(true);
    KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1/%2").arg(data_.id()).arg(action), KpKysely::GET);
    connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::actionData);
    kysely->kysy();
}

void LisaPalveluWidget::actionData(QVariant *data)
{
    kp()->odotusKursori(false);
    const QVariantMap& map = data->toMap();
    const QString& type = map.value("show").toString();
    if( type == "form") {
        actionDialog(map);
    } else if( type == "message") {
        actionMessage(map);
    } else if( type == "update") {
        emit updateStatus();
    } else if( type == "link") {
        actionLink(map.value("url").toString());
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

    if( data.contains("help")) {
        box.addButton(QMessageBox::Help);
    }

    if( icon == "info")
        box.setIcon(QMessageBox::Information);
    else if(icon == "warning")
        box.setIcon(QMessageBox::Warning);
    else if(icon == "critical")
        box.setIcon(QMessageBox::Critical);

    if( box.exec() == QMessageBox::Help) {
        kp()->ohje( data.value("help").toString() );
    }

    emit updateStatus();
}

void LisaPalveluWidget::actionLink(const QString url)
{
    if( !lastLink_.isEmpty() && lastLink_ == url &&
        linkTime_.secsTo(QDateTime::currentDateTime()) < 15) {

        // Näytetään osoite koska avaaminen ei välttis toimi
        QMessageBox::information(this, data_.title(),
                                 tr("Jatka verkkoselaimella osoitteeseen <a href=%1>%1</a>").arg(url));
        lastLink_.clear();
        return;

    }
    if( QDesktopServices::openUrl(url) ) {
        lastLink_ = url;
        linkTime_ = QDateTime::currentDateTime();
    } else {
        QMessageBox::information(this, data_.title(),
                                 tr("Jatka verkkoselaimella osoitteeseen <a href=%1>%1</a>").arg(url));
    }
}

void LisaPalveluWidget::loki()
{
    KpKysely* kysely = kp()->pilvi()->loginKysely(QString("/extras/%1/log").arg(data_.id()), KpKysely::GET);
    connect( kysely, &KpKysely::vastaus, this, &LisaPalveluWidget::naytaLoki);
    kysely->kysy();
}


void LisaPalveluWidget::naytaLoki(const QVariant *data)
{
    QDialog* lokiDlg = new QDialog(this);

    ExtraLogModel* model = new ExtraLogModel(lokiDlg);
    model->lataa(data->toList());
    QTableView* view = new QTableView(lokiDlg);
    view->horizontalHeader()->setStretchLastSection(true);
    view->setModel(model);
    view->setSelectionMode(QTableView::SelectionMode::NoSelection);
    view->setAlternatingRowColors(true);

    QHBoxLayout* leiska = new QHBoxLayout();
    leiska->addWidget(view);
    lokiDlg->setLayout(leiska);
    lokiDlg->setWindowTitle(data_.title());
    view->resizeColumnToContents(0);
    view->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
    lokiDlg->resize(600,400);
    lokiDlg->exec();

}

