#include <QTest>

class TuontiTesti : public QObject
{
    Q_OBJECT

public:
    TuontiTesti();
    ~TuontiTesti();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void ibanTesti();
    void senttiTesti();
};
