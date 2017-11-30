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
: QObject(Parent), Synchronizer(QMutex::Recursive)
{
	Infolist = getSavedDatabases();
}

ApplicationCore::~ApplicationCore(void) {}

QList<DBINFO> ApplicationCore::getSavedDatabases(void) const
{
	QMutexLocker Locker(&Synchronizer);
	QSettings Settings("EW-Statistics");
	QList<DBINFO> List;

	const int Size = Settings.beginReadArray("Databases");

	for (int i = 0; i < Size; ++i)
	{
		Settings.setArrayIndex(i);

		List.append(
		{
			Settings.value("name").toString(),
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
	QMutexLocker Locker(&Synchronizer);
	QSettings Settings("EW-Statistics");

	Settings.beginWriteArray("Databases");

	for (int i = 0; i < List.size(); ++i)
	{
		Settings.setArrayIndex(i);

		Settings.setValue("name", List[i].Name);
		Settings.setValue("server", List[i].Server);
		Settings.setValue("path", List[i].Path);
		Settings.setValue("active", List[i].Enabled);
	}

	Settings.endArray(); Infolist = List;
}

QVector<QSqlDatabase> ApplicationCore::getDatabases(void) const
{
	QMutexLocker Locker(&Synchronizer); return Databases;
}

void ApplicationCore::openDatabases(const QString& User, const QString& Password)
{
	QMutexLocker Locker(&Synchronizer); QStringList Errors;

	if (!Databases.isEmpty()) closeDatabases();

	if (!Infolist.isEmpty()) Databases.reserve(Infolist.size());

	for (const auto& Db : Infolist) if (Db.Enabled)
	{
		Databases.append(QSqlDatabase::addDatabase("QIBASE", QString::number(qHash(Db))));

		QSqlDatabase& Database = Databases.last();

		Database.setUserName(User);
		Database.setPassword(Password);

		Database.setHostName(Db.Server);
		Database.setDatabaseName(Db.Path);

		if (!Database.open())
		{
			Errors.append(Db.Server + ":" + Db.Path + " - " +
					    Database.lastError().text());

			Databases.removeLast();
		}
	}

	if (Errors.size())
	{
		emit onErrorOccurs(Errors.join('\n'));
	}

	emit onDatabasesConnect(Databases.size());
}

void ApplicationCore::closeDatabases(void)
{
	QMutexLocker Locker(&Synchronizer);
	QStringList Connections;

	for (auto& Database : Databases)
	{
		const QString Name = Database.connectionName();

		Database.close();
		Connections.append(Name);
	}

	Databases.clear();

	for (const auto& Name : Connections)
	{
		QSqlDatabase::removeDatabase(Name);
	}

	emit onDatabasesConnect(false);
}
