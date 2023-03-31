#ifndef ALIASWIDGET_H
#define ALIASWIDGET_H

#include <QGroupBox>

class AliasWidget : public QGroupBox
{
    Q_OBJECT
public:
    AliasWidget(const QString alias = QString(), QWidget* parent = nullptr);

signals:
    void updateStatus();


protected:
    void updateUi();
    void setup();

    QString alias_;
};

#endif // ALIASWIDGET_H
