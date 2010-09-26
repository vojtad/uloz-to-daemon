#include "CCaptchaSolver.h"

#include <QFile>

CCaptchaSolver::CCaptchaSolver(const QString & dataFile, QObject * parent) :
	QThread(parent),
	m_dataFile(dataFile)
{
}

void CCaptchaSolver::run()
{
	{
		QFile file(m_dataFile);

		if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		while(!file.canReadLine())
		{
			m_letters.push_back(CCaptchaLetter(file.readLine(1000)));
		}

		file.close();
	}

	forever
	{
		if()
	}

}
