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

#ifndef DATABASESDIALOG_HPP
#define DATABASESDIALOG_HPP

#include <QStandardItemModel>
#include <QSettings>
#include <QDialog>

#include "commonstructures.hpp"

namespace Ui
{
	class DatabasesDialog;
}

class DatabasesDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::DatabasesDialog* ui;

		QStandardItemModel* Model;

	public:

		explicit DatabasesDialog(const QList<DBINFO>& Databases,
							QWidget* Parent = nullptr);
		virtual ~DatabasesDialog(void) override;

		QList<DBINFO> getDatabases(void) const;

	public slots:

		virtual void accept(void) override;

		void setDatabases(const QList<DBINFO>& Databases);

	private slots:

		void addButtonClicked(void);
		void removeButtonClicked(void);

	signals:

		void onDialogAccepted(const QList<DBINFO>&);

};

#endif // DATABASESDIALOG_HPP
