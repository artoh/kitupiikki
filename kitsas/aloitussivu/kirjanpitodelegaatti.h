#ifndef KIRJANPITODELEGAATTI_H
#define KIRJANPITODELEGAATTI_H

#include <QItemDelegate>

class KirjanpitoDelegaatti : public QItemDelegate
{
    Q_OBJECT
public:
    enum { LogoRooli = Qt::UserRole + 1000,
           AlustettuRooli = Qt::UserRole + 1001,
           IlmoitusRooli = Qt::UserRole + 1002,
           HarjoitusRooli = Qt::UserRole + 1003,
           InboxRooli = Qt::UserRole + 1004,
           OutboxRooli = Qt::UserRole + 1005,
           MarkedRooli = Qt::UserRole + 1006
    };

    KirjanpitoDelegaatti(QObject *parent = nullptr, bool limitys = false);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QRect oikealla(int indeksi, QRect iso) const;

    bool limitys_ = false;
};

#endif // KIRJANPITODELEGAATTI_H
