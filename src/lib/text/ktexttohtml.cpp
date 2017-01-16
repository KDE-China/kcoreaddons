/*
  Copyright (c) 2002 Dave Corrie <kde@davecorrie.com>
  Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "ktexttohtml.h"
#include "ktexttohtml_p.h"
#include "ktexttohtmlemoticonsinterface.h"

#include <QString>
#include <QStringList>
#include <QFile>
#include <QRegExp>
#include <QPluginLoader>
#include <QVariant>
#include <QCoreApplication>

#include <limits.h>

#include "kcoreaddons_debug.h"

static KTextToHTMLEmoticonsInterface *s_emoticonsInterface = nullptr;

static void loadEmoticonsPlugin()
{
    static bool triedLoadPlugin = false;
    if (!triedLoadPlugin) {
        triedLoadPlugin = true;

        // Check if QGuiApplication::platformName property exists. This is a
        // hackish way of determining whether we are running QGuiApplication,
        // because we cannot load the FrameworkIntegration plugin into a
        // QCoreApplication, as it would crash immediatelly
        if (qApp->metaObject()->indexOfProperty("platformName") > -1) {
            QPluginLoader lib(QStringLiteral("kf5/KEmoticonsIntegrationPlugin"));
            QObject *rootObj = lib.instance();
            if (rootObj) {
                s_emoticonsInterface = rootObj->property(KTEXTTOHTMLEMOTICONS_PROPERTY).value<KTextToHTMLEmoticonsInterface*>();
            }
        }
    }
    if (!s_emoticonsInterface) {
        s_emoticonsInterface = new KTextToHTMLEmoticonsDummy();
    }
}



KTextToHTMLHelper::KTextToHTMLHelper(const QString &plainText, int pos, int maxUrlLen, int maxAddressLen)
    : mText(plainText)
    , mMaxUrlLen(maxUrlLen)
    , mMaxAddressLen(maxAddressLen)
    , mPos(pos)
{
}

KTextToHTMLEmoticonsInterface* KTextToHTMLHelper::emoticonsInterface()
{
    if (!s_emoticonsInterface) {
        loadEmoticonsPlugin();
    }
    return s_emoticonsInterface;
}

QString KTextToHTMLHelper::getEmailAddress()
{
    QString address;

    if (mText[mPos] == QLatin1Char('@')) {
        // the following characters are allowed in a dot-atom (RFC 2822):
        // a-z A-Z 0-9 . ! # $ % & ' * + - / = ? ^ _ ` { | } ~
        static const QString allowedSpecialChars = QStringLiteral(".!#$%&'*+-/=?^_`{|}~");

        // determine the local part of the email address
        int start = mPos - 1;
        while (start >= 0 && mText[start].unicode() < 128 &&
                (mText[start].isLetterOrNumber() ||
                 mText[start] == QLatin1Char('@') || // allow @ to find invalid email addresses
                 allowedSpecialChars.indexOf(mText[start]) != -1)) {
            if (mText[start] == QLatin1Char('@')) {
                return QString(); // local part contains '@' -> no email address
            }
            --start;
        }
        ++start;
        // we assume that an email address starts with a letter or a digit
        while ((start < mPos) && !mText[start].isLetterOrNumber()) {
            ++start;
        }
        if (start == mPos) {
            return QString(); // local part is empty -> no email address
        }

        // determine the domain part of the email address
        int dotPos = INT_MAX;
        int end = mPos + 1;
        while (end < mText.length() &&
                (mText[end].isLetterOrNumber() ||
                 mText[end] == QLatin1Char('@') || // allow @ to find invalid email addresses
                 mText[end] == QLatin1Char('.') ||
                 mText[end] == QLatin1Char('-'))) {
            if (mText[end] == QLatin1Char('@')) {
                return QString(); // domain part contains '@' -> no email address
            }
            if (mText[end] == QLatin1Char('.')) {
                dotPos = qMin(dotPos, end);   // remember index of first dot in domain
            }
            ++end;
        }
        // we assume that an email address ends with a letter or a digit
        while ((end > mPos) && !mText[end - 1].isLetterOrNumber()) {
            --end;
        }
        if (end == mPos) {
            return QString(); // domain part is empty -> no email address
        }
        if (dotPos >= end) {
            return QString(); // domain part doesn't contain a dot
        }

        if (end - start > mMaxAddressLen) {
            return QString(); // too long -> most likely no email address
        }
        address = mText.mid(start, end - start);

        mPos = end - 1;
    }
    return address;
}

bool KTextToHTMLHelper::atUrl()
{
    // the following characters are allowed in a dot-atom (RFC 2822):
    // a-z A-Z 0-9 . ! # $ % & ' * + - / = ? ^ _ ` { | } ~
    static const QString allowedSpecialChars = QStringLiteral(".!#$%&'*+-/=?^_`{|}~");

    // the character directly before the URL must not be a letter, a number or
    // any other character allowed in a dot-atom (RFC 2822).
    if ((mPos > 0) &&
            (mText[mPos - 1].isLetterOrNumber() ||
             (allowedSpecialChars.indexOf(mText[mPos - 1]) != -1))) {
        return false;
    }
    QChar ch = mText[mPos];
    return
        (ch == QLatin1Char('h') && (mText.mid(mPos, 7) == QLatin1String("http://") ||
                                    mText.mid(mPos, 8) == QLatin1String("https://"))) ||
        (ch == QLatin1Char('v') && mText.mid(mPos, 6) == QLatin1String("vnc://")) ||
        (ch == QLatin1Char('f') && (mText.mid(mPos, 7) == QLatin1String("fish://") ||
                                    mText.mid(mPos, 6) == QLatin1String("ftp://") ||
                                    mText.mid(mPos, 7) == QLatin1String("ftps://"))) ||
        (ch == QLatin1Char('s') && (mText.mid(mPos, 7) == QLatin1String("sftp://") ||
                                    mText.mid(mPos, 6) == QLatin1String("smb://"))) ||
        (ch == QLatin1Char('m') && mText.mid(mPos, 7) == QLatin1String("mailto:")) ||
        (ch == QLatin1Char('w') && mText.mid(mPos, 4) == QLatin1String("www.")) ||
        (ch == QLatin1Char('f') && (mText.mid(mPos, 4) == QLatin1String("ftp.") ||
                                    mText.mid(mPos, 7) == QLatin1String("file://"))) ||
        (ch == QLatin1Char('n') && mText.mid(mPos, 5) == QLatin1String("news:"));
}

bool KTextToHTMLHelper::isEmptyUrl(const QString &url)
{
    return url.isEmpty() ||
           url == QLatin1String("http://") ||
           url == QLatin1String("https://") ||
           url == QLatin1String("fish://") ||
           url == QLatin1String("ftp://") ||
           url == QLatin1String("ftps://") ||
           url == QLatin1String("sftp://") ||
           url == QLatin1String("smb://") ||
           url == QLatin1String("vnc://") ||
           url == QLatin1String("mailto") ||
           url == QLatin1String("www") ||
           url == QLatin1String("ftp") ||
           url == QLatin1String("news") ||
           url == QLatin1String("news://");
}

QString KTextToHTMLHelper::getUrl(bool *badurl)
{
    QString url;
    if (atUrl()) {
        // NOTE: see http://tools.ietf.org/html/rfc3986#appendix-A and especially appendix-C
        // Appendix-C mainly says, that when extracting URLs from plain text, line breaks shall
        // be allowed and should be ignored when the URI is extracted.

        // This implementation follows this recommendation and
        // allows the URL to be enclosed within different kind of brackets/quotes
        // If an URL is enclosed, whitespace characters are allowed and removed, otherwise
        // the URL ends with the first whitespace
        // Also, if the URL is enclosed in brackets, the URL itself is not allowed
        // to contain the closing bracket, as this would be detected as the end of the URL

        QChar beforeUrl, afterUrl;

        // detect if the url has been surrounded by brackets or quotes
        if (mPos > 0) {
            beforeUrl = mText[mPos - 1];

            /*if ( beforeUrl == '(' ) {
              afterUrl = ')';
            } else */if (beforeUrl == QLatin1Char('[')) {
                afterUrl = QLatin1Char(']');
            } else if (beforeUrl == QLatin1Char('<')) {
                afterUrl = QLatin1Char('>');
            } else if (beforeUrl == QLatin1Char('>')) {   // for e.g. <link>http://.....</link>
                afterUrl = QLatin1Char('<');
            } else if (beforeUrl == QLatin1Char('"')) {
                afterUrl = QLatin1Char('"');
            }
        }

        url.reserve(mMaxUrlLen);    // avoid allocs
        int start = mPos;
        bool previousCharIsSpace = false;
        bool previousCharIsADoubleQuote = false;
        bool previousIsAnAnchor = false;
        while ((mPos < mText.length()) &&
                (mText[mPos].isPrint() || mText[mPos].isSpace()) &&
                ((afterUrl.isNull() && !mText[mPos].isSpace()) ||
                 (!afterUrl.isNull() && mText[mPos] != afterUrl))) {
            if (mText[mPos].isSpace()) {
                previousCharIsSpace = true;
            } else if (!previousIsAnAnchor && mText[mPos] == QLatin1Char('[')) {
                break;
            } else { // skip whitespace
                if (previousCharIsSpace && mText[mPos] == QLatin1Char('<')) {
                    url.append(QLatin1Char(' '));
                    break;
                }
                previousCharIsSpace = false;
                if (mText[mPos] == QLatin1Char('>') && previousCharIsADoubleQuote) {
                    //it's an invalid url
                    if (badurl) {
                        *badurl = true;
                    }
                    return QString();
                }
                if (mText[mPos] == QLatin1Char('"')) {
                    previousCharIsADoubleQuote = true;
                } else {
                    previousCharIsADoubleQuote = false;
                }
                if (mText[mPos] == QLatin1Char('#')) {
                    previousIsAnAnchor = true;
                }
                url.append(mText[mPos]);
                if (url.length() > mMaxUrlLen) {
                    break;
                }
            }

            ++mPos;
        }

        if (isEmptyUrl(url) || (url.length() > mMaxUrlLen)) {
            mPos = start;
            url.clear();
        } else {
            --mPos;
        }
    }

    // HACK: This is actually against the RFC. However, most people don't properly escape the URL in
    //       their text with "" or <>. That leads to people writing an url, followed immediatley by
    //       a dot to finish the sentence. That would lead the parser to include the dot in the url,
    //       even though that is not wanted. So work around that here.
    //       Most real-life URLs hopefully don't end with dots or commas.
    QList<QChar> wordBoundaries;
    wordBoundaries << QLatin1Char('.') << QLatin1Char(',') << QLatin1Char(':') << QLatin1Char('!') << QLatin1Char('?') << QLatin1Char(')') << QLatin1Char('>');
    if (url.length() > 1) {
        do {
            if (wordBoundaries.contains(url.at(url.length() - 1))) {
                url.chop(1);
                --mPos;
            } else {
                break;
            }
        } while (url.length() > 1);
    }
    return url;
}

