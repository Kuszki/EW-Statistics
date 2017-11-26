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

#ifndef PAYMENTDIALOG_HPP
#define PAYMENTDIALOG_HPP

#include <QStandardItemModel>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QStandardItem>
#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class PaymentDialog;
}

class PaymentDialog : public QDialog
{
		Q_OBJECT

	private:

		Ui::PaymentDialog* ui;

	public:

		explicit PaymentDialog(QWidget* Parent = nullptr);
		virtual ~PaymentDialog(void) override;

		QMap<QString, bool> getGroups(void) const;

	public slots:

		virtual void accept(void) override;

		void setParameters(const QDate& Start, const QDate& Stop,
					    const QMap<QString, bool>& Groups,
					    double Payment, int Delay);

	private slots:

		void dialogButtonClicked(QAbstractButton* Button);

		void startDateChanged(const QDate& Date);
		void stopDateChanged(const QDate& Date);

		void iddleSpinChanged(int Value);

	signals:

		void onDialogAccepted(const QDate&, const QDate&,
						  const QMap<QString, bool>&,
						  double, bool, int);

};

#endif // PAYMENTDIALOG_HPP
