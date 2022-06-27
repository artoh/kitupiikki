#ifndef KPYHEDIT_H
#define KPYHEDIT_H

#include <QLineEdit>


/**
 * @brief Yksikköhinnan editori
 */


class KpYhEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpYhEdit(QWidget* parent = nullptr);

    void setValue(const double value);
    double value() const;

    void setText(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* event) override;

protected:
    int desimaalit_ = 3;
};

#endif // KPYHEDIT_H
