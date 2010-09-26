#ifndef CCAPTCHASOLVER_H
#define CCAPTCHASOLVER_H

#include <QThread>

#include <mad.h>

#include "CCaptchaLetter.h"

struct SolverData
{
	QByteArray data;

	quint32 zeroCount;
	bool space;
	bool input;
	qint8 index;
};

class CCaptchaSolver : public QThread
{
	Q_OBJECT
	public:
		CCaptchaSolver(const QString & dataFile, QObject * parent = 0);

		void run();

		QString solve(const QByteArray & data);

	private:
		QString m_dataFile;

		SolverData m_data;

	signals:

	public slots:

};

#endif // CAPTCHASOLVER_H
