#include "extradialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QComboBox>

#include "db/kirjanpito.h"
#include "tools/kpdateedit.h"
#include "kieli/monikielinen.h"

ExtraDialog::ExtraDialog(QWidget *parent)
    : QDialog(parent)
{

}

void ExtraDialog::init(const QString &title, const QVariantMap &map, const QVariantMap &values)
{
    setWindowTitle(title);

    QVBoxLayout* mainLayout = new QVBoxLayout();

    Monikielinen header(map.value("header").toMap());
    Monikielinen intro(map.value("intro").toMap());

    QLabel* headerLabel = new QLabel(header.teksti());
    headerLabel->setStyleSheet("font-weight: bold");
    QLabel* introLabel = new QLabel(intro.teksti());
    introLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);
    mainLayout->addWidget(introLabel);

    mainLayout->addLayout( initFields( map.value("fields").toList(), values ) );

    QString helpUrl = map.value("help").toString();

    mainLayout->addStretch();
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    connect( bbox, &QDialogButtonBox::accepted, this, &ExtraDialog::accept);
    connect( bbox, &QDialogButtonBox::rejected, this, &ExtraDialog::reject);
    connect( bbox, &QDialogButtonBox::helpRequested, this, [helpUrl] { kp()->ohje(helpUrl);});
    mainLayout->addWidget(bbox);

    setLayout(mainLayout);
}

QVariantMap ExtraDialog::values()
{
    QVariantMap data;

    for(QWidget* widget : findChildren<QWidget*>()) {
        const QString id = widget->objectName();
        if( id.isEmpty() ) continue;

        KpDateEdit* kpdate = qobject_cast<KpDateEdit*>(widget);
        if( kpdate ) {
            data.insert(id, kpdate->date().toString("yyyy-MM-dd"));
            continue;
        }

        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
        if( lineEdit ) {
            data.insert(id, lineEdit->text());
            continue;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(widget);
        if( combo ) {
            data.insert(id, combo->currentData().toString());
            continue;
        }
    }
    return data;
}

QFormLayout *ExtraDialog::initFields(const QVariantList &fields, const QVariantMap &values)
{
    QFormLayout* layout = new QFormLayout();

    for(const auto& item : fields) {
        QVariantMap map = item.toMap();

        Monikielinen label(map.value("label").toMap());
        const QString& type = map.value("type").toString();
        const QString& id = map.value("id").toString();
        const QString& value = map.contains("value") ? map.value("value").toString() : values.value(id, QString()).toString();
        QWidget* widget = nullptr;

        if( type == "text") {
            QLineEdit* edit = new QLineEdit();
            if( !value.isEmpty()) edit->setText(value);
            if( map.contains("regexp")) edit->setValidator(new QRegularExpressionValidator(QRegularExpression(map.value("regexp").toString())));
            widget = edit;

        } else if( type == "date") {
            KpDateEdit* edit = new KpDateEdit();
            if( !value.isEmpty()) edit->setDate(QDate::fromString(value, Qt::ISODate));
            edit->setDateRange( map.value("min", QDate()).toDate(), map.value("max", QDate()).toDate() );
            widget = edit;
        } else if( type == "select") {
            QComboBox *box = new QComboBox();
            for(const auto& citem : map.value("options").toList()) {
                QVariantMap cMap = citem.toMap();
                Monikielinen ctext(cMap.value("text").toMap());
                box->addItem( ctext.teksti(), cMap.value("value").toString() );
            }
            if( !value.isEmpty()) box->setCurrentIndex( box->findData(value) );
            widget = box;
        }

        if( widget ) {
            widget->setObjectName(id);
            layout->addRow( label.teksti(), widget );
        }

    }

    return layout;
}
