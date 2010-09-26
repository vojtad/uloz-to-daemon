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
