/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     liuhong <liuhong_cm@deepin.com>
 *
 * Maintainer: liuhong <liuhong_cm@deepin.com>
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

#include "addfingedialog.h"

#include <DTitlebar>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCloseEvent>
#include <QTimer>

using namespace dcc::accounts;
using namespace DCC_NAMESPACE::accounts;
#define test false

AddFingeDialog::AddFingeDialog(const QString &thumb, DAbstractDialog *parent)
    : DAbstractDialog(parent)
    , m_mainLayout(new QVBoxLayout(this))
    , m_titleHLayout(new QHBoxLayout)
    , m_contentVLayout(new QVBoxLayout)
    , m_btnHLayout(new QHBoxLayout)
    , m_fingerHLayout(new QVBoxLayout)
    , m_fingeWidget(new FingerWidget)
    , m_scanBtn(new DSuggestButton)
    , m_doneBtn(new DSuggestButton)
    , m_thumb(thumb)
    , m_cancelBtn(new QPushButton)
    , m_addBtn(new DSuggestButton)
    , m_titleTip(new DTipLabel)
    , m_qtimerTitleTip(new QTimer())
    , m_qtimerMsgTip(new QTimer())
{
    initWidget();
    initData();
}

void AddFingeDialog::initWidget()
{
    setMinimumSize(QSize(328,391));
    DTitlebar *titleIcon = new DTitlebar();
    titleIcon->setFrameStyle(QFrame::NoFrame);//无边框
    titleIcon->setBackgroundTransparent(true);//透明
    titleIcon->setMenuVisible(false);
    titleIcon->setTitle("");
    m_titleHLayout->addWidget(titleIcon, Qt::AlignTop);
    m_mainLayout->addLayout(m_titleHLayout);
    QFont font;
    font.setBold(true);
    m_titleTip->setFont(font);
    m_titleTip->setText(tr("放置手指"));
    m_fingerHLayout->addWidget(m_titleTip);
    m_fingerHLayout->addWidget(m_fingeWidget);
    m_contentVLayout->addLayout(m_fingerHLayout);

    m_btnHLayout->addWidget(m_cancelBtn);
    m_btnHLayout->addWidget(m_addBtn);
    m_btnHLayout->addWidget(m_scanBtn);
    m_btnHLayout->addWidget(m_doneBtn);
    m_btnHLayout->setContentsMargins(10,0,10,10);
    m_contentVLayout->addLayout(m_btnHLayout);
    m_mainLayout->addLayout(m_contentVLayout);

    m_mainLayout->setMargin(0);
    setLayout(m_mainLayout);
    m_cancelBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_addBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void AddFingeDialog::initData()
{
//    setWindowTitle(tr("Add Fingerprint"));

    m_scanBtn->setText(tr("Scan again"));
    m_doneBtn->setText(tr("Done"));

    m_scanBtn->setVisible(false);
    m_doneBtn->setVisible(false);

    m_cancelBtn->setText((tr("取消")));
    m_addBtn->setText((tr("添加")));
    connect(m_cancelBtn, &QPushButton::clicked, this, &AddFingeDialog::close);
    connect(m_scanBtn, &QPushButton::clicked, this, &AddFingeDialog::requestReEnrollThumb);//重新录入后，界面变化还需要再确认
    connect(m_addBtn, &QPushButton::clicked, this, &AddFingeDialog::requestEnrollThumb);
}

void AddFingeDialog::setFingerModel(FingerModel *model)
{
    m_model = model;
    connect(model, &FingerModel::testEnrollStatusChange, this, &AddFingeDialog::testOnEnrollStatusChanged);
//    connect(model, &FingerModel::testEnrollTouch, this, [this](const QString str1, bool pressed) {
//        m_tip->setVisible(pressed);
//    });
    testOnEnrollStatusChanged(m_model->testEnrollStatus(), m_model->testMsg());
}

void AddFingeDialog::setUsername(const QString &name)
{
    m_username = name;
}

//void AddFingeDialog::reEnrollStart()
//{
//    m_scanBtn->setVisible(false);
//    m_doneBtn->setVisible(false);

//    Q_EMIT requestReEnrollStart(m_thumb);

//    m_fingeWidget->reEnter();
//}

void AddFingeDialog::onEnrollStatusChanged(FingerModel::EnrollStatus status)
{
    switch (status) {
    case FingerModel::EnrollStatus::Ready:
        onViewPlayEnd();
        break;
    case FingerModel::EnrollStatus::Next:
        m_fingeWidget->next();
        m_fingeWidget->setFrequency(tr("Identifying fingerprint"));
        break;
    case FingerModel::EnrollStatus::Retry:
        m_fingeWidget->setFrequency(tr("Place your finger on the fingerprint reader, or swipe upwards or downwards, and then lift it off"));
        break;
    case FingerModel::EnrollStatus::Finished:
        m_fingeWidget->finished();
        m_fingeWidget->setFrequency(tr("Fingerprint added"));
        m_scanBtn->setVisible(true);
        m_doneBtn->setVisible(true);
        break;
    }
}

void AddFingeDialog::testOnEnrollStatusChanged(dcc::accounts::FingerModel::TestEnrollStatus status,
                                               dcc::accounts::FingerModel::TestMsg msg, int process)
{
//    auto test_status  = FingerModel::TestEnrollStatus::Default;
//    auto test_msg = FingerModel::TestMsg::unknown_error;
    switch (status) {
    //初始状态
    case FingerModel::TestEnrollStatus::Default:
        setDefaultMsgTip();
        break;
        //录入失败，是否需要提示？
    case FingerModel::TestEnrollStatus::enrollFailed:
        Q_EMIT enrollFailed();
        break;
        //正常采集状态， 此时msg代表采集进度
    case FingerModel::TestEnrollStatus::StagePassed:
        if (process < 35) {
            m_titleTip->setVisible(false);
            m_fingeWidget->setFrequency(tr("请抬起手指，再次按压"));
        }
        if (process >= 35 && process <100) {
            m_fingeWidget->setFrequency(tr("请抬起手指，调整按压区域，继续录入边缘指纹"));
        }
        if (process == 100) {
            m_titleTip->setText(tr("指纹录入完成！"));
            m_fingeWidget->setFrequency("");
            m_fingeWidget->finished();
            m_addBtn->setVisible(false);
            m_doneBtn->setVisible(true);
            connect(m_doneBtn, &DSuggestButton::clicked, this, &AddFingeDialog::close);
            break;
        }
        break;
        //重新采集
    case FingerModel::TestEnrollStatus::retry:
        if (msg != FingerModel::TestMsg::thumb_repeated ) {
            m_qtimerTitleTip->stop();
            m_qtimerTitleTip->setInterval(1000);
            m_titleTip->setText(tr("无法识别"));
            m_titleTip->update();
            connect(m_qtimerTitleTip, &QTimer::timeout,[this] {setDefaultTitleTip();});
            m_qtimerTitleTip->start();
        }
        m_qtimerMsgTip->setSingleShot(true);
        m_qtimerMsgTip->setInterval(2000);
        switch (msg) {
        //图形不可用
        case FingerModel::TestMsg::graphics_unuse:
            //            m_qtimerMsgTip->setInterval(2000);
            m_fingeWidget->setFrequency(tr("请清洁手指或调整触摸位置，再次按压指纹识别器"));
            break;
            //接触时间过短
        case FingerModel::TestMsg::time_short:
            m_fingeWidget->setFrequency(tr("指纹采集间隙，请勿移动手指，直到提示您抬起"));
            break;
            //检测到某次指纹信号与已收集的信号重复率过高无法达到录入标准
        case FingerModel::TestMsg::high_repetition_rate:
            m_fingeWidget->setFrequency(tr("请调整手指按压区域以录入更多指纹"));
            break;
            //断开,超时,这个信号没有？
            //        case FingerModel::TestMsg::error05:
            //            m_titleTip->setText(tr("录入中断"));
            //            m_fingeWidget->setFrequency(tr("如需重新录入，请点击重新录入，指纹录入过程会从头开始"));
            //            m_addBtn->setVisible(false);
            //            m_scanBtn->setVisible(true);
            //            connect(m_scanBtn, &QPushButton::clicked, [this] {
            //                m_addBtn->setVisible(true);
            //                m_scanBtn->setVisible(false);
            //                setDefaultTitleTip();
            //                setDefaultMsgTip();
            //            });
            //            break;
        case FingerModel::TestMsg::thumb_repeated:
            m_fingeWidget->setFrequency(tr("指纹已存在，请使用其他手指录入"));
            m_addBtn->setVisible(false);
            m_scanBtn->setVisible(true);
            connect(m_scanBtn, &QPushButton::clicked, [this] {
                m_addBtn->setVisible(true);
                m_scanBtn->setVisible(false);
                setDefaultTitleTip();
                setDefaultMsgTip();
            });
            break;
        }
        if(msg != FingerModel::TestMsg::thumb_repeated) {
            connect(m_qtimerMsgTip, &QTimer::timeout, [this] {setDefaultMsgTip();});
            m_qtimerMsgTip->start();
        }
        break;
    }
}
void AddFingeDialog::setDefaultMsgTip()
{
    m_fingeWidget->setFrequency(tr("请以手指按压指纹收集器，然后根据提示抬起"));
}

void AddFingeDialog::onViewPlayEnd()
{
    m_fingeWidget->setFrequency(tr("Place your finger on the fingerprint reader, or swipe upwards or downwards, and then lift it off"));

}

void AddFingeDialog::setDefaultTitleTip()
{
    m_titleTip->setText(tr("放置手指"));
}

void AddFingeDialog::closeEvent(QCloseEvent *event)
{
    Q_EMIT requestStopEnroll();
    event->accept();
}
