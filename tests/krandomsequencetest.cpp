/* This file is part of the KDE libraries
    Copyright (c) 1999 Waldo Bastian <bastian@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QList>
#include <QString>

#include "krandomsequence.h"
#include "krandom.h"

#include <stdio.h>

int
main(/*int argc, char *argv[]*/)
{
    long seed;
    KRandomSequence seq;

    seed = 2;
    seq.setSeed(seed); printf("Seed = %4ld :", seed);
    for (int i = 0; i < 20; i++) {
        printf("%3ld ", seq.getLong(100));
    }
    printf("\n");

    seed = 0;
    seq.setSeed(seed); printf("Seed = %4ld :", seed);
    for (int i = 0; i < 20; i++) {
        printf("%3ld ", seq.getLong(100));
    }
    printf("\n");

    seed = 0;
    seq.setSeed(seed); printf("Seed = %4ld :", seed);
    for (int i = 0; i < 20; i++) {
        printf("%3ld ", seq.getLong(100));
    }
    printf("\n");

    seed = 2;
    seq.setSeed(seed); printf("Seed = %4ld :", seed);
    for (int i = 0; i < 20; i++) {
        printf("%3ld ", seq.getLong(100));
    }
    printf("\n");

    seq.setSeed(KRandom::random());

    QList<QString> list;
    list.append(QLatin1String("A"));
    list.append(QLatin1String("B"));
    list.append(QLatin1String("C"));
    list.append(QLatin1String("D"));
    list.append(QLatin1String("E"));
    list.append(QLatin1String("F"));
    list.append(QLatin1String("G"));

    for (QList<QString>::Iterator str = list.begin(); str != list.end(); ++str) {
        printf("%s", str->toLatin1().data());
    }
    printf("\n");

    seq.randomize(list);

    for (QList<QString>::Iterator str = list.begin(); str != list.end(); ++str) {
        printf("%s", str->toLatin1().data());
    }
    printf("\n");

    seq.randomize(list);

    for (QList<QString>::Iterator str = list.begin(); str != list.end(); ++str) {
        printf("%s", str->toLatin1().data());
    }
    printf("\n");

    seq.randomize(list);

    for (QList<QString>::Iterator str = list.begin(); str != list.end(); ++str) {
        printf("%s", str->toLatin1().data());
    }
    printf("\n");
}
