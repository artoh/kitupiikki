#ifndef SHORTCUTMODEL_H
#define SHORTCUTMODEL_H

#include <QAbstractListModel>

class ShortcutModel : public QAbstractListModel
{
    Q_OBJECT

protected:
    class Shortcut {
    public:
        explicit Shortcut();
        Shortcut(const QString& name, const QStringList& rights, const QStringList& admin);
        Shortcut(const QVariantMap& map);

        QString name() const {return name_; }
        QStringList rights() const { return rights_;}
        QStringList admin() const { return admin_;}

    private:
        QString name_;
        QStringList rights_;
        QStringList admin_;
    };

public:
    enum { RightsRole = Qt::UserRole + 1, AdminRole  = Qt::UserRole + 2};

    explicit ShortcutModel(QObject *parent = nullptr);    

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

    int indexFor(const QStringList& rights, const QStringList& admin) const;
    QString nameFor(const QStringList& rights, const QStringList admin) const;

    void set(const QString & name, const QStringList& rights, const QStringList &admin, int i = -1);
    void poista(int indeksi);

private:
    QList<Shortcut> shortcuts_;
};

#endif // SHORTCUTMODEL_H
