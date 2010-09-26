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
	m_maxDownloads = count;
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
	if(!m_queue || m_activeCount == m_maxDownloads)
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
	CDownload * download = 0;
	foreach(CDownload * d, m_downloads)
	{
		if(d->data().id == i)
		{
			download = d;
		break;
		}
	}

	if(download != 0 && download->data().canStart())
	{
		if(!m_queue || m_activeCount < m_maxDownloads)
		{
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
	for(DownloadList::iterator it = m_downloads.begin(); it != m_downloads.end(); ++it)
	{
		if((*it)->data().id == i)
		{
			CDownload * d = *it;
			if(d->data().canRemove())
			{
				m_removed.push_back(d->data().id);
				m_downloads.erase(it);
				delete d;
			}
			break;
		}
	}
}

void CDownloadManager::abortDownload(quint32 i)
{
	CDownload * download = 0;
	foreach(CDownload * d, m_downloads)
	{
		if(d->data().id == i)
		{
			download = d;
			break;
		}
	}

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

QList<quint32> & CDownloadManager::removed()
{
	return m_removed;
}
