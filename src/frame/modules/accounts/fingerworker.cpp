/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             Hualet <mr.asianwang@gmail.com>
 *             kirigaya <kirigaya@mkacg.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             Hualet <mr.asianwang@gmail.com>
 *             kirigaya <kirigaya@mkacg.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fingerworker.h"

#include <QFutureWatcher>
#include <QtConcurrent>
#include <QProcess>

#include <unistd.h>
#include <sys/types.h>

using namespace dcc;
using namespace dcc::accounts;

const QString FingerPrintService("com.deepin.daemon.Authenticate.FingerPrint");

#define TEST false

FingerWorker::FingerWorker(FingerModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_fingerPrintInter(new Fingerprint(FingerPrintService, "/com/deepin/daemon/Authenticate.FingerPrint",
                                         QDBusConnection::systemBus(), this))
{
//    m_fingerPrintInter->setSync(false);

    //处理指纹后端的录入状态信号
    connect(m_fingerPrintInter, &Fingerprint::EnrollStatus, m_model, [this](const QString &id, int code, const QString &msg) {
        m_model->setTestEnrollStatus(code, msg);
    });
    //当前此信号末实现
    connect(m_fingerPrintInter, &Fingerprint::Touch, m_model, &FingerModel::testEnrollTouch);

    auto defualtDevice = m_fingerPrintInter->defaultDevice();
    m_model->setIsVaild(!defualtDevice.isEmpty());
}

void FingerWorker::refreshUserEnrollList(const QString &name)
{
    auto call = m_fingerPrintInter->ListFingers(name);
    call.waitForFinished();
    m_model->setThumbsList(name, call.value());
}

void FingerWorker::enrollStart(const QString &name, const QString &thumb)
{

    if(TEST) {
        refreshUserEnrollList(name);
        m_model->setTestEnrollStatus(1,"1");
    } else {
        if (!m_fingerPrintInter->devices().isEmpty()) {
            //后端接口需要传id不是用户名，需要改
            m_fingerPrintInter->Claim(name, true);
            m_fingerPrintInter->Enroll(name, thumb);
            refreshUserEnrollList(name);
        }
    }
}

void FingerWorker::reEnrollStart(const QString &thumb)
{
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, [this, watcher] {
        if (watcher->result()) {
            m_model->setEnrollStatus(FingerModel::EnrollStatus::Ready);
        }

        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run(this, &FingerWorker::reRecordFinger, thumb);
    watcher->setFuture(future);
}

void FingerWorker::stopEnroll()
{
    auto call = m_fingerPrintInter->StopEnroll();
    call.waitForFinished();

    auto id = QString::number(getuid());
    m_fingerPrintInter->Claim(id, false);
}

void FingerWorker::deleteFingerItem(const QString& userName, const QString& finger)
{
    m_fingerPrintInter->DeleteFinger(userName, finger);
    refreshUserEnrollList(userName);
}

void FingerWorker::onEnrollStatus(const QString &value, const bool status)
{
    qDebug() << "onEnrollStatus: " << value << "," << status;
    if (value == "enroll-completed" && status == 1) {
        m_model->setEnrollStatus(FingerModel::EnrollStatus::Finished);
        return;
    }

    if (value == "enroll-retry-scan") {
        m_model->setEnrollStatus(FingerModel::EnrollStatus::Retry);
        return;
    }

    m_model->setEnrollStatus(FingerModel::EnrollStatus::Next);
}

bool FingerWorker::recordFinger(const QString &name, const QString &thumb)
{
    //1, 判断设备是否可用，如果不可用，返回error
    //2, 调用Claim
    auto id = QString::number(getuid());
    auto call = m_fingerPrintInter->Claim(id, true);
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << call.error();
        qDebug() << "call Claim Error : " << call.error();
        return false;
    }

    call = m_fingerPrintInter->Enroll(id, thumb);
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << "call Enroll Error : " << call.error();
        return false;
    }

    return true;
}

bool FingerWorker::reRecordFinger(const QString &thumb)
{
    //1, 判断设备是否可用, 如果非独占，返回error
    //2, 调用StopEnroll
    auto id = QString::number(getuid());
    QDBusPendingCall call = m_fingerPrintInter->StopEnroll();
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << "call StopEnroll Error : " << call.error();
        return false;
    }

    call = m_fingerPrintInter->Enroll(id, thumb);
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << "call Claim Error : " << call.error();
        return false;
    }

    return true;
}

void FingerWorker::releaseEnroll()
{
    auto id = QString::number(getuid());
    QDBusPendingCall call = m_fingerPrintInter->Claim(id, false);
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << call.error();
    }
}

bool FingerWorker::cleanFinger(const QString &name)
{
    QDBusPendingCall call = m_fingerPrintInter->DeleteAllFingers(name);
    call.waitForFinished();

    if (call.isError()) {
        qDebug() << call.error();
        return false;
    }

    refreshUserEnrollList(name);
    return true;
}
