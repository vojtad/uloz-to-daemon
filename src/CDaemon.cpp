/*
 * uloz-to-daemon
 * Copyright (C) 2010 Vojta Drbohlav <vojta.d@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CDaemon.h"

#include <QCoreApplication>
#include <QDir>
#include <QSocketNotifier>
#include <QTcpSocket>

#include <sys/socket.h>

QList<CCaptchaLetter> CDaemon::captchaLetters;

int CDaemon::m_sigIntFd[2] = {0, 0};
int CDaemon::m_sigTermFd[2] = {0, 0};

CDaemon::CDaemon(QObject *parent) :
	QObject(parent),
	m_server(0)
{
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigIntFd))
		qFatal("Couldn't create INT socket pair.");

	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create TERM socket pair.");

	m_sigInt = new QSocketNotifier(m_sigIntFd[1], QSocketNotifier::Read, this);
	m_sigTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	m_server = new QTcpServer(this);

	connect(m_sigInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
	connect(m_sigTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(terminate()));
	connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
	connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(sendUpdate()));

	loadSettings();
}

CDaemon::~CDaemon()
{
	saveSettings();
}

#include <QDebug>

int CDaemon::run()
{
	if(!m_captchaLettersFile.isEmpty())
	{
		QFile file(m_captchaLettersFile);
		if(file.open(QIODevice::Text | QIODevice::ReadOnly))
		{
			while(!file.atEnd())
			{
				CDaemon::captchaLetters.push_back(CCaptchaLetter(file.readLine()));
			}
			file.close();
		}
	}

	if(CDaemon::captchaLetters.isEmpty())
	{
		qWarning("Unable to load captcha letters from '%s'.",
			   m_captchaLettersFile.toStdString().c_str());
		return 1;
	}

	if(!m_server->listen(m_listenAddress, m_listenPort))
	{
		qWarning("Unable to bind on '%s:%u'.", m_listenAddress.toString().toStdString().c_str(),
			   m_listenPort);
		return 2;
	}

	m_updateTimer.start();

	return 0;
}

void CDaemon::intSignalHandler(int)
{
	char a = 1;
	::write(m_sigIntFd[0], &a, sizeof(a));
}

void CDaemon::termSignalHandler(int)
{
	char a = 1;
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void CDaemon::handleSigInt()
{
	m_sigInt->setEnabled(false);
	char tmp;
	::read(m_sigIntFd[1], &tmp, sizeof(tmp));
	qApp->quit();
	m_sigInt->setEnabled(true);
}

void CDaemon::handleSigTerm()
{
	m_sigTerm->setEnabled(false);
	char tmp;
	::read(m_sigTermFd[1], &tmp, sizeof(tmp));
	qApp->quit();
	m_sigTerm->setEnabled(true);
}

void CDaemon::terminate()
{
	if(m_server != 0)
		m_server->close();
}

void CDaemon::loadSettings()
{
	m_settings.beginGroup("communication");
	m_listenAddress = QHostAddress(m_settings.value("listenAddress", "127.0.0.1").toString());
	m_listenPort = m_settings.value("listenPort", 1234).toInt();
	m_updateTimer.setInterval(m_settings.value("updateInterval", 1000).toInt());
	m_settings.endGroup();

	m_settings.beginGroup("downloads");
	m_downloadManager.setQueue(m_settings.value("queueEnabled", true).toBool());
	m_downloadManager.setMaxDownloads(m_settings.value("maxActiveDownloads", 5).toUInt());
	m_downloadManager.setUserAgents(m_settings.value("userAgents").toStringList());
	m_downloadManager.setDownloadDir(m_settings.value("downloadDir", QDir::homePath()).toString());
	m_settings.endGroup();

	m_settings.beginGroup("captchaSolver");
	m_captchaLettersFile = m_settings.value("lettersFile",
											"/usr/share/uloz-to-daemon/samples").toString();
	m_settings.endGroup();
}

void CDaemon::saveSettings()
{
	m_settings.beginGroup("communication");
	m_settings.setValue("listenAddress", m_listenAddress.toString());
	m_settings.setValue("listenPort", m_listenPort);
	m_settings.setValue("updateInterval", m_updateTimer.interval());
	m_settings.endGroup();

	m_settings.beginGroup("downloads");
	m_settings.setValue("queueEnabled", m_downloadManager.queue());
	m_settings.setValue("maxActiveDownloads", m_downloadManager.maxDownloads());
	m_settings.setValue("downloadDir", m_downloadManager.downloadDir());
	m_settings.endGroup();

	m_settings.beginGroup("captchaSolver");
	m_settings.setValue("lettersFile", m_captchaLettersFile);
	m_settings.endGroup();
}
