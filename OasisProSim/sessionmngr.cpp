#include "sessionmngr.h"
#include <QSqlError>

const QString sessionMngr::DATABASE_PATH = "/database/OasisPro.db";

sessionMngr::sessionMngr(QObject *parent) : QObject(parent) {
    batteryLifeTimer = new QTimer();
    sessionTimer = new QTimer();
    runningSession = false;
    sessionPaused = false;
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("OasisPro.db");
    if(!db.open()) {
        throw "Errow: Could not open database";
    }

    if(!DBInit()) {
        throw "Error: Database has not been initialized";
    }
    // add default values
    addUserRecord("User1");
    addUserRecord("User2");
    //connections
    connect(sessionTimer, SIGNAL(timeout()),this,SLOT (sessionTimerExpired()));
}

sessionMngr::~sessionMngr() {
    delete currSession;
}

Session* sessionMngr::getCurrentSession(){
    return currSession;
}

//DB Initialization. Creates a couple of tables to utlise later. Specifically a table for treatment history and a table to track users
bool sessionMngr::DBInit(){
    db.transaction();
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS treatmentHistory (sessionNum INTEGER PRIMARY KEY AUTOINCREMENT, user TEXT NOT NULL, sessionType INTEGER NOT NULL, duration INTEGER NOT NULL, intensityLevel INTEGER NOT NULL);");
    query.exec("CREATE TABLE IF NOT EXISTS users (name TEXT PRIMARY KEY);");
    return db.commit();
}


void sessionMngr::addSessionRecord(int intensityLevel){

    db.transaction();
    QSqlQuery query;
    query.prepare("INSERT INTO treatmentHistory (user, sessionType, duration, intensityLevel) VALUES (:user, :sessionType, :duration, :intensityLevel);");
    query.bindValue(":user", currSession->getUser());
    query.bindValue(":sessionType", currSession->getType());
    query.bindValue(":duration", currSession->getDuration());
    query.bindValue(":intensityLevel", intensityLevel);
    query.exec();
    db.commit();

}

void sessionMngr::addUserRecord(const QString &user){
    db.transaction();
    QSqlQuery query;
    query.prepare("INSERT INTO users (name) VALUES (:name);");
    query.bindValue(":name", user);
    query.exec();
    db.commit();
}

Session* sessionMngr::getSession(int sessionNum){

    db.transaction();
    QSqlQuery query;
    query.prepare("SELECT user, sessionType, duration, intensityLevel FROM treatmentHistory WHERE sessionNum = :sessionNum");

    query.bindValue(":sessionNum", sessionNum);
    query.exec();

    if (!db.commit()) {
        qInfo("querry did not work");
    }
    Session* ret = new Session("user", 0, 0, 0);
    if (query.next()) {
        const QString user = query.value(0).toString();
        int type = query.value(1).toInt();
        int dur = query.value(2).toInt();
        int intensity = query.value(3).toInt();
        ret->setUser(user);
        ret->setType(type);
        ret->setDuration(dur);
        ret->setIntensity(intensity);
    }
    return ret;
}

QStringList sessionMngr::getUserSessions(QString user) {
    db.transaction();
    QSqlQuery query;
    query.prepare("SELECT sessionNum, sessionType, duration, intensityLevel FROM treatmentHistory WHERE user = :user");
    query.bindValue(":user", user);
    query.exec();

    if (!db.commit()) {
        qInfo("querry did not work");
    }
    QStringList sessions;
    while (query.next()) {
        /*
        QString sessionNum = query.value(0).toString();
        QString type = query.value(1).toString();
        QString dur = query.value(2).toString();
        QString intensity = query.value(3).toString();
        */
        QString session = "";
        session.append(query.value(0).toString());
        session.append(" - Session Type: ");
        session.append(Session::sessionTypes.at(query.value(1).toInt()));
        session.append(" Duration: ");
        session.append(query.value(2).toString());
        session.append(" Intensity: ");
        session.append(query.value(3).toString());
        sessions.append(session);
    }
    return sessions;
}

bool sessionMngr::deleteRecords(){
    QSqlQuery query;
    query.prepare("DELETE FROM treatmentHistory");
    query.exec();
    query.prepare("DELETE FROM users");
    query.exec();
    return query.exec();
}



void sessionMngr::startSession(QString user, int sessionType, int duration, int intensity){

    qInfo("connection test worked");
    emit sessionStart();
    //creating session record at the start:
    currSession = new Session(user, sessionType, duration, intensity);
    //given duration * 1000 to convert to second
    sessionTimer->start(duration * 1000);
    batteryLifeTimer->start(1000); //the battery will decrease .01% every 1 seconds by default
}

// does not currently work need to change timer being used to a duration timer
void sessionMngr::pauseSession() {
    batteryLifeTimer->stop();
    remainingTime = sessionTimer->remainingTime();
    sessionTimer->stop();
    qInfo() << "Session paused with " << remainingTime << " left";
    sessionPaused = true;
}

void sessionMngr::unpauseSession() {
    qInfo("Session unpaused");
    batteryLifeTimer->start(1000);
    sessionTimer->start(remainingTime);
    sessionPaused = false;
}

bool sessionMngr::isSessionPaused(){
    return sessionPaused;
}

int sessionMngr::getRemainingTime() {
    return remainingTime;
}

void sessionMngr::sessionTimerExpired(){
    endSession();
}

//will be called when the session timer ends, or when the user pressed the power button during a session
void sessionMngr::endSession(){
    qInfo("ending session");
    emit sessionEnd();

    if (sender()==sessionTimer){
        currSession->setDuration(sessionTimer->interval());
    } else { //if the sender was not the session timer, it means the session was ended early, thus we must substract remaining time
        currSession->setDuration(sessionTimer->interval()-sessionTimer->remainingTime());
    }
    sessionTimer->stop();
    batteryLifeTimer->stop();

}
