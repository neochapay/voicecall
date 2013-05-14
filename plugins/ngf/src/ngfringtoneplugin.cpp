/*
 * This file is a part of the Voice Call Manager project
 *
 * Copyright (C) 2011-2012  Tom Swindell <t.swindell@rubyx.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include "common.h"
#include "ngfringtoneplugin.h"

#include <voicecallmanagerinterface.h>

#include <NgfClient>

#include <QtPlugin>

class NgfRingtonePluginPrivate
{
    Q_DECLARE_PUBLIC(NgfRingtonePlugin)

public:
    NgfRingtonePluginPrivate(NgfRingtonePlugin *q)
        : q_ptr(q), manager(NULL), currentCall(NULL), ngf(NULL), ringtoneEventId(-1)
    { /* ... */ }

    NgfRingtonePlugin *q_ptr;
    VoiceCallManagerInterface *manager;

    AbstractVoiceCallHandler *currentCall;

    Ngf::Client *ngf;
    quint32 ringtoneEventId;
};

NgfRingtonePlugin::NgfRingtonePlugin(QObject *parent)
    : AbstractVoiceCallManagerPlugin(parent), d_ptr(new NgfRingtonePluginPrivate(this))
{
    TRACE
}

NgfRingtonePlugin::~NgfRingtonePlugin()
{
    TRACE
    delete this->d_ptr;
}

QString NgfRingtonePlugin::pluginId() const
{
    TRACE
    return PLUGIN_NAME;
}

QString NgfRingtonePlugin::pluginVersion() const
{
    TRACE
    return PLUGIN_VERSION;
}

bool NgfRingtonePlugin::initialize()
{
    TRACE
    Q_D(NgfRingtonePlugin);
    d->ngf = new Ngf::Client(this);
    return true;
}

bool NgfRingtonePlugin::configure(VoiceCallManagerInterface *manager)
{
    TRACE
    Q_D(NgfRingtonePlugin);
    d->manager = manager;

    QObject::connect(d->ngf, SIGNAL(connectionStatus(bool)), SLOT(onConnectionStatus(bool)));
    QObject::connect(d->ngf, SIGNAL(eventFailed(quint32)), SLOT(onEventFailed(quint32)));
    QObject::connect(d->ngf, SIGNAL(eventCompleted(quint32)), SLOT(onEventCompleted(quint32)));
    QObject::connect(d->ngf, SIGNAL(eventPlaying(quint32)), SLOT(onEventPlaying(quint32)));
    QObject::connect(d->ngf, SIGNAL(eventPaused(quint32)), SLOT(onEventPaused(quint32)));

    return true;
}

bool NgfRingtonePlugin::start()
{
    TRACE
    Q_D(NgfRingtonePlugin);

    QObject::connect(d->manager, SIGNAL(voiceCallAdded(AbstractVoiceCallHandler*)), SLOT(onVoiceCallAdded(AbstractVoiceCallHandler*)));
    QObject::connect(d->manager, SIGNAL(silenceRingtoneRequested()), SLOT(onSilenceRingtoneRequested()));

    d->ngf->connect();
    return true;
}

bool NgfRingtonePlugin::suspend()
{
    TRACE
    return true;
}

bool NgfRingtonePlugin::resume()
{
    TRACE
    return true;
}

void NgfRingtonePlugin::finalize()
{
    TRACE
}

void NgfRingtonePlugin::onVoiceCallAdded(AbstractVoiceCallHandler *handler)
{
    TRACE
    Q_D(NgfRingtonePlugin);

    QObject::connect(handler, SIGNAL(statusChanged()), SLOT(onVoiceCallStatusChanged()));
    d->currentCall = handler;
}

void NgfRingtonePlugin::onVoiceCallStatusChanged()
{
    TRACE
    Q_D(NgfRingtonePlugin);
    DEBUG_T(QString("Voice call status changed to: ") + d->currentCall->statusText());

    if(d->currentCall->status() != AbstractVoiceCallHandler::STATUS_INCOMING)
    {
        DEBUG_T("Disconnecting from handler");
        QObject::disconnect(d->currentCall, SIGNAL(statusChanged()), this, SLOT(onVoiceCallStatusChanged()));

        d->currentCall = NULL;

        if(d->ringtoneEventId != -1)
        {
            DEBUG_T("Stopping ringtone");
            d->ngf->stop("ringtone");
            d->ringtoneEventId = -1;
        }
    } else if(d->ringtoneEventId == -1) {
        QMap<QString, QVariant> props;
        //props.insert("media.audio", true);

        if(d->currentCall->provider()->providerType() != "tel")
        {
            props.insert("type", "voip");
        }

        d->ringtoneEventId = d->ngf->play("ringtone", props);
        DEBUG_T(QString("Playing ringtone, event id: %1").arg(d->ringtoneEventId));
    }
}

void NgfRingtonePlugin::onSilenceRingtoneRequested()
{
    TRACE
    Q_D(NgfRingtonePlugin);
    if(d->ringtoneEventId != -1)
    {
        d->ngf->pause("ringtone");
        d->ringtoneEventId = -1;
    }
}

void NgfRingtonePlugin::onConnectionStatus(bool connected)
{
    TRACE
    DEBUG_T(QString("Connection to NGF daemon changed to: " + connected ? "connected" : "disconnected"));
}

void NgfRingtonePlugin::onEventFailed(quint32 eventId)
{
    TRACE
}

void NgfRingtonePlugin::onEventCompleted(quint32 eventId)
{
    TRACE
}

void NgfRingtonePlugin::onEventPlaying(quint32 eventId)
{
    TRACE
}

void NgfRingtonePlugin::onEventPaused(quint32 eventId)
{
    TRACE
}

Q_EXPORT_PLUGIN2(ngf-ringtone-plugin, NgfRingtonePlugin)