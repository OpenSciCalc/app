// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_DIAGNOSTICAPPLICATION_H
#define GUI_DIAGNOSTICAPPLICATION_H

#include "core/startupdiagnostics.h"
#include "gui/mainwindow.h"
#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QEvent>
#include <QThread>
#include <QTimer>
#include <type_traits>

class RecordingDiagnosticApplication : public QApplication {
public:
    RecordingDiagnosticApplication(int& argc, char** argv) : QApplication(argc, argv)
    {
        connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::aboutToBlock,
                this, []() { StartupDiagnostics::finishWhenIdle(); });
    }

    bool notify(QObject* receiver, QEvent* event) override
    {
        if (!StartupDiagnostics::isRecording())
            return QApplication::notify(receiver, event);
        // Other threads deliver their own events and are not counted here.
        if (QThread::currentThread() != thread())
            return QApplication::notify(receiver, event);
        // An event that the running event loop delivers directly. Events sent while
        // handling it, or before exec() starts, belong to the phase that sent them.
        const bool loopEvent = m_eventDepth == 0 && thread()->loopLevel() > 0;
        ++m_eventDepth;
        const bool result = notifyRecorded(receiver, event, loopEvent);
        --m_eventDepth;
        return result;
    }

private:
    bool notifyRecorded(QObject* receiver, QEvent* event, bool loopEvent)
    {
        if (event->type() == QEvent::LayoutRequest) {
            StartupDiagnostics::Scope timing("Process startup layout requests");
            return QApplication::notify(receiver, event);
        }
        if (event->type() != QEvent::Paint) {
            if (!loopEvent)
                return QApplication::notify(receiver, event);
            StartupDiagnostics::Scope timing(loopEventPhase(event));
            return QApplication::notify(receiver, event);
        }
        StartupDiagnostics::Scope timing("Paint startup widgets");
        const bool result = QApplication::notify(receiver, event);
        QWidget* widget = qobject_cast<QWidget*>(receiver);
        if (!m_firstPaintQueued && widget && widget->isVisible()
            && qobject_cast<MainWindow*>(widget->window())) {
            m_firstPaintQueued = true;
            QTimer::singleShot(0, this, []() { StartupDiagnostics::firstPaintFinished(); });
        }
        return result;
    }

    // Paints and layout requests keep their own phases above.
    static const char* loopEventPhase(const QEvent* event)
    {
        if (event->spontaneous())
            return "Dispatch startup window system events";
        switch (event->type()) {
        case QEvent::MetaCall:
            return "Run queued startup calls";
        case QEvent::Timer:
            return "Run startup timers";
        case QEvent::Polish:
        case QEvent::PolishRequest:
            return "Polish startup widgets";
        case QEvent::UpdateRequest:
            return "Process startup update requests";
        default:
            return "Dispatch other startup events";
        }
    }

    bool m_firstPaintQueued = false;
    int m_eventDepth = 0;
};

using DiagnosticApplication = std::conditional_t<StartupDiagnostics::isEnabled(),
    RecordingDiagnosticApplication, QApplication>;

#endif