QString KTextToHTMLHelper::highlightedText()
{
    // formating symbols must be prepended with a whitespace
    if ((mPos > 0) && !mText[mPos - 1].isSpace()) {
        return QString();
    }

    const QChar ch = mText[mPos];
    if (ch != QLatin1Char('/') && ch != QLatin1Char('*') && ch != QLatin1Char('_') && ch != QLatin1Char('-')) {
        return QString();
    }

    QRegExp re =
        QRegExp(QStringLiteral("\\%1((\\w+)([\\s-']\\w+)*( ?[,.:\\?!;])?)\\%2").arg(ch).arg(ch));
    re.setMinimal(true);
    if (re.indexIn(mText, mPos) == mPos) {
        int length = re.matchedLength();
        // there must be a whitespace after the closing formating symbol
        if (mPos + length < mText.length() && !mText[mPos + length].isSpace()) {
            return QString();
        }
        mPos += length - 1;
        switch (ch.toLatin1()) {
        case '*':
            return QLatin1String("<b>*") + re.cap(1) + QLatin1String("*</b>");
        case '_':
            return QLatin1String("<u>_") + re.cap(1) + QLatin1String("_</u>");
        case '/':
            return QLatin1String("<i>/") + re.cap(1) + QLatin1String("/</i>");
        case '-':
            return QLatin1String("<strike>-") + re.cap(1) + QLatin1String("-</strike>");
        }
    }
    return QString();
}


