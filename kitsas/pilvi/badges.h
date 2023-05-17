#ifndef BADGES_H
#define BADGES_H

#include <QStringList>

class Badges
{
public:
    enum Badge {
        INBOX =         0b000001,
        OUTBOX =        0b000010,
        MARKED=         0b000100,
        INFORMATION=    0b001000,
        NOTIFICATION=   0b010000,
        ERROR=          0b100000
    };

    Badges();
    Badges(const QStringList& badgeList);
    void set(const QStringList& badgeList);

    int badges() const;
    bool hasBagde(Badge badge) const;


private:
    int data_ = 0;


};

#endif // BADGES_H
