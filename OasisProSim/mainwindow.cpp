#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //setup timers
    idleTimer = new QTimer();
    softOffTimer = new QTimer();
    softOnTimer = new QTimer();
    lowBatteryTimer = new QTimer();

    //create manager
    mngr = new sessionMngr(this);

    //connect slots
    connect(ui->powerButton, SIGNAL(released()), this, SLOT(togglePwr()));
    connect(idleTimer, SIGNAL(timeout()), this, SLOT(idleTimerExpired()));
    connect(mngr->batteryLifeTimer, SIGNAL(timeout()), this, SLOT(batteryLifeTimerTick()));
    connect(ui->adminConnectedComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateConnection()));
    connect(ui->connectElectrodeButton, SIGNAL(released()), this, SLOT(toggleElectrodes()));
    connect(ui->checkButton, SIGNAL(released()), this, SLOT(checkButtonPress()));
    connect(ui->upIntButton, SIGNAL(released()), this, SLOT(changeInstensity()));
    connect(ui->downIntButton, SIGNAL(released()),this, SLOT(changeInstensity()));
    connect(ui->adminIntensityLevelspinBox, SIGNAL(valueChanged(int)), SLOT(changeInstensityAdmin(int)));
    connect(ui->setDefaultIntensityButton, SIGNAL(released()), this, SLOT(setDefaultIntensity()));
    connect(softOffTimer, SIGNAL(timeout()), this, SLOT(softOff()));
    connect(softOnTimer, SIGNAL(timeout()), this, SLOT(softOn()));
    connect(ui->adminBatteryRecharge, SIGNAL(released()), this, SLOT(rechargeBattery()));
    connect(ui->adminSelectUserComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeUser(QString)));
    connect(ui->selectReplayButton, SIGNAL(released()), this, SLOT(setReplayValues()));
    connect(lowBatteryTimer, SIGNAL(timeout()), this, SLOT(batteryBlink()));

    //connect siganls from sessionMngr to slots
    connect(mngr,SIGNAL(sessionStart()),this,SLOT (onSessionStart()));
    connect(mngr,SIGNAL(sessionEnd()),this,SLOT (onSessionEnd()));

    //init buttons / displays / battery
    isOn = false;
    sessionInProgress = false;
    isConnected = false;
    currUser = ui->adminSelectUserComboBox->currentText();
    setUserSessions();
    batteryLife = 99.99;
    currIntensity = 0;
    blinkCount = 0;
    defaultIntensity = 1; //all sessions will set their intenstiy to this value unless changed manually
    ui->adminBatterySpinBox->setValue(batteryLife);
    //init with start session button and increase intensity spin box off
    ui->checkButton->setDisabled(true);
    ui->adminIntensityLevelspinBox->setDisabled(true);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete mngr;
}

void MainWindow::togglePwr(){
    //if the device is off and has power remaining, activate the power button light and set "isOn" true, start the idle timer and battery progress
    if (!isOn){
        if (batteryLife<=0){
            qInfo("Battery dead, Please charge");
        }else{
            qInfo("Turning ontrue;");
            //enable start session button and increase intensity spin box since device turned on
            ui->checkButton->setEnabled(true);
            ui->adminIntensityLevelspinBox->setEnabled(true);
            isOn=true;
            ui->powrButtonLight->setStyleSheet("background-color:green");
            idleTimer->start(120000); //this will be set to 2 minutes
        }
    }else {
        //battery timer blinks only occur once per session
        batteryWarningGiven = criticalBatteryWarningGiven = false;
        //disable start session button and intensity spin box since device turned off
        ui->checkButton->setDisabled(true);
        ui->adminIntensityLevelspinBox->setDisabled(true);
        //if a session is in progress, end it properly before turning off
        if (sessionInProgress){
            mngr->endSession(); //will need to turn battery tick timer off in this function

        } else {
            qInfo("Turning off");
            isOn=false;
            changeInstensity();
            ui->powrButtonLight->setStyleSheet("background-color:white");
            idleTimer->stop();
        }
    }
}

void MainWindow::idleTimerExpired(){
    //we will impliment such that when a session is started, this timer is stopped so this function will not be called
    qInfo("Idle timer expired");
    togglePwr();
}

