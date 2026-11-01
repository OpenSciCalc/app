// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CORE_PROCESSLAUNCH_H
#define CORE_PROCESSLAUNCH_H

#include <QtGlobal>

// Startup phase timings begin at main(). The time before it covers process
// creation, library loading, security scans and static initialization. It is
// recorded in every build because it costs one system call.
namespace ProcessLaunch {

// Wall-clock nanoseconds since the OS created this process, or -1.
qint64 elapsedNs();

// Call first in main().
void recordMainEntry();

// Process launch to main(), or -1 when unrecorded or unavailable.
qint64 mainEntryNs();

}

#endif
