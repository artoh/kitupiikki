#ifndef VARINVALINTA_H
#define VARINVALINTA_H

#include <QLabel>
#include <QColor>

class VarinValinta : public QLabel
{
    Q_OBJECT
public:
    VarinValinta(QWidget* parent = nullptr);

    void setColor(const QString& variteksti);
    QString color() const;

    void paivita();
    void vaihda();

signals:
    void variVaihtui(const QColor& vari);

protected:
    QColor vari_;

    void mousePressEvent(QMouseEvent *event) override;
};

#endif // VARINVALINTA_H
