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

#ifndef CCAPTCHA_H
#define CCAPTCHA_H

#include <QObject>
#include <QRunnable>
#include <QVector>
#include <mad.h>

class CCaptcha;

struct CaptchaInternalData
{
	CaptchaInternalData()
	{
		letterIndex = -1;
		zeroCount = 0;
		input = false;
		space = false;
		abort = false;
		captcha = 0;
	}

	QByteArray data;
	qint8 letterIndex;
	quint32 zeroCount;
	bool input;
	bool space;
	bool abort;
	CCaptcha * captcha;
};

class CCaptcha : public QObject, public QRunnable
{
	Q_OBJECT
	public:
		CCaptcha(const QByteArray & data, QObject * parent = 0);

		void run();
		void abort();

		void analyzeSample(qint32 sample);
		static mad_flow input(void * data, mad_stream * stream);
		static mad_flow output(void * data, mad_header const * header, mad_pcm * pcm);

	private:
		CaptchaInternalData m_data;
		QVector<quint32> m_spikeCount[4]; // 4 letters

	signals:
		void finished(bool ok, const QString & captcha);
};

#endif // CCAPTCHA_H
