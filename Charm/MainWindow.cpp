#include <algorithm>

#include <QSettings>

#include <Core/CharmCommand.h>
#include <Core/CharmConstants.h>

#include "Data.h"
#include "MainWindow.h"
#include "ViewHelpers.h"
#include "Application.h"
#include "CharmAboutDialog.h"
#include "Commands/CommandRelayCommand.h"
#include "Reports/ReportConfigurationPage.h"
#include "CharmPreferences.h"

#include "ui_MainWindow.h"

MainWindow::MainWindow()
    : QMainWindow()
    , m_ui( new Ui::MainWindow() )
    , m_viewActionsGroup( this )
    , m_actionShowHideView( this )
    , m_actionQuit( this )
    , m_actionAboutDialog( this )
    , m_actionPreferences( this )
    , m_actionEventEditor( &m_viewActionsGroup )
    , m_actionTasksView( &m_viewActionsGroup )
    , m_actionToggleView( this )
    , m_eventEditor( this )
    , m_actionReporting( this )
    , m_reportDialog( this )
{
    m_ui->setupUi( this );

    // view corrections:
    centralWidget()->layout()->setMargin( 0 );
    centralWidget()->layout()->setSpacing( 0 );
    m_ui->viewStack->layout()->setMargin( 0 );
    m_ui->viewStack->layout()->setSpacing( 0 );

    // "register" view modes:
    m_modes << &m_tasksView << &m_eventEditor;
    m_ui->viewStack->addWidget( &m_tasksView );
    m_ui->viewStack->addWidget( &m_eventEditor );
    Q_FOREACH( ViewModeInterface* mode, m_modes ) {
        QWidget* modeWidget = dynamic_cast<QWidget*>( mode );
        Q_ASSERT( modeWidget );
        if ( modeWidget ) {
            connect( modeWidget, SIGNAL( emitCommand( CharmCommand* ) ),
                     SLOT( sendCommand( CharmCommand* ) ) );
        }
    }

    // set up actions
    connect( &m_actionShowHideView, SIGNAL( triggered() ),
             SLOT( slotShowHideView() ) );
    m_actionQuit.setText( tr( "Quit" ) );
    m_actionQuit.setIcon( Data::quitCharmIcon() );
    !connect( &m_actionQuit, SIGNAL( triggered( bool ) ),
             SLOT( slotQuit() ) );

    m_actionAboutDialog.setText( tr( "About Charm" ) );
    connect( &m_actionAboutDialog, SIGNAL( triggered() ),
             SLOT( slotAboutDialog() ) );

    m_actionPreferences.setText( tr( "Preferences" ) );
    m_actionPreferences.setIcon( Data::configureIcon() );
    connect( &m_actionPreferences, SIGNAL( triggered( bool ) ),
             SLOT( slotEditPreferences( bool ) ) );

    // m_actionEventEditor.setCheckable( true );
//     m_eventEditor.setVisible( m_actionEventEditor.isChecked() );
//     connect( &m_actionEventEditor, SIGNAL( toggled( bool ) ),
//              &m_eventEditor, SLOT( setVisible( bool ) ) );
//     connect( &m_eventEditor, SIGNAL( visible( bool ) ),
//              &m_actionEventEditor, SLOT( setChecked( bool ) ) );

    // set up Charm menu:
    QMenu* appMenu = new QMenu( tr( "Charm" ), menuBar() );
    appMenu->addAction( &m_actionPreferences );
    m_actionPreferences.setEnabled( true );
    appMenu->addAction( &m_actionAboutDialog );
    appMenu->addAction( &m_actionQuit );

    // set up view menu:
    m_actionReporting.setText( tr( "Reports..." ) );
    connect( &m_actionReporting, SIGNAL( triggered() ),
             SLOT( slotReportDialog() ) );

    m_actionEventEditor.setText( tr( "Event Editor" ) );
    m_actionTasksView.setText( tr( "Tasks View" ) );
    Q_FOREACH( QAction* action, m_viewActionsGroup.actions() ) {
        action->setCheckable( true );
    }
    m_actionTasksView.setShortcut( Qt::CTRL + Qt::Key_T );
    m_actionEventEditor.setShortcut( Qt::CTRL + Qt::Key_E );
    m_actionReporting.setShortcut( Qt::CTRL + Qt::Key_R );
    // set default view mode:
    // FIXME restore view selection from settings:
    m_actionTasksView.setChecked( true );
    slotSelectViewMode( &m_actionTasksView );
    connect( &m_viewActionsGroup, SIGNAL( triggered( QAction* ) ),
             SLOT( slotSelectViewMode( QAction* ) ) );

    QMenu* viewMenu = new QMenu( tr( "View" ), menuBar() );
    viewMenu->addActions( m_viewActionsGroup.actions() );
    viewMenu->addSeparator();
    m_actionToggleView.setShortcut( Qt::CTRL + Qt::Key_V );
    m_actionToggleView.setText( tr( "Toggle View" ) );
    connect( &m_actionToggleView, SIGNAL( triggered() ), SLOT( slotToggleView() ) );
    viewMenu->addAction( &m_actionToggleView );
    viewMenu->addSeparator();
    viewMenu->addAction( &m_actionReporting );

    // FIXME this might be a context-sensitive view-mode-menu:
//     QMenu* taskMenu = new QMenu ( tr( "Task" ), menuBar() );
//     taskMenu->addAction( &m_actionEventStarted );
//     taskMenu->addAction( &m_actionEventEnded );
    menuBar()->addMenu( appMenu );
    menuBar()->addMenu( viewMenu );

    // system tray icon:
    m_trayIcon.setIcon( Data::charmIcon() );
    m_trayIcon.show();
    connect( &m_trayIcon, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
             SLOT( slotTrayIconActivated( QSystemTrayIcon::ActivationReason ) ) );
    m_systrayContextMenu.addAction( &m_actionShowHideView );
    m_systrayContextMenu.addAction( m_tasksView.actionStopAllTasks() );
    m_systrayContextMenu.addSeparator();
    m_systrayContextMenu.addAction( &m_actionQuit );
    m_trayIcon.setContextMenu( &m_systrayContextMenu );
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::restore()
{
    show();
}

void MainWindow::slotQuit()
{
    // this saves changes:
    m_eventEditor.close();
    m_reportDialog.close();
    emit quit();
}

void MainWindow::slotEditPreferences( bool )
{
    CharmPreferences dialog( CONFIGURATION, this );

    if ( dialog.exec() ) {
        CONFIGURATION.eventsInLeafsOnly = dialog.eventsInLeafsOnly();
        CONFIGURATION.oneEventAtATime = dialog.oneEventAtATime();
        CONFIGURATION.taskTrackerFontSize = dialog.taskTrackerFontSize();
        CONFIGURATION.always24hEditing = dialog.always24hEditing();
        slotConfigurationChanged();
        emit saveConfiguration();
    }
}

void MainWindow::slotShowHideView()
{   // hide or restore the view
    if ( isVisible() ) {
        hide();
        m_reportDialog.hide();
    } else {
        restore();
    }
}

void MainWindow::stateChanged( State previous )
{
    Q_FOREACH( ViewModeInterface* mode, m_modes ) {
        if ( previous == Constructed ) {
            mode->setModel( & Application::instance().model() );
        }
        mode->stateChanged( previous );
    }

    switch( Application::instance().state() ) {
    case Connected:
        // FIXME remember in Gui state:
        m_ui->viewStack->setCurrentWidget( &m_tasksView );
        setEnabled( true );
        break;
    case Disconnecting:
        setEnabled( false );
        saveGuiState();
        m_ui->viewStack->setCurrentWidget( m_ui->openingPage );
        break;
        // fallthrough intended
    case StartingUp:
        m_ui->viewStack->setCurrentWidget( m_ui->openingPage );
        restoreGuiState();
        break;
    case ShuttingDown:
    case Dead:
    case Connecting:
        setEnabled( false );
        break;
    default:
        break;
    };
}

void MainWindow::commitCommand( CharmCommand* command )
{
    command->finalize();
    delete command;
}

void MainWindow::sendCommand( CharmCommand *cmd)
{
    cmd->prepare();
    CommandRelayCommand* relay = new CommandRelayCommand( this );
    relay->setCommand( cmd );
    emit emitCommand( relay );
}

void MainWindow::slotTrayIconActivated( QSystemTrayIcon::ActivationReason reason )
{
    switch( reason ) {
    case QSystemTrayIcon::Context:
        // show context menu
        // m_contextMenu.show();
        break;
    case QSystemTrayIcon::DoubleClick:
        slotShowHideView();
        break;
    case QSystemTrayIcon::Trigger:
        // single click
        break;
    case QSystemTrayIcon::MiddleClick:
        // ...
        break;
    case QSystemTrayIcon::Unknown:
    default:
        break;
    }
}

void MainWindow::showEvent( QShowEvent* )
{
    m_actionShowHideView.setText( tr( "Hide Charm Window" ) );
}

void MainWindow::hideEvent( QHideEvent* )
{
    m_actionShowHideView.setText( tr( "Show Charm Window" ) );
}

void MainWindow::slotAboutDialog()
{
    CharmAboutDialog dialog( this );
    dialog.exec();
}

void MainWindow::restoreGuiState()
{
    // restore geometry
    QSettings settings;
    if ( settings.contains( MetaKey_MainWindowGeometry ) ) {
        restoreGeometry( settings.value( MetaKey_MainWindowGeometry ).toByteArray() );
    }
    // call all the view modes:
    for_each( m_modes.begin(), m_modes.end(),
              std::mem_fun( &ViewModeInterface::restoreGuiState ) );
}

void MainWindow::saveGuiState()
{
    QSettings settings;
    // save geometry
    settings.setValue( MetaKey_MainWindowGeometry, saveGeometry() );
    // call all the view modes:
    for_each( m_modes.begin(), m_modes.end(),
              std::mem_fun( &ViewModeInterface::saveGuiState ) );
}

void MainWindow::slotConfigurationChanged()
{
    for_each( m_modes.begin(), m_modes.end(),
              std::mem_fun( &ViewModeInterface::configurationChanged ) );
}

void MainWindow::slotSelectViewMode( QAction* action )
{
    if ( action == &m_actionEventEditor ) {
        m_ui->viewStack->setCurrentWidget( &m_eventEditor );
    } else if ( action == &m_actionTasksView ) {
        m_ui->viewStack->setCurrentWidget( &m_tasksView );
    }
}

void MainWindow::slotCurrentBackendStatusChanged( const QString& text )
{
    QString title;
    QTextStream stream( &title );
    stream << "Charm ("
           << CONFIGURATION.user.name()
           << " - " << text << ")";

    setWindowTitle( title );
    m_trayIcon.setToolTip( title );
}

void MainWindow::slotReportDialog()
{
    m_reportDialog.back();
    if ( m_reportDialog.exec() ) {
        ReportConfigurationPage* page = m_reportDialog.selectedPage();
        qDebug() << "MainWindow::slotReportDialog: report of type"
                 << page->name() << "selected";
        QDialog* preview = page->makeReportPreviewDialog( this );
        // preview is destroy-on-close and non-modal:
        preview->show();
    }
}

void MainWindow::slotToggleView()
{
    ViewModeInterface* current = dynamic_cast<ViewModeInterface*>( m_ui->viewStack->currentWidget() );
    if ( current ) {
        Q_ASSERT( m_modes.size() == m_ui->viewStack->count() -1 ); // -1: startup page, never shown again
        QList<ViewModeInterface*>::iterator mode = std::find( m_modes.begin(), m_modes.end(), current );
        Q_ASSERT( mode != m_modes.end() ); // how could that be if m_modes is not empty?
        ++mode;
        if ( mode == m_modes.end() ) {
            mode = m_modes.begin();
        }
        QWidget* widget = dynamic_cast<QWidget*>( *mode );
        // baah:
        if ( widget == &m_eventEditor ) {
            m_actionEventEditor.trigger();
        } else if ( widget == &m_tasksView ) {
            m_actionTasksView.trigger();
        } else { // cannot happen:
            Q_ASSERT( widget && false );
        }
    }
}

#include "MainWindow.moc"
