#ifndef TUKIWIDGET_H
#define TUKIWIDGET_H

#include <QWidget>

#include "pilvi/pilvikayttaja.h"

namespace Ui {
class TukiWidget;
}

class TukiWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TukiWidget(QWidget *parent = nullptr);
    ~TukiWidget();

    void kirjauduttu(PilviKayttaja kayttaja);

    void ohjeet();
    void tuki();
    void palaute();
    void virheenjaljitys();

private:
    Ui::TukiWidget *ui;
};

#endif // TUKIWIDGET_H
