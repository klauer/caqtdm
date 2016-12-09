/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#include <QApplication>
#include <QNetworkAccessManager>
#include <iostream>
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QWaitCondition>
#include <QEventLoop>
#include <time.h>
#include <sys/timeb.h>
#include "sfRetrieval.h"

#define qasc(x) x.toLatin1().constData()

#ifdef MOBILE_ANDROID
#  include <unistd.h>
#endif

// this sleep will not block the GUI and QThread::msleep is protected in Qt4.8 (so do not use that)
class Sleep
{
public:
    static void msleep(unsigned long msecs)
    {
#ifndef MOBILE_ANDROID
        QMutex mutex;
        mutex.lock();
        QWaitCondition waitCondition;
        waitCondition.wait(&mutex, msecs);
        mutex.unlock();
#else
        // not nice, but the above does not work on android now (does not wait)
        usleep(msecs * 100);
#endif
    }
};

sfRetrieval::sfRetrieval()
{
    finished = false;
    manager = new QNetworkAccessManager;
    errorString = "";
}

bool sfRetrieval::requestUrl(const QUrl url, const QByteArray &json, int secondsPast)
{
    finished = false;
    totalCount = 0;
    secndsPast = secondsPast;
    QString out = QString(json);
    printf("caQtDM -- request from %s with %s\n", qasc(url.toString()), qasc(out));
    downloadUrl = url;

    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Timeout", "86400");

    QNetworkReply* reply = manager->post(request, json);
    connect(reply, SIGNAL(finished()), this, SLOT(finishReply()));

    //printf("go on wait for finish\n");
    //wait until download was done (up to 3 seconds)
    int looped = 0;
    for(int i=0; i<10; i++) {
        qApp->processEvents();
        Sleep::msleep(300);
        qApp->processEvents();
        if(downloadFinished()) {
            //printf("download finished\n");
            return true;
        }
        //printf("loop\n");
        looped++;
    }
    if(!downloadFinished()) {
        printf("download not finished\n");
        return false;
    }
    printf("download not finished\n");
    return false;
}

int sfRetrieval::downloadFinished()
{
    return finished;
}

#include <ctime>

void sfRetrieval::finishReply()
{
    struct timeb now;

    //printf("network reply completed\n");
    QObject* obj = sender();
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(obj);

    if(reply->error()) {
        errorString = tr("%1: %2").arg(parseError(reply->error())).arg(downloadUrl.toString());
        printf("%s\n", qasc(errorString));
        return;
    }

    QString out = QString(reply->readAll());
    QStringList result = out.split("\n", QString::SkipEmptyParts);
    printf("number of values received = %d\n",  result.count());
    X.resize(result.count()-1);
    Y.resize(result.count()-1);

    ftime(&now);
    int count = 0;
    for(int i=1; i< result.count(); ++i) {
         QStringList line = result[i].split(";", QString::SkipEmptyParts);
         QString time = line[1];
         double timed = time.toDouble();
         unsigned long epoch32 = (unsigned long) timed;
         time_t t = epoch32;
         double seconds = (double) now.time + (double) now.millitm / (double)1000;
         if((seconds - line[1].toDouble()) < secndsPast) {
            X[count] = -(seconds - line[1].toDouble()) / 3600.0;
            Y[count++] = line[2].toDouble();
            //if(count < 10) printf("%f channel=%s seconds=%s value=%s  values=%f %f time=%s", seconds - line[1].toDouble(), qasc(line[0]),  qasc(line[1]), qasc(line[2]), X[i-1],Y[i-1], ctime(&t));
         }
    }
    totalCount = count;

    finished = true;
}

int sfRetrieval::getCount()
{
    return totalCount;
}

void sfRetrieval::getData(QVector<double> &x, QVector<double> &y)
{
    x = X;
    y = Y;
}

const QString sfRetrieval::lastError()
{
    return errorString;
}

const QString sfRetrieval::parseError(QNetworkReply::NetworkError error)
{
    QString errstr = "";
    switch(error)
    {
    case QNetworkReply::ConnectionRefusedError:
        errstr = tr("ConnectionRefusedError");
        break;
    case QNetworkReply::HostNotFoundError:
        errstr = tr("HostNotFoundError");
        break;
    case QNetworkReply::RemoteHostClosedError:
        errstr = tr("RemoteHostClosedError");
        break;
    case QNetworkReply::TimeoutError:
        errstr = tr("TimeoutError");
        break;
    case QNetworkReply::ContentAccessDenied:
        errstr = tr("ContentAccessDenied");
        break;
    case QNetworkReply::ProtocolFailure:
        errstr = tr("ProtocolFailure");
        break;
    case QNetworkReply::ContentNotFoundError:
        errstr = tr("ContentNotFoundError");
        break;
    default:
        errstr = tr("unknownError");
        break;
    }
    return errstr;
}