void MainWindow::batteryLifeTimerTick(){
    batteryLife -= (0.01 + (0.01 * currIntensity));
    //update the ui and handle dead battery behaviour
    ui->adminBatterySpinBox->setValue(batteryLife);

    //critical low battery blink
    if (batteryLife <= 10 && !criticalBatteryWarningGiven) {
        qInfo() << "Critical battery power blinking";
        lowBatteryTimer->start(1000);
        criticalBatteryWarningGiven = true;
    }
    //regular low battery blink
    else if (batteryLife <= 20 && !batteryWarningGiven) {
        qInfo() << "Low battery power blinking";
        lowBatteryTimer->start(1000);
        batteryWarningGiven = true;
    }
    else if (batteryLife<=0){
        qInfo("Battery has died");
        togglePwr();
        mngr->batteryLifeTimer->stop(); //replace once endSession() is done, should probably be done in sessionmngr
        return;
    }

}

void MainWindow::batteryBlink() {
    if (blinkCount < 5) {
        blinkCount++;
        ui->top2Graphs->setValue(0);
        ui->middle3Graphs->setValue(0);
        if (batteryLife <= 10) {
            ui->bottom3Graphs->setValue(1);
            QTimer::singleShot(500, this, SLOT(changeInstensity()));
        }
        else {
            ui->bottom3Graphs->setValue(2);
            QTimer::singleShot(500, this, SLOT(changeInstensity()));
        }
    }
    else {
        lowBatteryTimer->stop();
        changeInstensityDisplay();
        blinkCount = 0;
    }
}


void MainWindow::toggleElectrodes() {
    if (isConnected) {
        ui->adminConnectedComboBox->setCurrentIndex(1);
    }
    else {
        ui->adminConnectedComboBox->setCurrentIndex(0);
    }
}

void MainWindow::updateConnection(){
    //reconnect ear clips from paused session
    if(mngr->isSessionPaused() && ui->adminConnectedComboBox->currentIndex() == 0) {
        isConnected = true;
        mngr->unpauseSession();
    }
    //session is running and user disconnects ear clips
    else if (sessionInProgress && ui->adminConnectedComboBox->currentIndex() == 1){
        isConnected = false;
        noConnectionBlink();
        mngr->pauseSession();
    }
    else if (ui->adminConnectedComboBox->currentIndex() == 0) {
        isConnected = true;;
    }
    else{
        isConnected = false;
    }
}

void MainWindow::changeUser(QString user) {
    currUser = user;
    setUserSessions();
}

void MainWindow::setUserSessions() {
    QStringList userSessions =  mngr->getUserSessions(currUser);
    ui->replaysDropdown->clear();
    ui->replaysDropdown->addItems(userSessions);
}

void MainWindow::onSessionStart(){
    //the signal from session manager has informed the main window of session start, so update the members and stop the idleTimer
    sessionInProgress=true;
    idleTimer->stop();
    currIntensity=0;
    softOnTimer->start(500);
}

void MainWindow::setReplayValues() {
    QString sessionNum = ui->replaysDropdown->currentText().at(0);
    Session* tempSession = mngr->getSession(sessionNum.toInt());
    ui->sessionSelectionComboBox->setCurrentText(Session::sessionTypes.at(tempSession->getType()));
    if (tempSession->getDuration() == 20){
        ui->timeSelectionComboBox->setCurrentIndex(0);
    }
    else if(tempSession->getDuration() == 45) {
        ui->timeSelectionComboBox->setCurrentIndex(1);
    }
    else {
        ui->timeSelectionComboBox->setCurrentIndex(2);
        ui->userDesignatedSpinBox->setValue(tempSession->getDuration());
    }
    defaultIntensity = tempSession->getIntensity();
    qInfo("Recording values set, start a new session as normal");

}

void MainWindow::softOn(){
    if (currIntensity==defaultIntensity){
        changeInstensity();
        softOnTimer->stop();
        qInfo("soft on complete, enjoy the session");
    }else{
        changeInstensity();
        currIntensity++;
    }
}


void MainWindow::onSessionEnd(){
    //add the record to the database if the user choses to
    if (ui->adminRecordReplayCheckBox->isChecked()){
        mngr->addSessionRecord(currIntensity);
        setUserSessions();
    }
    ui->adminSelectUserComboBox->setEnabled(true);
    //finally start the soft off process
    currIntensity=8; //the intensity associated with the session will have already been saved, this is soley to visually show the soft off process
    softOffTimer->start(500);
}

void MainWindow::softOff(){
    if (currIntensity==0){
        qInfo("soft off complete, powering off");
        changeInstensity();
        softOffTimer->stop();
        sessionInProgress=false;
        isOn=false;
        ui->powrButtonLight->setStyleSheet("background-color:white");
    } else {
        changeInstensity();
        currIntensity--;
    }
}

