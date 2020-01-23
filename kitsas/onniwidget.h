#ifndef ONNIWIDGET_H
#define ONNIWIDGET_H

#include <QWidget>
#include "ui_onniwidget.h"
#include "db/kirjanpito.h"

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
    void nayta(const QString& teksti, Kirjanpito::Onni tyyppi = Kirjanpito::Onnistui, int aika = 5000);
    void aikakului();

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    Ui::onniWidget *ui;
    int ikkunat = 0;
};

#endif // ONNIWIDGET_H
