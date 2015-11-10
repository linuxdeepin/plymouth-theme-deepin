#include <QHBoxLayout>
#include <QDebug>

#include <libdui/dlabel.h>
#include <libdui/libdui_global.h>
#include <libdui/dconstants.h>

#include "constants.h"

#include "networkglobal.h"
#include "networkbaseeditline.h"

DUI_USE_NAMESPACE

NetworkBaseEditLine::NetworkBaseEditLine(const QString &section, const QString &key,
                                         DBusConnectionSession *dbus, const QString &title,
                                         QWidget *rightWidget, QWidget *parent):
    QWidget(parent),
    m_dbus(dbus),
    m_section(section),
    m_key(key)
{
    QHBoxLayout *layout = new QHBoxLayout;
    DLabel *label = new DLabel(title);

    rightWidget->setFixedSize(DCC::ModuleContentWidth / 1.6, DUI::MENU_ITEM_HEIGHT);

    layout->setMargin(0);
    layout->setSpacing(15);
    layout->addWidget(label, 0, Qt::AlignRight);
    layout->addWidget(rightWidget, 0, Qt::AlignRight);

    setLayout(layout);
    updateVisible();

    connect(dbus, &DBusConnectionSession::AvailableKeysChanged, this, &NetworkBaseEditLine::updateVisible);
    connect(dbus, &DBusConnectionSession::AvailableSectionsChanged, this, &NetworkBaseEditLine::updateVisible);
    connect(dbus, &DBusConnectionSession::ConnectionDataChanged, this, [this]{
        if(isVisible() && alwaysUpdate()) {
            setCacheValue(dbusKey());
        }
    });
}

QString NetworkBaseEditLine::section() const
{
    return m_section;
}

QString NetworkBaseEditLine::key() const
{
    return m_key;
}

QJsonValue NetworkBaseEditLine::cacheValue() const
{
    return m_cacheValue;
}

bool NetworkBaseEditLine::alwaysUpdate() const
{
    return m_alwaysUpdate;
}

bool NetworkBaseEditLine::setKeyAlways() const
{
    return m_setKeyAlways;
}

void NetworkBaseEditLine::setDBusKey(const QJsonValue &key)
{
    if (key != cacheValue() || isValueError() || setKeyAlways()) {

        QString json = "\"";

        if(key.isString())
            json.append(key.toString());
        else if(key.isDouble())
            json.append(QString::number(key.toInt(-1)));

        json += "\"";

        qDebug() << key << json;

        m_dbus->SetKey(section(), this->key(), json).waitForFinished();
    }

    setCacheValue(key);
}

const QJsonValue NetworkBaseEditLine::dbusKey()
{
    const QString &json = m_dbus->GetKey(section(), key());
    const QJsonDocument &json_doc = QJsonDocument::fromJson("{\"value\":" + json.toUtf8() + "}");

    return json_doc.object()["value"];
}

const QJsonArray NetworkBaseEditLine::getAvailableValues()
{
    const QString &json = m_dbus->GetAvailableValues(section(), key());
    const QJsonDocument &json_doc = QJsonDocument::fromJson(json.toUtf8());

    return json_doc.array();
}

const QList<QJsonValue> NetworkBaseEditLine::getAvailableValuesValue()
{
    QList<QJsonValue> list;
    const QJsonArray &array = getAvailableValues();

    if(array.isEmpty()) {
        return list;
    }

    for(const QJsonValue &value : array) {
        list << value.toObject()["Value"];
    }

    return list;
}

const QString NetworkBaseEditLine::getAvailableValuesTextByValue()
{
    const QJsonArray &array = getAvailableValues();

    if(!array.isEmpty() && !cacheValue().isNull()) {
        for(const QJsonValue &value : array) {
            const QJsonObject &obj = value.toObject();
            if(obj["Value"] == cacheValue()) {
                return obj["Text"].toString();
            }
        }
    }

    return "";
}

const QStringList NetworkBaseEditLine::getAvailableValuesText()
{
    const QJsonArray &array = getAvailableValues();

    QStringList list;

    if(array.isEmpty()) {
        return list;
    }

    for(const QJsonValue &value : array) {
        list << value.toObject()["Text"].toString();
    }

    return list;
}

int NetworkBaseEditLine::getAvailableValuesIndex()
{
    const QJsonArray &array = getAvailableValues();

    if(!array.isEmpty() && !cacheValue().isNull()) {
        for(int i = 0; i < array.count(); ++i) {
            const QJsonObject &obj = array[i].toObject();
            if(obj["Value"] == cacheValue()) {
                return i;
            }
        }
    }

    return -1;
}

void NetworkBaseEditLine::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);

    if(m_cacheValue.isNull() || alwaysUpdate()) {
        setCacheValue(dbusKey());
    } else {
        emit widgetShown();
    }

    emit showed();
}

void NetworkBaseEditLine::setCacheValue(const QJsonValue &value)
{
    if(m_cacheValue == value)
        return;

    m_cacheValue = value;

    emit cacheValueChanged();
}

void NetworkBaseEditLine::updateVisible()
{
    setVisible(m_dbus->availableSections().indexOf(section()) != -1
            &&  m_dbus->availableKeys()[section()].indexOf(key()) != -1);
}

void NetworkBaseEditLine::setSection(const QString &section)
{
    m_section = section;
}

void NetworkBaseEditLine::setKey(const QString &key)
{
    m_key = key;
}

void NetworkBaseEditLine::setAlwaysUpdate(bool alwaysUpdate)
{
    m_alwaysUpdate = alwaysUpdate;
}

void NetworkBaseEditLine::setSetKeyAlways(bool setKeyAlways)
{
    m_setKeyAlways = setKeyAlways;
}

bool NetworkBaseEditLine::isValueError()
{
    return !m_dbus->errors().isEmpty() && !m_dbus->errors()[section()].isEmpty()
            && !m_dbus->errors()[section()][key()].isEmpty();
}
