#ifndef CDAEMON_H
#define CDAEMON_H

#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QSettings>
#include <QTcpServer>
#include <QTimer>

#include "CCaptchaLetter.h"
#include "CDownloadManager.h"

class QSocketNotifier;
class QTcpSocket;

enum ClientOpcodes
{
	OPCODE_ADD = 0, // data = url
	OPCODE_REMOVE, // data = id
	OPCODE_START, // data = id
	OPCODE_STOP, // data = id
	OPCODE_LIST,
	OPCODE_QUEUE
};

class CDaemon : public QObject
{
	typedef QList<QTcpSocket *> ClientList;

	Q_OBJECT
	public:
		CDaemon(QObject * parent = 0);
		~CDaemon();

		int run();

		static void intSignalHandler(int unsused);
		static void termSignalHandler(int unsused);

		static QList<CCaptchaLetter> captchaLetters;

	private slots:
		void newConnection();
		void handleSigInt();
		void handleSigTerm();
		void terminate();

		void handleClientRead();
		void handleClientDisconnected();

		void sendUpdate();

	private:
		void loadSettings();
		void saveSettings();

		void handleAdd(QDataStream & stream);
		void handleRemove(QDataStream & stream);
		void handleStart(QDataStream & stream);
		void handleAbort(QDataStream & stream);
		void handleQueue(QDataStream & stream);

	private:
		static int m_sigIntFd[2];
		static int m_sigTermFd[2];
		QSocketNotifier * m_sigInt;
		QSocketNotifier * m_sigTerm;

		QSettings m_settings;
		QHostAddress m_listenAddress;
		quint16 m_listenPort;

		QTcpServer * m_server;
		ClientList m_clients;

		QString m_captchaLettersFile;

		CDownloadManager m_downloadManager;
		QTimer m_updateTimer;
};

#endif // CDAEMON_H
