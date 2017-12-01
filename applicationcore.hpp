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

#ifndef APPLICATIONCORE_HPP
#define APPLICATIONCORE_HPP

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QObject>

#include "commonstructures.hpp"

class ApplicationCore : public QObject
{

		Q_OBJECT

	private:

		mutable QMutex Synchronizer;

		QVector<QSqlDatabase> Databases;
		QList<DBINFO> Infolist;

	public:

		explicit ApplicationCore(QObject* Parent = nullptr);
		virtual ~ApplicationCore(void) override;

		QList<DBINFO> getSavedDatabases(void) const;
		void saveDatabasesList(const QList<DBINFO>& List);

		QVector<QSqlDatabase> getDatabases(void) const;

	public slots:

		void openDatabases(const QString& User,
					    const QString& Password);
		void closeDatabases(void);

	signals:

		void onDatabasesConnect(bool);
		void onDatabasesDisconnect(void);

		void onErrorOccurs(const QString&);

};

#endif // APPLICATIONCORE_HPP
