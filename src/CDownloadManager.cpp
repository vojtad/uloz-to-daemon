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

#include "CDownloadManager.h"

#include <QDebug>

CDownloadManager::CDownloadManager(QObject * parent) :
	QObject(parent),
	m_activeCount(0)
{
	m_queue = true;
}

quint8 CDownloadManager::maxDownloads() const
{
	return m_maxDownloads;
}

void CDownloadManager::setMaxDownloads(quint8 count)
{
	quint8 old = m_maxDownloads;

	m_maxDownloads = count;
	if(count > old)
		startDownloads();
}

const QStringList & CDownloadManager::userAgents() const
{
	return m_userAgents;
}

void CDownloadManager::setUserAgents(const QStringList & list)
{
	m_userAgents = list;
}

const QString & CDownloadManager::downloadDir() const
{
	return m_downloadDir;
}

bool CDownloadManager::queue() const
{
	return m_queue;
}

void CDownloadManager::setQueue(bool active)
{
	m_queue = active;
}

void CDownloadManager::setDownloadDir(const QString & dir)
{
	m_downloadDir = dir;
}

void CDownloadManager::addDownload(const QString & url, bool autoStart)
{
	DownloadData data;
	data.url = url;
	data.downloadDir = m_downloadDir;
	if(m_queue && autoStart)
		data.state = STATE_WAITING;

	if(!m_userAgents.isEmpty())
		data.userAgent = m_userAgents.at(qrand() % m_userAgents.count());
	else
		qWarning("No User-Agent loaded. Using empty one.");

	CDownload * download = new CDownload(data);
	connect(download, SIGNAL(done(bool)), this, SLOT(done(bool)));

	m_downloads.push_back(download);

	if(autoStart && (!m_queue || m_activeCount < m_maxDownloads))
	{
		download->start();
		++m_activeCount;
	}
}

void CDownloadManager::startDownloads()
{
	if(!m_queue || m_activeCount >= m_maxDownloads)
		return;

	foreach(CDownload * d, m_downloads)
	{
		if(d->data().state == STATE_WAITING)
		{
			d->start();
			++m_activeCount;
			if(m_activeCount == m_maxDownloads)
				break;
		}
	}
}

void CDownloadManager::startDownload(quint32 i)
{
	CDownload * download = findDownload(i);

	if(download != 0 && download->data().canStart(m_queue))
	{
		if(!m_queue || m_activeCount < m_maxDownloads)
		{
			download->data().reset();
			download->start();
			++m_activeCount;
		}
		else
		{
			download->data().state = STATE_WAITING;
		}
	}
}

void CDownloadManager::removeDownload(quint32 i)
{
	m_removeQueue.push_back(i);
}

void CDownloadManager::abortDownload(quint32 i)
{
	CDownload * download = findDownload(i);

	if(download != 0 && download->data().canAbort())
	{
		download->stop();
	}
}

void CDownloadManager::done(bool ok)
{
	Q_UNUSED(ok);

	--m_activeCount;

	startDownloads();
}

DownloadList & CDownloadManager::downloads()
{
	return m_downloads;
}

CDownload * CDownloadManager::findDownload(quint32 id)
{
	foreach(CDownload * d, m_downloads)
	{
		if(d->data().id == id)
			return d;
	}

	return 0;
}

QList<quint32> CDownloadManager::updateRemoveQueue()
{
	QList<quint32> removed;

	QList<quint32>::iterator it = m_removeQueue.begin();
	while(it != m_removeQueue.end())
	{
		CDownload * d = findDownload(*it);
		if(d == 0)
		{
			it = m_removeQueue.erase(it);
		}
		else
		{
			if(d->data().canRemove())
			{
				removed.push_back(d->data().id);
				m_downloads.removeOne(d);
				delete d;
			}
			else
			{
				d->stop();
			}

			++it;
		}
	}

	return removed;
}
