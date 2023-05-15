#include "extradialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QComboBox>
#include <QPlainTextEdit>

#include "db/kirjanpito.h"
#include "tools/kpdateedit.h"
#include "kieli/monikielinen.h"
#include "db/tilinvalintaline.h"
#include "tools/tilicombo.h"


ExtraDialog::ExtraDialog(QWidget *parent)
    : QDialog(parent)
{

}

void ExtraDialog::init(const QString &title, const QVariantMap &map, const QVariantMap &values)
{
    setWindowTitle(title);

    QVBoxLayout* mainLayout = new QVBoxLayout();

    Monikielinen header(map.value("header"));
    Monikielinen intro(map.value("intro"));
    Monikielinen footer(map.value("footer"));

    QLabel* headerLabel = new QLabel(header.teksti());
    headerLabel->setStyleSheet("font-weight: bold");
    QLabel* introLabel = new QLabel(intro.teksti());
    introLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);
    mainLayout->addWidget(introLabel);

    mainLayout->addLayout( initFields( map.value("fields").toList(), values ) );


    QString helpUrl = map.value("help").toString();

    mainLayout->addStretch();

    QLabel* footerLabel = new QLabel(footer.teksti());
    footerLabel->setWordWrap(true);
    mainLayout->addWidget(footerLabel);

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

        TilinvalintaLine* tiliEdit = qobject_cast<TilinvalintaLine*>(widget);
        if( tiliEdit) {
            data.insert(id, tiliEdit->valittuTilinumero());
            continue;
        }

        TiliCombo* tcombo = qobject_cast<TiliCombo*>(widget);
        if( tcombo ) {
            data.insert(id, tcombo->valittuTilinumero());
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

        QPlainTextEdit* editor = qobject_cast<QPlainTextEdit*>(widget);
        if( editor ) {
            data.insert(id, editor->toPlainText());
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
        } else if( type == "accountline") {
            TilinvalintaLine* edit = new TilinvalintaLine();
            edit->asetaModel(kp()->tilit());
            if( map.contains("filter"))
                edit->suodataTyypilla( map.value("filter").toString() );
            if( !value.isEmpty())
                edit->valitseTiliNumerolla(value.toInt());
            widget = edit;
        } else if( type == "accountcombo") {
            TiliCombo* edit = new TiliCombo();
            if( map.contains("filter"))
                edit->suodataTyypilla(map.value("filter").toString());
            if( !value.isEmpty())
                edit->valitseTili( value.toInt() );
            widget = edit;
        } else if( type == "textedit") {
            QPlainTextEdit* edit = new QPlainTextEdit();
            if( !value.isEmpty())
                edit->setPlainText(value);
            widget = edit;
        }

        if( widget ) {
            widget->setObjectName(id);
            layout->addRow( label.teksti(), widget );
        }

    }

    return layout;
}
