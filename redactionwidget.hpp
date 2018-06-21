/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
 *  Copyright (C) 2018  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
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

#ifndef REDACTIONWIDGET_HPP
#define REDACTIONWIDGET_HPP

#include <QWidget>

#include "commonstructures.hpp"
#include "applicationcore.hpp"

namespace Ui
{
	class RedactionWidget;
}

class RedactionWidget : public QWidget
{

		Q_OBJECT

	public: struct FIELD
	{
		QString Name;

		QHash<int, QString> LongDict;
		QHash<int, QString> ShortDict;
	};

	private:

		mutable QMutex Synchronizer;

		ApplicationCore* Core;
		Ui::RedactionWidget* ui;

	public:

		explicit RedactionWidget(QWidget* Parent = nullptr);
		virtual ~RedactionWidget(void) override;

	protected:

		QHash<QString, QList<const FIELD*>> loadTables(QSqlDatabase& Db, const QHash<int, FIELD>& Fields);
		QHash<int, QVector<double>> loadLayers(QSqlDatabase& Db);
		QHash<int, FIELD> loadfields(QSqlDatabase& Db);

};

#endif // REDACTIONWIDGET_HPP