QString KTextToHTMLHelper::pngToDataUrl(const QString &iconPath)
{
    if (iconPath.isEmpty()) {
        return QString();
    }

    QFile pngFile(iconPath);
    if (!pngFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        return QString();
    }

    QByteArray ba = pngFile.readAll();
    pngFile.close();
    return QStringLiteral("data:image/png;base64,%1").arg(QLatin1String(ba.toBase64().constData()));
}


QString KTextToHTML::convertToHtml(const QString &plainText, const KTextToHTML::Options &flags, int maxUrlLen, int maxAddressLen)
{
    KTextToHTMLHelper helper(plainText, maxUrlLen, maxAddressLen);

    QString str;
    QString result(static_cast<QChar *>(nullptr), helper.mText.length() * 2);
    QChar ch;
    int x;
    bool startOfLine = true;

    for (helper.mPos = 0, x = 0; helper.mPos < helper.mText.length();
            ++helper.mPos, ++x) {
        ch = helper.mText[helper.mPos];
        if (flags & PreserveSpaces) {
            if (ch == QLatin1Char(' ')) {
                if (helper.mPos + 1 < helper.mText.length()) {
                    if (helper.mText[helper.mPos + 1] != QLatin1Char(' ')) {

                        // A single space, make it breaking if not at the start or end of the line
                        const bool endOfLine = helper.mText[helper.mPos + 1] == QLatin1Char('\n');
                        if (!startOfLine && !endOfLine) {
                            result += QLatin1Char(' ');
                        } else {
                            result += QLatin1String("&nbsp;");
                        }
                    } else {

                        // Whitespace of more than one space, make it all non-breaking
                        while (helper.mPos < helper.mText.length() && helper.mText[helper.mPos] == QLatin1Char(' ')) {
                            result += QLatin1String("&nbsp;");
                            ++helper.mPos;
                            ++x;
                        }

                        // We incremented once to often, undo that
                        --helper.mPos;
                        --x;
                    }
                } else {
                    // Last space in the text, it is non-breaking
                    result += QLatin1String("&nbsp;");
                }

                if (startOfLine) {
                    startOfLine = false;
                }
                continue;
            } else if (ch == QLatin1Char('\t')) {
                do {
                    result += QLatin1String("&nbsp;");
                    ++x;
                } while ((x & 7) != 0);
                --x;
                startOfLine = false;
                continue;
            }
        }
        if (ch == QLatin1Char('\n')) {
            result += QLatin1String("<br />\n"); // Keep the \n, so apps can figure out the quoting levels correctly.
            startOfLine = true;
            x = -1;
            continue;
        }

        startOfLine = false;
        if (ch == QLatin1Char('&')) {
            result += QLatin1String("&amp;");
        } else if (ch == QLatin1Char('"')) {
            result += QLatin1String("&quot;");
        } else if (ch == QLatin1Char('<')) {
            result += QLatin1String("&lt;");
        } else if (ch == QLatin1Char('>')) {
            result += QLatin1String("&gt;");
        } else {
            const int start = helper.mPos;
            if (!(flags & IgnoreUrls)) {
                bool badUrl = false;
                str = helper.getUrl(&badUrl);
                if (badUrl) {
                    QString resultBadUrl;
                    const int helperTextSize(helper.mText.count());
                    for (int i = 0; i < helperTextSize; ++i) {
                        const QChar chBadUrl = helper.mText[i];
                        if (chBadUrl == QLatin1Char('&')) {
                            resultBadUrl += QLatin1String("&amp;");
                        } else if (chBadUrl == QLatin1Char('"')) {
                            resultBadUrl += QLatin1String("&quot;");
                        } else if (chBadUrl == QLatin1Char('<')) {
                            resultBadUrl += QLatin1String("&lt;");
                        } else if (chBadUrl == QLatin1Char('>')) {
                            resultBadUrl += QLatin1String("&gt;");
                        } else {
                            resultBadUrl += chBadUrl;
                        }
                    }
                    return resultBadUrl;
                }
                if (!str.isEmpty()) {
                    QString hyperlink;
                    if (str.left(4) == QLatin1String("www.")) {
                        hyperlink = QLatin1String("http://") + str;
                    } else if (str.left(4) == QLatin1String("ftp.")) {
                        hyperlink = QLatin1String("ftp://") + str;
                    } else {
                        hyperlink = str;
                    }

                    result += QLatin1String("<a href=\"") + hyperlink + QLatin1String("\">") + str.toHtmlEscaped() + QLatin1String("</a>");
                    x += helper.mPos - start;
                    continue;
                }
                str = helper.getEmailAddress();
                if (!str.isEmpty()) {
                    // len is the length of the local part
                    int len = str.indexOf(QLatin1Char('@'));
                    QString localPart = str.left(len);

                    // remove the local part from the result (as '&'s have been expanded to
                    // &amp; we have to take care of the 4 additional characters per '&')
                    result.truncate(result.length() -
                                    len - (localPart.count(QLatin1Char('&')) * 4));
                    x -= len;

                    result += QLatin1String("<a href=\"mailto:") + str + QLatin1String("\">") + str + QLatin1String("</a>");
                    x += str.length() - 1;
                    continue;
                }
            }
            if (flags & HighlightText) {
                str = helper.highlightedText();
                if (!str.isEmpty()) {
                    result += str;
                    x += helper.mPos - start;
                    continue;
                }
            }
            result += ch;
        }
    }

    if (flags & ReplaceSmileys) {
        QStringList exclude;
        exclude << QStringLiteral("(c)") << QStringLiteral("(C)") << QStringLiteral("&gt;:-(") << QStringLiteral("&gt;:(") << QStringLiteral("(B)") << QStringLiteral("(b)") << QStringLiteral("(P)") << QStringLiteral("(p)");
        exclude << QStringLiteral("(O)") << QStringLiteral("(o)") << QStringLiteral("(D)") << QStringLiteral("(d)") << QStringLiteral("(E)") << QStringLiteral("(e)") << QStringLiteral("(K)") << QStringLiteral("(k)");
        exclude << QStringLiteral("(I)") << QStringLiteral("(i)") << QStringLiteral("(L)") << QStringLiteral("(l)") << QStringLiteral("(8)") << QStringLiteral("(T)") << QStringLiteral("(t)") << QStringLiteral("(G)");
        exclude << QStringLiteral("(g)") << QStringLiteral("(F)") << QStringLiteral("(f)") << QStringLiteral("(H)");
        exclude << QStringLiteral("8)") << QStringLiteral("(N)") << QStringLiteral("(n)") << QStringLiteral("(Y)") << QStringLiteral("(y)") << QStringLiteral("(U)") << QStringLiteral("(u)") << QStringLiteral("(W)") << QStringLiteral("(w)");

        result = helper.emoticonsInterface()->parseEmoticons(result, true, exclude);
    }

    return result;
}
