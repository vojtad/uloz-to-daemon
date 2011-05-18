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

#include "CDownload.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QThreadPool>
#include <QUrl>

#include "CCaptcha.h"

QAtomicInt CDownload::m_idGenerator = 0;

CDownload::CDownload(const DownloadData & data, QObject * parent) :
	QObject(parent),
	m_file(0),
	m_manager(new QNetworkAccessManager(this)),
	m_reply(0),
	m_captcha(0),
	m_data(data)
{
	m_data.id = m_idGenerator.fetchAndAddRelaxed(1);
	m_data.update = true;

	m_timeoutTimer.setInterval(10000);
	m_timer.setSingleShot(true);
	m_timer.setInterval(2000);
	m_timer.setSingleShot(true);

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(start()));
	connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(netTimeout()));
}

DownloadData & CDownload::data()
{
	return m_data;
}

const DownloadData & CDownload::data() const
{
	return m_data;
}

void CDownload::start()
{
	m_data.fileName = m_data.name;
	if(m_data.fileName.isEmpty())
		m_data.fileName = QString("file_%1").arg(m_data.url);

	QNetworkRequest request;
	request.setUrl(QUrl(m_data.url));
	request.setRawHeader("User-Agent", m_data.userAgent.toAscii());

	m_reply = m_manager->get(request);
	connect(m_reply, SIGNAL(finished()), this, SLOT(retrieveCaptchaUrl()));
	connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
	connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));

	m_data.state = STATE_RETRIEVING_URL;
	m_data.miscState = 0;
	m_data.update = true;
}

void CDownload::netTimeout()
{
	m_reply->abort();
}

void CDownload::stop()
{
	if(m_data.state == STATE_WILL_RETRY)
	{
		m_data.state = STATE_ABORTED;
		m_data.update = true;

		m_timer.stop();
		emit done(false);
	}
	else
	{
		m_data.state = STATE_ABORTING;
		m_data.update = true;

		if(m_reply != 0)
		{
			m_reply->abort();
		}

		if(m_captcha != 0)
		{
			m_captcha->abort();
		}
	}
}

void CDownload::retry()
{
	if(m_data.state == STATE_ABORTING)
	{
		m_data.state = STATE_ABORTED;
		m_data.update = true;

		emit done(false);
	}
	else if(m_data.triesLeft == 0)
	{
		emit done(false);
	}
	else
	{
		--m_data.triesLeft;
		m_data.downloaded = 0;
		m_data.size = 0;
		m_data.state = STATE_WILL_RETRY;
		m_data.miscState = 0;
		m_data.update = true;

		m_timer.start();
	}
}

void CDownload::retrieveCaptchaUrl()
{
	if(m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid())
	{
		m_reply->deleteLater();

		QNetworkRequest request;
		request.setUrl(m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
		request.setRawHeader("User-Agent", m_data.userAgent.toAscii());

		m_reply = m_manager->get(request);
		connect(m_reply, SIGNAL(finished()), this, SLOT(retrieveCaptchaUrl()));
		connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
		connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));
	}
	else if(m_reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = m_reply->readAll();
		QRegExp rxSubmitUrl("form name=\"dwn\" action=\"([^\"]+)\"");
		QRegExp rxCaptchaId("name=\"captcha_nb\" value=\"([0-9]+)\"");

		m_reply->deleteLater();
		m_reply = 0;

		if(rxSubmitUrl.indexIn(data, 0) == -1)
		{
			handleError(ERROR_PARSE_SUBMIT_URL);
		}
		else if(rxCaptchaId.indexIn(data, 0) == -1)
		{
			handleError(ERROR_PARSE_CAPTCHA_ID);
		}
		else
		{
			bool ok;
			m_captchaId = rxCaptchaId.cap(1).toInt(&ok);
			if(!ok)
			{
				handleError(ERROR_PARSE_CAPTCHA_ID);
				return;
			}

			m_submitUrl = rxSubmitUrl.cap(1);

			QNetworkRequest request;
			request.setUrl(QUrl(QString("http://img.uloz.to/captcha/sound/%1.mp3").arg(m_captchaId)));
			request.setRawHeader("User-Agent", m_data.userAgent.toAscii());

			m_reply = m_manager->get(request);

			connect(m_reply, SIGNAL(finished()), this, SLOT(downloadAndSolveCaptcha()));
			connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
			connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));

			m_data.state = STATE_CAPTCHA;
			m_data.update = true;
		}
	}
	else
	{
		handleNetError();
	}
}

