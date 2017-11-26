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
#ifndef PAYMENTWIDGET_HPP
#define PAYMENTWIDGET_HPP

#include <QWidget>

#include "commonstructures.hpp"
#include "applicationcore.hpp"
#include "paymentdialog.hpp"

namespace Ui
{
	class PaymentWidget;
}

class PaymentWidget : public QWidget
{

		Q_OBJECT

	private:

		ApplicationCore* Core;

		Ui::PaymentWidget* ui;

		QDate startDate;
		QDate stopDate;

		QMap<QString, bool> allGroups;

		double singlePayment;
		int iddleDelay;

	public:

		explicit PaymentWidget(ApplicationCore* App, QWidget* Parent = nullptr);
		virtual ~PaymentWidget(void) override;

	private slots:

		void refreshButtonClicked(void);
		void optionsButtonClicked(void);

		void searchTextChanged(const QString& Text);

	public slots:

		void refreshData(const QDate& From, const QDate& To);

		void setParameters(const QDate& Start, const QDate& Stop,
					    const QMap<QString, bool>& Groups,
					    double Payment, bool Refresh, int Delay);

	signals:

		void onDetailsUpdate(const QString&);

};

#endif // PAYMENTWIDGET_HPP
