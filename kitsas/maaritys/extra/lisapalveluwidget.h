#ifndef LISAPALVELUWIDGET_H
#define LISAPALVELUWIDGET_H

#include <QGroupBox>
#include <QDateTime>
#include "pilvi/pilviextra.h"

class LisaPalveluWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit LisaPalveluWidget(const QVariantMap& data = QVariantMap(), QWidget *parent = nullptr);

protected:
    void updateUi();

    void activate();
    void passivate();

    void setOnOff(bool on = true);
    void action(const QString& action);
    void actionData(QVariant* data);
    void actionDialog(const QVariantMap& dialogData);
    void actionMessage(const QVariantMap& data);
    void actionLink(const QString url);

    void loki();
    void naytaLoki(const QVariant* data);

    PilviExtra data_;

    QString lastLink_;
    QDateTime linkTime_;


signals:
    void updateStatus();


};

#endif // LISAPALVELUWIDGET_H
