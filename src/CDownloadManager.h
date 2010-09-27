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

		void addDownload(const QString & url, bool autoStart);
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
