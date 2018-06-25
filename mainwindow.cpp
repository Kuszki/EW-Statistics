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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* Parent)
: QMainWindow(Parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this); Thread.start();

	Core = new ApplicationCore();

	About = new AboutDialog(this);
	Details = new QTextBrowser(this);

	Details->setWordWrapMode(QTextOption::NoWrap);
	Details->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	Core->moveToThread(&Thread);

	PaymentWidget* Payments = new PaymentWidget(Core, this);
	RedactionWidget* Redaction = new RedactionWidget(Core, this);

	removeDetailsInfo();
	setCentralWidget(Details);
	appendModule(Payments);
	appendModule(Redaction);
	databaseConnected(false);

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Window");
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
	restoreGeometry(Settings.value("geometry").toByteArray());
	restoreState(Settings.value("state").toByteArray());
	Settings.endGroup();

	connect(Payments, &PaymentWidget::onDetailsUpdate, this, &MainWindow::updateDetailsInfo);
	connect(Payments, &PaymentWidget::onDetailsRemove, this, &MainWindow::removeDetailsInfo);

	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);
	connect(ui->actionDatabases, &QAction::triggered, this, &MainWindow::databasesActionClicked);
	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::connectActionClicked);

	connect(ui->actionDisconnect, &QAction::triggered, Core, &ApplicationCore::closeDatabases);

	connect(Core, &ApplicationCore::onDatabasesConnect, this, &MainWindow::databaseConnected);
}

MainWindow::~MainWindow(void)
{
	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Window");
	Settings.setValue("state", saveState());
	Settings.setValue("geometry", saveGeometry());
	Settings.endGroup();

	Thread.exit();
	Thread.wait();

	delete Core;
	delete ui;
}

void MainWindow::appendModule(QWidget* Module)
{
	for (const auto D : Modules) if (D->widget() == Module) return;

	QDockWidget* Dock = new QDockWidget(this);

	Dock->setObjectName(Module->objectName());
	Dock->setWindowTitle(Module->windowTitle());
	Dock->setWindowIcon(Module->windowIcon());
	Dock->setWidget(Module);

	addDockWidget(Qt::LeftDockWidgetArea, Dock);

	Modules.append(Dock);
}

void MainWindow::updateDetailsInfo(const QString& Info)
{
	if (Info.isEmpty()) Details->setVisible(false);
	else
	{
		Details->setText(Info);
		Details->setVisible(true);
	}

	Details->setMinimumWidth(Details->sizeHint().width());
}

void MainWindow::removeDetailsInfo(void)
{
	Details->setVisible(false);
}

void MainWindow::connectActionClicked(void)
{
	if (Core->getSavedDatabases().isEmpty()) { databasesActionClicked(); return; }

	ConnectDialog* Dialog = new ConnectDialog(this);

	connect(Dialog, &ConnectDialog::onAccept, this, &MainWindow::loginDataAccepted);

	connect(Dialog, &ConnectDialog::onAccept, Core, &ApplicationCore::openDatabases);
	connect(Dialog, &ConnectDialog::accepted, Dialog, &ConnectDialog::deleteLater);
	connect(Dialog, &ConnectDialog::rejected, Dialog, &ConnectDialog::deleteLater);

	connect(Core, &ApplicationCore::onDatabasesConnect, Dialog, &ConnectDialog::connected);
	connect(Core, &ApplicationCore::onErrorOccurs, Dialog, &ConnectDialog::refused);

	Dialog->open();
}

void MainWindow::databasesActionClicked(void)
{
	DatabasesDialog* Dialog = new DatabasesDialog(Core->getSavedDatabases(), this);

	connect(Dialog, &DatabasesDialog::onDialogAccepted, Core, &ApplicationCore::saveDatabasesList);

	connect(Dialog, &DatabasesDialog::accepted, Dialog, &DatabasesDialog::deleteLater);
	connect(Dialog, &DatabasesDialog::rejected, Dialog, &DatabasesDialog::deleteLater);

	Dialog->open();
}

void MainWindow::loginDataAccepted(void)
{
	ui->actionConnect->setEnabled(false);
}

void MainWindow::databaseConnected(bool OK)
{
	ui->actionConnect->setEnabled(!OK);
	ui->actionDisconnect->setEnabled(OK);
	ui->actionDatabases->setEnabled(!OK);

	for (auto Group : Modules)
	{
		Group->widget()->setEnabled(OK);
	}
}

void MainWindow::databaseDisconnected(void)
{
	databaseConnected(false);
}
