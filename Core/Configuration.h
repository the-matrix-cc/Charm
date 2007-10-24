#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>

#include "User.h"

class QSettings;

class Configuration
{
public:
    // only append to that, to no break old configurations:
    enum TaskTrackerFontSize {
        TaskTrackerFont_Small,
        TaskTrackerFont_Regular,
        TaskTrackerFont_Large
    };

    static Configuration& instance();

    void writeTo( QSettings& );
    // read the configuration from the settings object
    // this will not modify the settings group etc but just use it
    // returns true if all individual settings have been found (the
    // configuration is complete)
    bool readFrom( QSettings& );

    // helper method
    void dump( const QString& why = QString::null );

    // database metainformation
    bool eventsInLeafsOnly;
    bool oneEventAtATime;

    QString configurationName;
    int installationId;
    User user;  // this user's id
    QString localStorageType; // SqLite, MySql, ...
    QString localStorageDatabase; // database name (path, with sqlite)
    bool newDatabase; // true if the configuration has just been created
    bool failure; // used to reconfigure on failures
    QString failureMessage; // a message to show the user if something is wrong with the configuration

    // appearance properties
    int taskPaddingLength; // auto-determined
    bool showOnlySubscribedTasks;
    TaskTrackerFontSize taskTrackerFontSize;
    bool always24hEditing;

private:
    // allow test classes to create configuration objects (tests are
    // the only  application that can have (test) multiple
    // configurations):
    friend class SqLiteStorageTests;

    Configuration();

};


#endif