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

#ifndef CCAPTCHALETTER_H
#define CCAPTCHALETTER_H

#include <QList>
#include <QString>

class CCaptchaLetter
{
	public:
		CCaptchaLetter(const QString & data);

		quint32 absDiff(const QList<quint32> & spikeCount) const;
		char letter() const;

	private:
		char m_letter;
		QList<quint32> m_spikeCount;
};

#endif // CCAPTCHALETTER_H