void CDownload::downloadAndSolveCaptcha()
{
	if(m_reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = m_reply->readAll();

		m_reply->deleteLater();
		m_reply = 0;

		m_captcha = new CCaptcha(data, this);
		connect(m_captcha, SIGNAL(finished(bool, const QString &)),
				this, SLOT(captchaSolved(bool, const QString &)));
		QThreadPool::globalInstance()->start(m_captcha);
	}
	else
	{
		handleNetError();
	}
}

void CDownload::captchaSolved(bool ok, const QString & captcha)
{
	if(ok && !captcha.isEmpty() && captcha.indexOf("-") == -1)
	{
		QString postData;
		QNetworkRequest request;

		m_captcha = 0;

		postData.append(QString("captcha_nb=%1").arg(m_captchaId));
		postData.append(QString("&captcha_user=%1").arg(captcha));

		request.setUrl(QUrl(m_submitUrl));
		request.setRawHeader("User-Agent", m_data.userAgent.toAscii());
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
		m_reply = m_manager->post(request, postData.toAscii());

		connect(m_reply, SIGNAL(finished()), this, SLOT(preDownloadRedirect()));
		connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
		connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));
	}
	else
	{
		handleError(ERROR_CANNOT_SOLVE_CAPTCHA);
	}
}

void CDownload::preDownloadRedirect()
{
	if(m_reply->error() == QNetworkReply::NoError)
	{
		QVariant redir = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		if(redir.isValid())
		{
			QString url = redir.toString();
			if(url.indexOf("no#cpt") != -1)
			{
				handleError(ERROR_CANNOT_SOLVE_CAPTCHA);
				return;
			}

			QNetworkRequest request;
			request.setUrl(QUrl(m_reply->rawHeader("Location")));
			request.setRawHeader("User-Agent", m_data.userAgent.toAscii());

			m_reply = m_manager->get(request);

			connect(m_reply, SIGNAL(finished()), this, SLOT(beginDownload()));
			connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
			connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));

			m_data.state = STATE_DOWNLOADING;
			m_data.update = true;
		}
	}
	else
	{
		handleNetError();
	}
}

void CDownload::beginDownload()
{
	if(m_reply->error() == QNetworkReply::NoError)
	{
		QVariant redir = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		if(redir.isValid())
		{
			QString url = redir.toString();
			if(url.indexOf("no#cpt") != -1)
			{
				handleError(ERROR_CANNOT_SOLVE_CAPTCHA);
				return;
			}

			quint32 i = 1;
			QDir dir(m_data.downloadDir);
			QFileInfo info(dir.filePath(m_data.fileName));
			while(QFile::exists(dir.filePath(m_data.fileName)))
			{
				m_data.fileName = QString("%1 (%2).%3").arg(info.baseName(),
															QString::number(i++),
															info.completeSuffix());
			}

			m_file = new QFile(dir.filePath(m_data.fileName));
			if(!m_file->open(QIODevice::WriteOnly))
			{
				handleError(ERROR_CANNOT_OPEN_FILE);
				return;
			}

			QNetworkRequest request;

			request.setUrl(QUrl(m_reply->rawHeader("Location")));
			request.setRawHeader("User-Agent", m_data.userAgent.toAscii());

			m_reply->deleteLater();
			m_reply = m_manager->get(request);

			m_timeoutTimer.setInterval(30000);

			connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)),
					this, SLOT(updateProgress(qint64,qint64)));
			connect(m_reply, SIGNAL(readyRead()), this, SLOT(writeData()));
			connect(m_reply, SIGNAL(finished()), this, SLOT(finishDownload()));
			connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), &m_timeoutTimer, SLOT(start()));
			connect(m_reply, SIGNAL(finished()), &m_timeoutTimer, SLOT(stop()));
		}
	}
	else
	{
		handleNetError();
	}
}

