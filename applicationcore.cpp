/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
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

#include "applicationcore.hpp"

ApplicationCore::ApplicationCore(QObject* Parent)
: QObject(Parent)
{

}

ApplicationCore::~ApplicationCore(void)
{

}

QList<DBINFO> ApplicationCore::getSavedDatabases(void) const
{
	QSettings Settings("EW-Statistics"); QList<DBINFO> List;

	const int Size = Settings.beginReadArray("Databases");

	for (int i = 0; i < Size; ++i)
	{
		Settings.setArrayIndex(i);

		List.append(
		{
			Settings.value("server").toString(),
			Settings.value("path").toString(),
			Settings.value("active").toBool()
		});
	}

	Settings.endArray();

	return List;
}

void ApplicationCore::saveDatabasesList(const QList<DBINFO>& List)
{
	QSettings Settings("EW-Statistics");

	Settings.beginWriteArray("Databases");

	for (int i = 0; i < List.size(); ++i)
	{
		Settings.setArrayIndex(i);

		Settings.setValue("server", List[i].Server);
		Settings.value("path", List[i].Path);
		Settings.value("active", List[i].Enabled);
	}

	Settings.endArray();
}
