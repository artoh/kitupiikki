#include "raporttikoostajawidget.h"
#include "ui_raporttikoostajawidget.h"

#include "db/kirjanpito.h"
#include "kieli/monikielinen.h"
#include "naytin/naytinikkuna.h"
#include "kieli/kielet.h"

#include <QJsonDocument>
#include <QMessageBox>


RaporttiKoostajaWidget::RaporttiKoostajaWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RaporttiKoostajaWidget)
{
    ui->setupUi(this);

    lataaValinnat();

    connect( ui->previewButton, &QPushButton::clicked, this, &RaporttiKoostajaWidget::preview);
    connect( ui->sendButton, &QPushButton::clicked, this, &RaporttiKoostajaWidget::send);
}

RaporttiKoostajaWidget::~RaporttiKoostajaWidget()
{
    valintaMap();   // To save ;)
    delete ui;
}

void RaporttiKoostajaWidget::lataaValinnat()
{
    KpKysely* kysely = kpk("/raportti/koostevalinnat");
    kysely->lisaaAttribuutti("kieli",  Kielet::instanssi()->nykyinen());
    connect(kysely, &KpKysely::vastaus, this, &RaporttiKoostajaWidget::valinnatSaapuu);
    kysely->kysy();        
}

void RaporttiKoostajaWidget::valinnatSaapuu(QVariant *data)
{
    QVariantMap lastData = QJsonDocument::fromJson( kp()->asetukset()->asetus("RaporttiKooste").toUtf8() ).toVariant().toMap();

    QVariantMap map = data->toMap();
    alustaJaksot( map.value("periods").toList());
    alustaValinnat( map.value("options").toList(), lastData.value("options").toStringList());

    ui->kieliCombo->valitse( lastData.value("language").toString());
    ui->vastaanottajaEdit->setText( lastData.value("emails").toString());
    ui->otsikkoEdit->setText( lastData.value("title").toString());
    ui->textEdit->setPlainText(lastData.value("freetext").toString() );

}

void RaporttiKoostajaWidget::alustaJaksot(const QVariantList &lista)
{
    const int currentIndex = ui->jaksoCombo->currentIndex();
    ui->jaksoCombo->clear();
    for(const auto& item : lista) {
        QVariantMap map = item.toMap();
        const QString jaksoTeksti = map.value("text").toString();
        QVariantMap jaksoData;
        jaksoData.insert("startDate", map.value("startDate").toDate().toString("yyyy-MM-dd"));
        jaksoData.insert("endDate", map.value("endDate").toDate().toString("yyyy-MM-dd"));
        QIcon icon(":/pic/tyhja16.png");
        if (map.value("reported").toBool()) icon = QIcon(":/pic/ok.png");
        else if( map.value("vat").toBool()) icon = QIcon(":/pic/vero.png");
        ui->jaksoCombo->addItem( icon, jaksoTeksti, jaksoData );
    }
    if( currentIndex > -1) {
        ui->jaksoCombo->setCurrentIndex( currentIndex );
    }
}

void RaporttiKoostajaWidget::alustaValinnat(const QVariantList &lista, const QStringList &valitut)
{
    ui->listWidget->clear();
    for(const auto& item: lista) {
        QVariantMap map = item.toMap();
        QString label = map.value("label").toString();
        const QString& value = map.value("value").toString();
        QListWidgetItem* option = new QListWidgetItem(ui->listWidget);
        option->setText( label );
        option->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        option->setData(Qt::UserRole, value);
        option->setCheckState(valitut.contains(value) ? Qt::Checked :  Qt::Unchecked);
    }
}

QVariantMap RaporttiKoostajaWidget::valintaMap() const
{
    QVariantMap map;
    const QVariantMap jaksoData = ui->jaksoCombo->currentData().toMap();
    map.insert("period", jaksoData);
    map.insert("language", ui->kieliCombo->kieli());
    map.insert("title", ui->otsikkoEdit->text());
    map.insert("emails", emails());
    map.insert("freetext", ui->textEdit->toPlainText());
    QStringList options;

    for(int i=0; i < ui->listWidget->count(); i++) {
        QListWidgetItem* item = ui->listWidget->item(i);
        if( item->checkState() == Qt::Checked) {
            options.append( item->data(Qt::UserRole).toString() );
        }
    }
    map.insert("options", options);

    kp()->asetukset()->aseta("RaporttiKooste", QString::fromUtf8(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact)));

    return map;
}

QString RaporttiKoostajaWidget::emails() const
{
    QStringList parts = ui->vastaanottajaEdit->text().split(",");
    QStringList formatted;
    for(const auto& part : parts) {
        QStringList name;
        QString address;
        QStringList words = part.split(" ");
        for(QString& word: words) {
            word.remove("\"");
            if( word.contains("@")) {
                address = word;
            } else {
                name.append(word);
            }
        }
        if( address.isEmpty()) {
            return QString();
        } else if( name.isEmpty()) {
            formatted.append(address);
        } else {
            formatted.append("\"" + name.join(" ") + "\" " + address);
        }
    }
    return formatted.join(", ");
}

void RaporttiKoostajaWidget::preview()
{
    QVariantMap map = valintaMap();
    map.insert("action", "preview");
    KpKysely* kysely = kpk("/raportti/kooste", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &RaporttiKoostajaWidget::showPreview);
    kysely->kysy(map);
}

void RaporttiKoostajaWidget::showPreview(QVariant *data)
{
    QByteArray ba = data->toByteArray();
    NaytinIkkuna::nayta(ba);
}

void RaporttiKoostajaWidget::send()
{
    ui->sendButton->setEnabled(false);
    QVariantMap map = valintaMap();
    map.insert("action", "send");
    KpKysely* kysely = kpk("/raportti/kooste", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &RaporttiKoostajaWidget::sended);
    kysely->kysy(map);
}

void RaporttiKoostajaWidget::sended(QVariant *data)
{
    QVariantMap map = data->toMap();
    QStringList rejected = map.value("rejected").toStringList();
    QStringList accepted = map.value("accepted").toStringList();
    if( !rejected.isEmpty()) {
        QMessageBox::critical(this, tr("Raportin lähettäminen epäonnistui"), tr("Raporttia ei voitu lähettää osoitteeseen %1").arg(rejected.join(", ")));
    } else if( !map.value("id").toInt()) {
        QMessageBox::critical(this, tr("Raportin tallentaminen epäonnistui"), tr("Raportin tallentaminen kirjanpitoon epäonnistui"));
    } else if( !accepted.isEmpty()) {
        emit kp()->onni( tr("Raportti lähetetty %1 vastaanottajalle").arg( accepted.count()) );
    } else {
        emit kp()->onni( tr("Raportti tallennettu"));
    }
    lataaValinnat();    // To see check mark
    ui->sendButton->setEnabled( true );
}
