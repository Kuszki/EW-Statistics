/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Compute various statistics for EWMAPA software                         *
 *  Copyright (C) 2017  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the  Free Software Foundation, either  version 3 of the  License, or   *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This  program  is  distributed  in the hope  that it will be useful,   *
 *  but WITHOUT ANY  WARRANTY;  without  even  the  implied  warranty of   *
 *  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have  received a copy  of the  GNU General Public License   *
 *  along with this program. If not, see http://www.gnu.org/licenses/.     *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include "commonstructures.hpp"
#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	qRegisterMetaType<DBINFO>("DBINFO");

	qRegisterMetaType<QList<DBINFO>>("QList<DBINFO>");
	qRegisterMetaType<QVector<double>>("QVector<double>");
	qRegisterMetaType<QSet<QStringList>>("QSet<QStringList>");
	qRegisterMetaType<QMap<QString,bool>>("QMap<QString,bool>");

	qRegisterMetaType<QList<PaymentWidget::RECORD>>("QList<RECORD>");

	a.setApplicationName("EW-Statistics");
	a.setOrganizationName("Łukasz \"Kuszki\" Dróżdż");
	a.setOrganizationDomain("https://github.com/Kuszki");
	a.setApplicationVersion("1.0");

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&qtTranslator);

	QTranslator baseTranslator;
	baseTranslator.load("qtbase_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&baseTranslator);

	QTranslator appTranslator;
	appTranslator.load("ew-statistics_" + QLocale::system().name());
	a.installTranslator(&appTranslator);

	MainWindow w;
	w.show();

	return a.exec();
}
