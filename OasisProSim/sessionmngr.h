#ifndef SESSIONMNGR_H
#define SESSIONMNGR_H

#include <QObject>
#include "session.h"
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QList>
#include <QTimer>
#include <QtDebug>

class sessionMngr : public QObject
{
    Q_OBJECT
public:
    QTimer* batteryLifeTimer; //timer for ticking batter
    QTimer* sessionTimer; //overall session timer
    static const QString DATABASE_PATH;
    explicit sessionMngr(QObject *parent = nullptr);
    ~sessionMngr();
    bool isSessionPaused();
    int getRemainingTime();
    void startSession(QString user,int sessionType, int duration, int intensity); //called when a session is started and the user wants to record it
    void addSessionRecord(int currIntensity);//called when a session ends and the user wants to record it
    void addUserRecord(const QString &user);
    QStringList getUserSessions(QString user); //gets a user's recorded sessions for display
    void pauseSession();
    void unpauseSession();
    void endSession();
    Session* getCurrentSession(); //returns current session object
    Session* getSession(int sessionNum); //gets a session from the database when a user wants to use a recording
    bool deleteRecords();

private:
    bool DBInit();
    QSqlDatabase db;
    bool runningSession;
    bool sessionPaused;
    int remainingTime;
    Session *currSession;

public slots:
    void sessionTimerExpired();

signals:
    void sessionStart();
    void sessionEnd();
};

#endif // SESSIONMNGR_H
