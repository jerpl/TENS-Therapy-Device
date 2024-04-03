#ifndef SESSION_H
#define SESSION_H

#include <QString>

class Session
{
public:
    Session(const QString user, int type, int dur, int intensity);
    void setIntensity(int intensity);
    void setUser(QString user);
    void setDuration(int dur);
    void setType(int type);
    int getType();
    int getIntensity();
    int getDuration();
    QString getUser();
    void print();
    const static QStringList sessionTypes;

private:
    QString user;
    int duration;
    int intensity;
    int type;
};

#endif // SESSION_H
