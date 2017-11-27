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

#include "paymentwidget.hpp"
#include "ui_paymentwidget.h"

PaymentWidget::PaymentWidget(ApplicationCore* App, QWidget* Parent)
: QWidget(Parent), Core(App), ui(new Ui::PaymentWidget)
{
	ui->setupUi(this);

	static const QMap<QString, bool> defaultGroups =
	{
		{ tr("User"), false },
		{ tr("Database"), false },
		{ tr("Month"), false },
		{ tr("Day"), false }
	};

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	startDate = Settings.value("start", QDate::currentDate()).toDate();
	stopDate = Settings.value("stop", QDate::currentDate()).toDate();
	singlePayment = Settings.value("payment", 15.0).toDouble();

	QStringList List = Settings.value("groups").toStringList();
	QVariantList On = Settings.value("enabled").toList();

	Settings.endGroup();

	for (int i = 0; i < List.size() && i < On.size(); ++i)
	{
		if (!allGroups.contains(List[i]) && defaultGroups.contains(List[i]))
		{
			allGroups.insert(List[i], On[i].toBool());
		}
	}

	for (const auto& Key : defaultGroups.keys())
	{
		if (!allGroups.contains(Key))
		{
			allGroups.insert(Key, false);
		}
	}
}

PaymentWidget::~PaymentWidget(void)
{
	QStringList Groups = allGroups.keys();
	QVariantList On;

	for (const auto& S : allGroups) On.append(S);

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	Settings.setValue("start", startDate);
	Settings.setValue("stop", stopDate);
	Settings.setValue("payment", singlePayment);
	Settings.setValue("groups", Groups);
	Settings.setValue("enabled", On);
	Settings.endGroup();

	delete ui;
}

QList<PaymentWidget::RECORD> PaymentWidget::filterRecords(const QList<PaymentWidget::RECORD>& List, const FILTER& Filter) const
{
	QList<PaymentWidget::RECORD> Filtered;

	for (const auto R : List)
	{
		bool OK = true;

		OK = OK && (Filter.User.isEmpty() || Filter.User == R.User);
		OK = OK && (Filter.Base.isEmpty() || Filter.Base == R.Database);

		OK = OK && (Filter.Month == 0 || Filter.Month == R.Time.date().month());
		OK = OK && (Filter.Day == 0 || Filter.Day == R.Time.date().day());

		if (OK) Filtered.append(R);
	}

	return Filtered;
}

void PaymentWidget::refreshButtonClicked(void)
{
	refreshData(startDate, stopDate);
}

void PaymentWidget::optionsButtonClicked(void)
{
	PaymentDialog* Dialog = new PaymentDialog(this);

	connect(Dialog, &PaymentDialog::accepted, Dialog, &PaymentDialog::deleteLater);
	connect(Dialog, &PaymentDialog::rejected, Dialog, &PaymentDialog::deleteLater);

	connect(Dialog, &PaymentDialog::onDialogAccepted, this, &PaymentWidget::setParameters);

	Dialog->setParameters(startDate, stopDate, allGroups, singlePayment, iddleDelay);
	Dialog->open();
}

void PaymentWidget::searchTextChanged(const QString& Text)
{
	// TODO
}

void PaymentWidget::refreshData(const QDate& From, const QDate& To)
{
	// TODO
}

void PaymentWidget::setParameters(const QDate& Start, const QDate& Stop, const QMap<QString, bool>& Groups, double Payment, bool Refresh, int Delay)
{
	startDate = Start;
	stopDate = Stop;

	allGroups = Groups;

	singlePayment = Payment;
	iddleDelay = Delay;

	if (Refresh) refreshData(Start, Stop);
}
