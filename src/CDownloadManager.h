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

#ifndef CDOWNLOADMANAGER_H
#define CDOWNLOADMANAGER_H

#include <QList>
#include <QNetworkAccessManager>
#include <QStringList>
#include <QThread>

#include "CDownload.h"

enum DownloadLists
{
	LIST_ACTIVE = 0,
	LIST_WAITING,
	LIST_INACTIVE
};

typedef QList<CDownload *> DownloadList;

class CDownloadManager : public QObject
{
	Q_OBJECT

	public:
		CDownloadManager(QObject * parent = 0);

		quint8 maxDownloads() const;
		void setMaxDownloads(quint8 count);

		const QStringList & userAgents() const;
		void setUserAgents(const QStringList & list);

		const QString & downloadDir() const;
		void setDownloadDir(const QString & dir);

		bool queue() const;
		void setQueue(bool active);

		void addDownload(const QString & name, const QString & url, bool autoStart);
		void startDownloads();
		void startDownload(quint32 i);
		void removeDownload(quint32 i);
		void abortDownload(quint32 i);

		QList<quint32> updateRemoveQueue();

		DownloadList & downloads();

	private:
		CDownload * findDownload(quint32 id);
		void updateWaiting();

	private:
		quint8 m_maxDownloads;
		QStringList m_userAgents;
		QString m_downloadDir;

		QList<quint32> m_removeQueue;
		DownloadList m_downloads;
		bool m_queue;
		quint32 m_activeCount;

	private slots:
		void done(bool ok);
};

#endif // CDOWNLOADMANAGER_H
