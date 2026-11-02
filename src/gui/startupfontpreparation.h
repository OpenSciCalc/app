// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_STARTUPFONTPREPARATION_H
#define GUI_STARTUPFONTPREPARATION_H

#include "core/startupdiagnostics.h"

#include <QFontDatabase>
#include <QThread>
#include <memory>

class StartupFontPreparation {
public:
    StartupFontPreparation()
    {
        // Some platforms create their font backend on first use without a lock.
        // Create it here, so the worker below cannot race the UI thread for it.
        // This call does not load the font list.
        QFontDatabase::standardSizes();

        // The first text widget otherwise loads the font database on the UI
        // thread. QFontDatabase is thread-safe, so overlap that work with window
        // construction after QApplication has initialized. The UI thread waits
        // inside Qt when it needs a font before the worker has finished.
        const auto task = StartupDiagnostics::task("Prepare font database (async total)");
        m_thread.reset(QThread::create([task]() {
            {
                StartupDiagnostics::Scope timing("Prepare font database");
                QFontDatabase::families();
            }
            if (task)
                task->finish();
        }));
        m_thread->start();
    }

    ~StartupFontPreparation() { wait(); }

    void wait()
    {
        if (!m_thread)
            return;
        StartupDiagnostics::Scope timing("Wait for font preparation");
        m_thread->wait();
        m_thread.reset();
    }

private:
    std::unique_ptr<QThread> m_thread;
};

#endif
