#ifndef NAYTIN_NAYTINSCENE_H
#define NAYTIN_NAYTINSCENE_H

#include <QGraphicsScene>

namespace Naytin {

class NaytinScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit NaytinScene(QObject *parent = nullptr);
    bool pudotusSallittu() const { return pudotusSallittu_;}
    void salliPudotus(bool sallittu = true);

signals:
    void tiedostoPudotettu(const QString polku);

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

protected:
    bool pudotusSallittu_ = false;
};

} // namespace Naytin

#endif // NAYTIN_NAYTINSCENE_H
