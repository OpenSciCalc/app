// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/processlaunch.h"

#include <atomic>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(Q_OS_LINUX)
#include <QByteArray>
#include <QFile>
#include <QList>
#include <time.h>
#include <unistd.h>
#endif

namespace {
std::atomic<qint64> g_mainEntryNs(-1);
}

qint64 ProcessLaunch::elapsedNs()
{
#if defined(Q_OS_WIN)
    FILETIME created, exited, kernel, user, now;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        return -1;
    GetSystemTimePreciseAsFileTime(&now);
    const auto ticks = [](const FILETIME& time) {
        ULARGE_INTEGER value;
        value.LowPart = time.dwLowDateTime;
        value.HighPart = time.dwHighDateTime;
        return value.QuadPart;
    };
    const quint64 start = ticks(created);
    const quint64 current = ticks(now);
    // FILETIME counts 100 ns intervals.
    return current >= start ? qint64(current - start) * 100 : -1;
#elif defined(Q_OS_MACOS)
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    struct kinfo_proc info = {};
    size_t size = sizeof(info);
    if (sysctl(mib, 4, &info, &size, nullptr, 0) != 0 || size == 0)
        return -1;
    struct timeval now = {};
    if (gettimeofday(&now, nullptr) != 0)
        return -1;
    const struct timeval& start = info.kp_proc.p_starttime;
    const qint64 elapsedUs = (qint64(now.tv_sec) - start.tv_sec) * 1000000
        + (qint64(now.tv_usec) - start.tv_usec);
    return elapsedUs >= 0 ? elapsedUs * 1000 : -1;
#elif defined(Q_OS_LINUX)
    QFile file(QStringLiteral("/proc/self/stat"));
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray stat = file.readAll();
    // The command name can contain spaces. The fields after it start at field
    // 3, so the start time (field 22) has index 19. It counts clock ticks
    // since boot, usually 10 ms each.
    const int nameEnd = stat.lastIndexOf(')');
    if (nameEnd < 0)
        return -1;
    const QList<QByteArray> fields = stat.mid(nameEnd + 2).split(' ');
    bool ok = false;
    const qint64 startTicks = fields.value(19).toLongLong(&ok);
    const long ticksPerSecond = sysconf(_SC_CLK_TCK);
    struct timespec now = {};
    if (!ok || ticksPerSecond <= 0 || clock_gettime(CLOCK_BOOTTIME, &now) != 0)
        return -1;
    const qint64 nowNs = qint64(now.tv_sec) * 1000000000 + now.tv_nsec;
    const qint64 startNs = startTicks * 1000000000 / ticksPerSecond;
    return nowNs >= startNs ? nowNs - startNs : -1;
#else
    return -1;
#endif
}

void ProcessLaunch::recordMainEntry()
{
    g_mainEntryNs.store(elapsedNs());
}

qint64 ProcessLaunch::mainEntryNs()
{
    return g_mainEntryNs.load();
}
