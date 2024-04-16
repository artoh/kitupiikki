#ifndef BADGES_H
#define BADGES_H

#include <QStringList>

class Badges
{
public:
    enum Badge {
        INBOX =         0b0000001,
        OUTBOX =        0b0000010,
        MARKED=         0b0000100,
        INFORMATION=    0b0001000,
        NOTIFICATION=   0b0010000,
        ERROR=          0b0100000,
        WARN=           0b1000000,
        DRAFT=         0b10000000,
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
