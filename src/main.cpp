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
