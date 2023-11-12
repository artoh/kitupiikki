#ifndef YKSITYINENLISAOSADIALOGI_H
#define YKSITYINENLISAOSADIALOGI_H

#include <QDialog>

namespace Ui {
class YksityinenLisaosaDialogi;
}

class YksityinenLisaosaDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit YksityinenLisaosaDialogi(QWidget *parent = nullptr);
    ~YksityinenLisaosaDialogi();

    static QString getId(QWidget* parent = nullptr);
protected:
    void refreshOk();

private:
    Ui::YksityinenLisaosaDialogi *ui;
};

#endif // YKSITYINENLISAOSADIALOGI_H
