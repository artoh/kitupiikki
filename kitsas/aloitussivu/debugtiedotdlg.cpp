#include "debugtiedotdlg.h"
#include "ui_debugtiedotdlg.h"

#include "tools/kitsaslokimodel.h"
#include "jarjestelmatiedot.h"

#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

#include <QJsonDocument>
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

DebugTiedotDlg::DebugTiedotDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DebugTiedotDlg),
    jarjestelma{ new JarjestelmaTiedot(this)}
{
    ui->setupUi(this);

    ui->debugView->setModel(KitsasLokiModel::instanssi());
    ui->jarjestelmaView->setModel(jarjestelma);
}

DebugTiedotDlg::~DebugTiedotDlg()
{
    delete ui;
}

QVariantMap DebugTiedotDlg::data()
{
    QVariantMap map;
    map.insert("description", ui->kuvausEdit->toPlainText());
    map.insert("system", jarjestelma->asList());
    map.insert("log", KitsasLokiModel::instanssi()->asList());
    map.insert("database", kp()->kirjanpitoPolku());
    return map;
}

void DebugTiedotDlg::sent()
{
    QMessageBox::information(this, tr("Tiedot lähetetty"),
                             tr("Tekniset tiedot on nyt lähetty tuen luettavaksi."));

    QDialog::accept();
}

void DebugTiedotDlg::virhe()
{
    QMessageBox::critical(this, tr("Tietojen lähettäminen epäonnistui"),
                          tr("Tietojen lähettäminen tukee epäonnistui. Yritä hetken kuluttua uudestaan."));
}

void DebugTiedotDlg::accept()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely("/support", KpKysely::POST);
    connect( kysymys, &KpKysely::vastaus, this, &DebugTiedotDlg::sent);
    connect( kysymys, &KpKysely::virhe, this, &DebugTiedotDlg::virhe);
    kysymys->kysy(data());
}

