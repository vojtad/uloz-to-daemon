#include "CDaemon.h"

#include <QDebug>
#include <QTcpSocket>

void CDaemon::newConnection()
{
	while(m_server->hasPendingConnections())
	{
		QTcpSocket * client = m_server->nextPendingConnection();
		connect(client, SIGNAL(readyRead()), this, SLOT(handleClientRead()));
		//connect(client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleClientDisconnected()));
		connect(client, SIGNAL(disconnected()), this, SLOT(handleClientDisconnected()));
		m_clients.append(client);
	}
}

void CDaemon::handleClientRead()
{
	QTcpSocket * socket = qobject_cast<QTcpSocket *>(sender());

	while(socket->bytesAvailable() > 8)
	{
		QDataStream stream(socket);
		quint64 size;

		stream >> size;
		if(socket->bytesAvailable() >= size)
		{
			QByteArray buffer(socket->read(size));
			QDataStream bStream(buffer);
			quint8 opcode;

			bStream >> opcode;
			switch(opcode)
			{
			case OPCODE_ADD:
				handleAdd(bStream); break;
			case OPCODE_REMOVE:
				handleRemove(bStream); break;
			case OPCODE_START:
				handleStart(bStream); break;
			case OPCODE_STOP:
				handleAbort(bStream); break;
			default:
				qWarning() << "Unhandled packet:" << quint32(opcode); break;
			}
		}
	}
}

void CDaemon::handleClientDisconnected()
{
	QTcpSocket * client = qobject_cast<QTcpSocket *>(sender());
	m_clients.erase(qFind(m_clients.begin(), m_clients.end(), client));
}

void CDaemon::handleAdd(QDataStream & stream)
{
	bool autoStart = false;
	QString url;

	while(!stream.atEnd())
	{
		stream >> autoStart >> url;
		m_downloadManager.addDownload(url, autoStart);
	}
}

void CDaemon::handleRemove(QDataStream & stream)
{
	quint32 id;

	while(!stream.atEnd())
	{
		stream >> id;
		m_downloadManager.removeDownload(id);
	}
}

void CDaemon::handleStart(QDataStream & stream)
{
	quint32 id;

	while(!stream.atEnd())
	{
		stream >> id;
		m_downloadManager.startDownload(id);
	}
}

void CDaemon::handleAbort(QDataStream & stream)
{
	quint32 id;

	while(!stream.atEnd())
	{
		stream >> id;
		m_downloadManager.abortDownload(id);
	}
}

void CDaemon::sendUpdate()
{
	if(!m_clients.isEmpty())
	{
		if(!m_downloadManager.downloads().isEmpty())
		{
			QByteArray buffer;
			QDataStream stream(&buffer, QIODevice::WriteOnly);

			stream << quint64(0); // placeholder
			stream << quint8(OPCODE_LIST);

			foreach(CDownload * download, m_downloadManager.downloads())
			{
				download->data().recalcSpeed();
				download->data().serialize(stream);
			}

			stream.device()->seek(0);
			stream << quint64(buffer.size() - sizeof(quint64));

			foreach(QTcpSocket * c, m_clients)
			{
				if(c->isWritable())
				{
					c->write(buffer);
				}
			}
		}

		QList<quint32> removed = m_downloadManager.updateRemoveQueue();
		if(!removed.isEmpty())
		{
			QByteArray buffer;
			QDataStream stream(&buffer, QIODevice::WriteOnly);

			stream << quint64(0); // placeholder
			stream << quint8(OPCODE_REMOVE);

			while(!removed.isEmpty())
			{
				quint32 id = removed.takeFirst();
				stream << id;
			}

			stream.device()->seek(0);
			stream << quint64(buffer.size() - sizeof(quint64));

			foreach(QTcpSocket * c, m_clients)
			{
				if(c->isWritable())
				{
					c->write(buffer);
				}
			}
		}
	}
}
