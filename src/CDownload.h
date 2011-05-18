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

#ifndef CDOWNLOAD_H
#define CDOWNLOAD_H

#include <QDataStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QTimer>
#include <QWaitCondition>

#define DOWNLOAD_TRIES 100

enum DownloadState
{
	STATE_NONE = 0,
	STATE_WAITING,
	STATE_CAPTCHA,
	STATE_RETRIEVING_URL,
	STATE_DOWNLOADING,
	STATE_FINISHED,
	STATE_NET_ERROR,
	STATE_ERROR,
	STATE_WILL_RETRY,
	STATE_ABORTED,
	STATE_ABORTING
};

enum Errors
{
	ERROR_PARSE_SUBMIT_URL = 0,
	ERROR_PARSE_CAPTCHA_ID,
	ERROR_CANNOT_SOLVE_CAPTCHA,
	ERROR_CANNOT_OPEN_FILE
};

struct DownloadData
{
	DownloadData()
	{
		id = 0;
		state = STATE_NONE;
		miscState = 0;
		downloaded = 0;
		size = 0;
		speed = 0;
		triesLeft = DOWNLOAD_TRIES;
		update = false;
		lastSpeedCalcTime = 0;
		lastSpeedCalcBytes = 0;
	}

	DownloadData(const DownloadData & data) :
		id(data.id),
		name(data.name),
		url(data.url),
		userAgent(data.userAgent),
		fileName(data.fileName),
		downloadDir(data.downloadDir),
		state(data.state),
		miscState(data.miscState),
		downloaded(data.downloaded),
		size(data.size),
		speed(data.speed),
		triesLeft(data.triesLeft),

		update(data.update),
		lastSpeedCalcTime(data.lastSpeedCalcTime),
		lastSpeedCalcBytes(data.lastSpeedCalcBytes)
	{
	}

	quint32 id;
	QString name;
	QString url;
	QString userAgent;

	QString fileName;
	QString downloadDir;
	DownloadState state:8;
	quint16 miscState;
	qint64 downloaded;
	qint64 size;
	quint32 speed;
	quint32 triesLeft;

	void serialize(QDataStream & stream) const;
	void unserialize(QDataStream & stream);
	void recalcSpeed();
	void reset();

	bool canRemove() const;
	bool canStart(bool queue) const;
	bool canAbort() const;

	bool update;
	time_t lastSpeedCalcTime;
	qint64 lastSpeedCalcBytes;
};

class QFile;
class CCaptcha;

class CDownload : public QObject
{
	Q_OBJECT

	public:
		CDownload(const DownloadData & data, QObject * parent = 0);

		DownloadData & data();
		const DownloadData & data() const;

		void stop();

	private:
		void retry();

	private:
		QFile * m_file;
		QNetworkAccessManager * m_manager;
		QNetworkReply * m_reply;
		CCaptcha * m_captcha;
		DownloadData m_data;

		quint32 m_captchaId;
		QString m_submitUrl;
		QTimer m_timer;
		QTimer m_timeoutTimer;

		static QAtomicInt m_idGenerator;

	public slots:
		void start();
		void netTimeout();

	private slots:
		void retrieveCaptchaUrl();
		void downloadAndSolveCaptcha();
		void captchaSolved(bool ok, const QString & captcha);
		void preDownloadRedirect();
		void beginDownload();
		void finishDownload();

		void updateProgress(qint64 downloaded, qint64 size);
		void handleNetError();
		void handleError(Errors error);
		void writeData();

	signals:
		void done(bool ok);
};

#endif // CDOWNLOAD_H
