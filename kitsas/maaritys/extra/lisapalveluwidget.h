#ifndef LISAPALVELUWIDGET_H
#define LISAPALVELUWIDGET_H

#include <QGroupBox>
#include "pilvi/pilviextra.h"

class LisaPalveluWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit LisaPalveluWidget(const QVariantMap& data = QVariantMap(), QWidget *parent = nullptr);

protected:
    void updateUi();

    PilviExtra data_;

signals:

};

#endif // LISAPALVELUWIDGET_H
