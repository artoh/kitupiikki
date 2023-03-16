#ifndef EXTRADIALOG_H
#define EXTRADIALOG_H

#include <QDialog>
#include <QVariantMap>

class QFormLayout;

class ExtraDialog : public QDialog
{
    Q_OBJECT
public:
    ExtraDialog(QWidget* parent = nullptr);
    void init(const QString& title, const QVariantMap& map, const QVariantMap& values = QVariantMap());

    QVariantMap values();
protected:
    QFormLayout* initFields(const QVariantList& fields, const QVariantMap& values);
};

#endif // EXTRADIALOG_H
