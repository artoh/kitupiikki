#ifndef LISAOSALOKIIKKUNA_H
#define LISAOSALOKIIKKUNA_H

#include <QMainWindow>

class LisaosaLokiModel;
class QTableView;

class LisaosaLokiIkkuna : public QMainWindow
{
    Q_OBJECT
public:
    explicit LisaosaLokiIkkuna(QWidget *parent = nullptr);
    ~LisaosaLokiIkkuna();

    void lataaLoki(const QString& lisaosaId, const QString& lisaosanimi);

private:
    void refresh();

private:
    LisaosaLokiModel* model_;
    QTableView* view_;

signals:

};

#endif // LISAOSALOKIIKKUNA_H
