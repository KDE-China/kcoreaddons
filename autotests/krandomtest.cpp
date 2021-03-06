/* This file is part of the KDE libraries
    Copyright (c) 2016 Michael Pyne <mpyne@kde.org>
    Copyright (c) 2016 Arne Spiegelhauer <gm2.asp@gmail.com>

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

#include <krandom.h>
#include <krandomsequence.h>
#include <stdlib.h>

#include <QtTest>

#include <QObject>
#include <QString>
#include <QRegExp>
#include <QVarLengthArray>
#include <QTextStream>
#include <QProcess>

#include <iostream>
#include <algorithm>

typedef QVarLengthArray<int> intSequenceType;

static const char *binpath;

static bool seqsAreEqual(const intSequenceType &l, const intSequenceType &r)
{
    if(l.size() != r.size()) {
        return false;
    }
    const intSequenceType::const_iterator last(l.end());

    intSequenceType::const_iterator l_first(l.begin());
    intSequenceType::const_iterator r_first(r.begin());

    while(l_first != last && *l_first == *r_first) {
        l_first++; r_first++;
    }

    return l_first == last;
}

// Fills seq with random bytes produced by a new process. Seq should already
// be sized to the needed amount of random numbers.
static bool getChildRandSeq(intSequenceType &seq)
{
    QProcess subtestProcess;

    // Launch a separate process to generate random numbers to test first-time
    // seeding.
    subtestProcess.start(QLatin1String(binpath), QStringList() << QString::number(seq.count()));
    subtestProcess.waitForFinished();

    QTextStream childStream(subtestProcess.readAllStandardOutput());

    std::generate(seq.begin(), seq.end(), [&]() {
            int temp; childStream >> temp; return temp;
            });

    char c;
    childStream >> c;
    return c == '\n' && childStream.status() == QTextStream::Ok;
}

class KRandomTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test_random();
    void test_randomString();
    void test_randomStringThreaded();
    void test_KRS();
};

void KRandomTest::test_random()
{
    int testValue = KRandom::random();

    QVERIFY(testValue >= 0);
    QVERIFY(testValue < RAND_MAX);

    // Verify seeding results in different numbers across different procs
    // See bug 362161
    intSequenceType out1(10), out2(10);

    QVERIFY(getChildRandSeq(out1));
    QVERIFY(getChildRandSeq(out2));

    QVERIFY(!seqsAreEqual(out1, out2));
}

void KRandomTest::test_randomString()
{
    const int desiredLength = 12;
    const QRegExp outputFormat(QStringLiteral("[A-Za-z0-9]+"));
    QString testString = KRandom::randomString(desiredLength);

    QCOMPARE(testString.length(), desiredLength);
    QVERIFY(outputFormat.exactMatch(testString));
}

void KRandomTest::test_KRS()
{
    using std::generate;
    using std::all_of;

    const int maxInt = 50000;
    KRandomSequence krs1, krs2;
    intSequenceType out1(10), out2(10);

    generate(out1.begin(), out1.end(), [&]() { return krs1.getInt(maxInt); });
    generate(out2.begin(), out2.end(), [&]() { return krs2.getInt(maxInt); });
    QVERIFY(!seqsAreEqual(out1, out2));
    QVERIFY(all_of(out1.begin(), out1.end(), [&](int x) { return x < maxInt; }));
    QVERIFY(all_of(out2.begin(), out2.end(), [&](int x) { return x < maxInt; }));

    // Compare same-seed
    krs1.setSeed(5000);
    krs2.setSeed(5000);

    generate(out1.begin(), out1.end(), [&]() { return krs1.getInt(maxInt); });
    generate(out2.begin(), out2.end(), [&]() { return krs2.getInt(maxInt); });
    QVERIFY(seqsAreEqual(out1, out2));
    QVERIFY(all_of(out1.begin(), out1.end(), [&](int x) { return x < maxInt; }));
    QVERIFY(all_of(out2.begin(), out2.end(), [&](int x) { return x < maxInt; }));

    // Compare same-seed and assignment ctor

    krs1 = KRandomSequence(8000);
    krs2 = KRandomSequence(8000);

    generate(out1.begin(), out1.end(), [&]() { return krs1.getInt(maxInt); });
    generate(out2.begin(), out2.end(), [&]() { return krs2.getInt(maxInt); });
    QVERIFY(seqsAreEqual(out1, out2));
    QVERIFY(all_of(out1.begin(), out1.end(), [&](int x) { return x < maxInt; }));
    QVERIFY(all_of(out2.begin(), out2.end(), [&](int x) { return x < maxInt; }));
}

class KRandomTestThread : public QThread
{
protected:
    void run() override
    {
        result = KRandom::randomString(32);
    };
public:
    QString result;
};

void KRandomTest::test_randomStringThreaded()
{
    static const int size = 5;
    KRandomTestThread* threads[size];
    for (int i=0; i < size; ++i) {
        threads[i] = new KRandomTestThread();
        threads[i]->start();
    }
    QSet<QString> results;
    for (int i=0; i < size; ++i) {
        threads[i]->wait(2000);
        results.insert(threads[i]->result);
    }
    // each thread should have returned a unique result
    QCOMPARE(results.size(), size);
    for (int i=0; i < size; ++i) {
        delete threads[i];
    }
}

// Used by getChildRandSeq... outputs random numbers to stdout and then
// exits the process.
static void childGenRandom(int count)
{
    // No logic to 300, just wanted to avoid it accidentally being 2.4B...
    if (count <= 0 || count > 300) {
        exit(-1);
    }

    while (--count > 0) {
        std::cout << KRandom::random() << ' ';
    }

    std::cout << KRandom::random() << '\n';
    exit(0);
}

// Manually implemented to dispatch to child process if needed to support
// subtests
int main(int argc, char *argv[])
{
    if (argc > 1) {
        childGenRandom(std::atoi(argv[1]));
        Q_UNREACHABLE();
    }

    binpath = argv[0];
    KRandomTest randomTest;
    return QTest::qExec(&randomTest);
}

#include "krandomtest.moc"
