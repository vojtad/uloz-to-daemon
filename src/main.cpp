#include <QtCore/QCoreApplication>

#ifdef Q_OS_LINUX
#include <signal.h>
#endif

#include "CDaemon.h"

#ifdef Q_OS_LINUX
static int setup_unix_signal_handlers()
{
	struct sigaction sint, term;

	sint.sa_handler = CDaemon::intSignalHandler;
	sigemptyset(&sint.sa_mask);
	sint.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sint, 0) > 0)
	   return 1;

	term.sa_handler = CDaemon::termSignalHandler;
	sigemptyset(&term.sa_mask);
	term.sa_flags = SA_RESTART;

	if (sigaction(SIGTERM, &term, 0) > 0)
	   return 2;

	return 0;
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
	{
		int ret = setup_unix_signal_handlers();
		if(ret != 0)
			return ret;
	}
#endif

	QCoreApplication::setApplicationName("uloz-to-daemon");
	QCoreApplication::setOrganizationDomain("dev-z.cz");

    QCoreApplication a(argc, argv);

	CDaemon daemon;

	int ret = daemon.run();
	if(ret != 0)
		return ret;

	return a.exec();
}
