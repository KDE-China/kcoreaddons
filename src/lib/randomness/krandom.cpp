/* This file is part of the KDE libraries
    Copyright (c) 1999 Matthias Kalle Dalheimer <kalle@kde.org>
    Copyright (c) 2000 Charles Samuels <charles@kde.org>
    Copyright (c) 2005 Joseph Wenninger <kde@jowenn.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
        */

#include "krandom.h"

#include <stdlib.h>
#ifdef Q_OS_WIN
#include <process.h>
#else // Q_OS_WIN
#include <unistd.h>
#endif // Q_OS_WIN
#include <stdio.h>
#include <time.h>
#ifndef Q_OS_WIN
#include <sys/time.h>
#endif //  Q_OS_WIN
#include <fcntl.h>

#include <QFile>
#include <QThread>
#include <QThreadStorage>

int KRandom::random()
{
    static QThreadStorage<bool> initialized_threads;
    if (!initialized_threads.localData()) {
        unsigned int seed;
        initialized_threads.setLocalData(true);
        QFile urandom(QStringLiteral("/dev/urandom"));
        bool opened = urandom.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!opened || urandom.read(reinterpret_cast<char *>(&seed), sizeof(seed)) != sizeof(seed)) {
            // No /dev/urandom... try something else.
            qsrand(getpid());
            seed = qrand() ^ time(nullptr) ^ reinterpret_cast<quintptr>(QThread::currentThread());
        }
        qsrand(seed);
    }
    return qrand();
}

QString KRandom::randomString(int length)
{
    if (length <= 0) {
        return QString();
    }

    QString str; str.resize(length);
    int i = 0;
    while (length--) {
        int r = random() % 62;
        r += 48;
        if (r > 57) {
            r += 7;
        }
        if (r > 90) {
            r += 6;
        }
        str[i++] =  char(r);
        // so what if I work backwards?
    }
    return str;
}
