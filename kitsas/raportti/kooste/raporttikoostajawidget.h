#ifndef RAPORTTIKOOSTAJAWIDGET_H
#define RAPORTTIKOOSTAJAWIDGET_H

#include <QDialog>

namespace Ui {
class RaporttiKoostajaWidget;
}

class RaporttiKoostajaWidget : public QDialog
{
    Q_OBJECT

public:
    explicit RaporttiKoostajaWidget(QWidget *parent = nullptr);
    ~RaporttiKoostajaWidget();

private:
    void lataaValinnat();
    void valinnatSaapuu(QVariant* data);

    void alustaJaksot(const QVariantList& lista);
    void alustaValinnat(const QVariantList& lista, const QStringList& valitut);

    QVariantMap valintaMap() const;
    QString emails() const;

    void preview();
    void showPreview(QVariant* data);

    void send();
    void sended(QVariant* data);


    Ui::RaporttiKoostajaWidget *ui;
};

#endif // RAPORTTIKOOSTAJAWIDGET_H
