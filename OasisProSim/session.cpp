#include "session.h"
#include <QDebug>

const QStringList Session::sessionTypes = {"Delta", "Theta", "Alpha", "Beta 1"};


Session::Session(QString user, int type, int duration, int intensity):
    user(user), type(type), duration(duration), intensity(intensity) {}

void Session::print(){
    qInfo() << "User: " << user;
    qInfo() << "Session Type: " << sessionTypes.at(type);
    qInfo() << "Duration: " << duration;
    qInfo() << "Intensity: " << intensity;
}

void Session::setIntensity(int newIntensity){
    intensity = newIntensity;
}

void Session::setDuration(int newDuration){
    duration = newDuration;
}

int Session::getType(){
    return type;
}

void Session::setType(int newType){
    type = newType;
}

int Session::getIntensity(){
    return intensity;
}

int Session::getDuration(){
    return duration;
}

void Session::setUser(QString newUser) {
    user = newUser;
}

QString Session::getUser() {
    return user;
}
