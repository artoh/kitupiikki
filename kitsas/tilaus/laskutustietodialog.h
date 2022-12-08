#ifndef LASKUTUSTIETODIALOG_H
#define LASKUTUSTIETODIALOG_H

#include <QDialog>

namespace Ui {
    class TilausYhteys;
}

class QDialogButtonBox;

class LaskutustietoDialog : public QDialog
{
    Q_OBJECT
public:
    LaskutustietoDialog(QWidget* parent = nullptr);
    ~LaskutustietoDialog();

protected:
    void haeTiedot();
    void tiedotSaapuu(QVariant* data);

    void maksutapaVaihtui();

protected:
    Ui::TilausYhteys* ui;
    QDialogButtonBox *box;
};

#endif // LASKUTUSTIETODIALOG_H
