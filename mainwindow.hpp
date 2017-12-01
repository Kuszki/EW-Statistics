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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QTextBrowser>
#include <QDockWidget>
#include <QMainWindow>

#include "commonstructures.hpp"
#include "databasesdialog.hpp"
#include "applicationcore.hpp"
#include "connectdialog.hpp"
#include "paymentwidget.hpp"
#include "aboutdialog.hpp"

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{

		Q_OBJECT

	private:

		QVector<QDockWidget*> Modules;

		ApplicationCore* Core;
		AboutDialog* About;

		Ui::MainWindow* ui;
		QTextBrowser* Details;

		QThread Thread;

	public:

		explicit MainWindow(QWidget* Parent = nullptr);
		virtual ~MainWindow(void) override;

		void appendModule(QWidget* Module);

	public slots:

		void updateDetailsInfo(const QString& Info);
		void removeDetailsInfo(void);

	private slots:

		void connectActionClicked(void);
		void databasesActionClicked(void);

		void loginDataAccepted(void);

		void databaseConnected(bool OK);
		void databaseDisconnected(void);

};

#endif // MAINWINDOW_HPP
