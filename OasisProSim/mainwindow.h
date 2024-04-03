#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtGlobal>
#include "sessionmngr.h"
#include <string>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    //member var
    bool isOn;
    bool isConnected;
    double batteryLife;
    bool sessionInProgress;
    int currIntensity;
    int defaultIntensity;
    QString currUser;
    int blinkCount;

    //keeps track of if the user has preiously been warned out the battery, resets when device is turned off
    bool batteryWarningGiven;
    bool criticalBatteryWarningGiven;

    //members
    sessionMngr* mngr;
    QTimer* idleTimer;
    //timer used for animation of soft on and off
    QTimer* softOffTimer;
    QTimer* softOnTimer;
    //timer used for animation when the battery is low
    QTimer* lowBatteryTimer;

    //member methods
    bool connectionTest();
    void changeInstensityDisplay(); //function to decouple intensity change logic from display
    int getTimeSelection();
    void setUserSessions();
    void noConnectionBlink();

private slots:
    void togglePwr(); //turns device on and off

public slots:
    void toggleElectrodes();
    void idleTimerExpired(); //shuts device off if no session is started for 2 minutes after power on
    void batteryLifeTimerTick(); //gradually decreases the remaining battery life
    void updateConnection();
    void onSessionStart(); //lets the sessionmanager know a session started and is requested to be recorded
    void softOn();
    void onSessionEnd(); //lets the sessionmanager know a session ended and is requested to be recorded
    void softOff();
    void checkButtonPress(); //start session button
    void changeInstensity(); //up and down intensity buttons
    void changeInstensityAdmin(int); //spin box in admin intensity change
    void setDefaultIntensity();
    void rechargeBattery();
    void changeUser(QString user);
    void setReplayValues(); //sets the device values to a selected recording's values
    void batteryBlink(); //used for animation when battery is low
    void noConnectionTestBlink(); //used for animation when no connection
    void okayConnectionTest(); //used for animation when okay connection
    void resetConnectionTest(); //used for animation when displaying connection status


};
#endif // MAINWINDOW_H
