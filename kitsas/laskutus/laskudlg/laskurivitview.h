#ifndef LASKURIVITVIEW_H
#define LASKURIVITVIEW_H


#include <QTableView>

class TositeRivit;

class LaskuRivitView : public QTableView
{
    Q_OBJECT
public:
    LaskuRivitView(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void performDrag(int sourceRow);

    QPoint startPos_;
    int sourceRow_ = -1;

    TositeRivit *rivit();
};

#endif // LASKURIVITVIEW_H