void CDownload::finishDownload()
{
	if(m_reply->isFinished() && m_reply->error() == QNetworkReply::NoError)
	{
		if(m_data.size == 0)
		{
			retry();
		}
		else
		{
			m_data.state = STATE_FINISHED;
			m_data.update = true;

			m_file->close();
			delete m_file;
			m_file = 0;

			m_reply->deleteLater();
			m_reply = 0;

			emit done(true);
		}
	}
	else
	{
		handleNetError();
	}
}

void CDownload::updateProgress(qint64 downloaded, qint64 size)
{
	m_data.downloaded = downloaded;
	m_data.size = size;
	m_data.update = true;
}

void CDownload::handleNetError()
{
	if(m_file != 0 && m_file->isOpen())
	{
		m_file->close();
	}

	if(m_data.state != STATE_ABORTING)
	{
		m_data.state = STATE_NET_ERROR;
		m_data.miscState = m_reply->error();
		m_data.update = true;

		if(m_file != 0)
		{
			m_file->remove();
		}
	}

	delete m_file;
	m_file = 0;
	m_reply->deleteLater();
	m_reply = 0;

	if(m_file != 0)
	{
		if(m_file->isOpen())
			m_file->close();
		delete m_file;
		m_file = 0;
	}

	retry();
}

void CDownload::handleError(Errors error)
{
	if(m_file != 0 && m_file->isOpen())
	{
		m_file->close();
	}

	if(m_data.state != STATE_ABORTING)
	{
		m_data.state = STATE_ERROR;
		m_data.miscState = error;
		m_data.update = true;

		if(m_file != 0)
		{
			m_file->remove();
		}
	}

	delete m_file;
	m_file = 0;
	if(m_reply != 0)
	{
		m_reply->deleteLater();
		m_reply = 0;
	}

	retry();
}

void CDownload::writeData()
{
	m_file->write(m_reply->readAll());
}

void DownloadData::serialize(QDataStream & stream) const
{
	stream << id << name << url << userAgent << fileName << quint8(state) << miscState
			<< downloaded << size << speed;
}

void DownloadData::unserialize(QDataStream & stream)
{
	quint8 st;

	stream >> id >> name >> url >> userAgent >> fileName >> st >> miscState
			>> downloaded >> size >> speed;

	state = DownloadState(st);
}

void DownloadData::recalcSpeed()
{
	time_t now = QDateTime::currentDateTime().toTime_t();
	if((now - lastSpeedCalcTime) != 0)
	{
		speed = (downloaded - lastSpeedCalcBytes) / (now - lastSpeedCalcTime);
		lastSpeedCalcTime = now;
	}
	else
	{
		lastSpeedCalcTime = now;
	}

	lastSpeedCalcBytes = downloaded;
}

void DownloadData::reset()
{
	downloaded = 0;
	size = 0;
	speed = 0;
	triesLeft = DOWNLOAD_TRIES;
	lastSpeedCalcTime = 0;
	lastSpeedCalcBytes = 0;

	update = true;
}

bool DownloadData::canRemove() const
{
	return (state == STATE_WAITING || state == STATE_FINISHED || state == STATE_ERROR ||
			state == STATE_NET_ERROR || state == STATE_ABORTED);
}

bool DownloadData::canStart(bool queue) const
{
	return (state == STATE_NONE || state == STATE_NET_ERROR || state == STATE_ERROR ||
			state == STATE_ABORTED || (!queue && state == STATE_WAITING));
}

bool DownloadData::canAbort() const
{
	return (state == STATE_WAITING || state == STATE_CAPTCHA || state == STATE_RETRIEVING_URL ||
			state == STATE_DOWNLOADING || state == STATE_WILL_RETRY);
}
