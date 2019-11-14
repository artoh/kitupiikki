/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CHECKCOMBO_H
#define CHECKCOMBO_H

#include <QComboBox>
#include <QStyledItemDelegate>
#include <QDate>

class QStandardItem;
class QStandardItemModel;

/**
 * @brief ComboBox, josta voi valita useamman
 *
 * Pohjana käytetty Damiel Sorelin gistiä mistic100/qchecklist.h
 */
class CheckCombo : public QComboBox
{
    Q_OBJECT
public:
    CheckCombo(QWidget* parent = nullptr);

    QStandardItem* addItem(const QString& label, const QVariant& data, const Qt::CheckState checkState = Qt::Unchecked);

    QVariantList selectedDatas() const;
    QList<int> selectedInts() const;

public slots:
    void haeMerkkaukset(const QDate& paivalle = QDate());
    void setSelectedItems(const QList<int>& list);
    void setSelectedItems(const QVariantList& list);
    void haeRyhmat();

private slots:
    void onModelDataChanged();
    void onItemPressed(const QModelIndex& index);
    void updateText();

protected:
    bool eventFilter(QObject *object, QEvent* event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QStandardItemModel* model_;

public:
    class CheckComboStyledItemDelegate : public QStyledItemDelegate
    {
    public:
        CheckComboStyledItemDelegate(QObject *parent = nullptr);

        void paint(QPainter * painter_, const QStyleOptionViewItem & option_, const QModelIndex & index_) const;
    };
};

#endif // CHECKCOMBO_H
