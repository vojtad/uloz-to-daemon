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

#include "CCaptchaLetter.h"

#include <QDebug>
#include <QStringList>

CCaptchaLetter::CCaptchaLetter(const QString & data)
{
	QStringList list = data.split(";");
	QString lt = list.takeFirst();
	m_letter = lt.at(0).toAscii();
	list.removeLast();

	foreach(const QString & s, list)
	{
		m_spikeCount.push_back(s.toULong());
	}
}

quint32 CCaptchaLetter::absDiff(const QList<quint32> & spikeCount) const
{
	quint32 ret = 0;
	for(size_t i = 0; i < spikeCount.count(); ++i)
	{
		ret += qAbs(m_spikeCount.at(i) - spikeCount.at(i));
	}

	return ret;
}

char CCaptchaLetter::letter() const
{
	return m_letter;
}
