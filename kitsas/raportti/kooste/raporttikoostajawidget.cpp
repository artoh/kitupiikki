#include "raporttikoostajawidget.h"
#include "ui_raporttikoostajawidget.h"

#include "db/kirjanpito.h"
#include "kieli/monikielinen.h"
#include "naytin/naytinikkuna.h"

#include <QJsonDocument>


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
    kp()->asetukset()->aseta("RaporttiKooste", QString::fromUtf8(QJsonDocument::fromVariant(valintaMap()).toJson(QJsonDocument::Compact)));
    delete ui;
}

void RaporttiKoostajaWidget::lataaValinnat()
{
    KpKysely* kysely = kpk("/raportti/koostevalinnat");
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

}

void RaporttiKoostajaWidget::alustaJaksot(const QVariantList &lista)
{
    ui->jaksoCombo->clear();
    for(const auto& item : lista) {
        QVariantMap map = item.toMap();
        const QString jaksoTeksti = QString("%1 - %2").arg( map.value("startDate").toDate().toString("dd.MM"), map.value("endDate").toDate().toString("dd.MM.yyyy") );
        QVariantMap jaksoData;
        jaksoData.insert("startDate", map.value("startDate").toDate().toString("yyyy-MM-dd"));
        jaksoData.insert("endDate", map.value("endDate").toDate().toString("yyyy-MM-dd"));
        ui->jaksoCombo->addItem( jaksoTeksti, jaksoData );
    }
}

void RaporttiKoostajaWidget::alustaValinnat(const QVariantList &lista, const QStringList &valitut)
{
    ui->listWidget->clear();
    for(const auto& item: lista) {
        QVariantMap map = item.toMap();
        Monikielinen teksti( map.value("label"));
        const QString& value = map.value("value").toString();
        QListWidgetItem* option = new QListWidgetItem(ui->listWidget);
        option->setText( teksti.teksti() );
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
    return formatted.join("\n");
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
    QVariantMap map = valintaMap();
    map.insert("action", "send");
    KpKysely* kysely = kpk("/raportti/kooste", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &RaporttiKoostajaWidget::sended);
    kysely->kysy(map);
}

void RaporttiKoostajaWidget::sended(QVariant *data)
{
    QVariant map = data->toMap();
    /* TODO */
}
