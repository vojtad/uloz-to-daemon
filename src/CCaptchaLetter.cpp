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