int MainWindow::getTimeSelection() {
    if (ui->timeSelectionComboBox->currentIndex() == 0){
        return 20;
    }
    else if(ui->timeSelectionComboBox->currentIndex() == 1) {
        return 45;
    }
    else {
        return ui->userDesignatedSpinBox->value();
    }
}

void MainWindow::checkButtonPress(){
    //TO DO: we need to actually adjust the values here to pass
    if (!sessionInProgress && connectionTest() && isOn) {
        int sessionType = ui->sessionSelectionComboBox->currentIndex();
        int duration = getTimeSelection();
        mngr->startSession(ui->adminSelectUserComboBox->currentText(), sessionType, duration, defaultIntensity);
        ui->adminSelectUserComboBox->setDisabled(true);
    }
}

bool MainWindow::connectionTest(){
    //need to expand on this later by displaying according to the manual
    //inform console of connection state
    QString connectionResult;
    if (isConnected) {
        connectionResult = "device probes are connected";
        QTimer::singleShot(0, this, SLOT(okayConnectionTest()));
    }
    else {
        connectionResult = "not connected, try again";
        noConnectionBlink();
    }
    qInfo() << connectionResult;
    return isConnected;
}

void MainWindow::noConnectionBlink() {
    QTimer::singleShot(0, this, SLOT(noConnectionTestBlink()));
    QTimer::singleShot(500, this, SLOT(resetConnectionTest()));
    QTimer::singleShot(1000, this, SLOT(noConnectionTestBlink()));
    QTimer::singleShot(1500, this, SLOT(resetConnectionTest()));
    QTimer::singleShot(2000, this, SLOT(noConnectionTestBlink()));
    QTimer::singleShot(2500, this, SLOT(resetConnectionTest()));
}

void MainWindow::noConnectionTestBlink() {
    ui->bottom3Graphs->setValue(0);
    ui->middle3Graphs->setValue(0);
    ui->top2Graphs->setValue(2);
}

void MainWindow::okayConnectionTest() {
    ui->bottom3Graphs->setValue(3);
}

void MainWindow::resetConnectionTest() {
    changeInstensityDisplay();
}

void MainWindow::changeInstensity(){
    if (isOn){
        if (sender()==ui->upIntButton){
            if (currIntensity<8){
                currIntensity++;
            }
        }
        else if (sender()==ui->downIntButton){
            if (currIntensity>0){
                currIntensity--;
            }
        }
        //now set the three graphs on the UI to match the intensity
        changeInstensityDisplay();

        //block signal temporarily while setting new intensity value (prevents intensity setting loop between spin box and buttons)
        ui->adminIntensityLevelspinBox->blockSignals(true);
        ui->adminIntensityLevelspinBox->setValue(currIntensity);
        ui->adminIntensityLevelspinBox->blockSignals(false);
    }
    else { //this segment will reset the graphs to 0 when someone turns off the device suddenly and a session is NOT active
        currIntensity=0;
        ui->adminIntensityLevelspinBox->setValue(currIntensity);
        ui->bottom3Graphs->setValue(0);
        ui->middle3Graphs->setValue(0);
        ui->top2Graphs->setValue(0);
    }
}

void MainWindow::changeInstensityDisplay() {
    if (currIntensity<4){
        ui->top2Graphs->setValue(0);
        ui->middle3Graphs->setValue(0);
        ui->bottom3Graphs->setValue(currIntensity);
    }
    else if (currIntensity<7){
        ui->bottom3Graphs->setValue(3);
        ui->middle3Graphs->setValue(currIntensity-3);
        ui->top2Graphs->setValue(0);
    }
    else {
        ui->bottom3Graphs->setValue(3);
        ui->middle3Graphs->setValue(3);
        ui->top2Graphs->setValue(currIntensity-6);
    }
}

void MainWindow::changeInstensityAdmin(int newVal) {
    if (newVal > currIntensity){
        currIntensity++;
    }
    else {
        currIntensity--;
    }
    changeInstensityDisplay();
}

void MainWindow::setDefaultIntensity(){
    defaultIntensity = currIntensity;
    qInfo("Default intensity updated to current intensity");
}

void MainWindow::rechargeBattery() {
    batteryLife = 99.99;
    ui->adminBatterySpinBox->setValue(batteryLife);
    qInfo() << "Battery recharged";
}
