#ifndef ONNIWIDGET_H
#define ONNIWIDGET_H

#include <QWidget>
#include "ui_onniwidget.h"

/**
 * @brief Pieni widget, joka näyttää hetken aikaa viestiä onnistumisesta
 */
class OnniWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OnniWidget(QWidget *parent = nullptr);

signals:

public slots:
    void nayta(const QString& teksti, int aika = 5000);

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    Ui::onniWidget *ui;
};

#endif // ONNIWIDGET_H
