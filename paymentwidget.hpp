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

#include <QMutexLocker>
#include <QWidget>
#include <QDebug>

#include <QtConcurrent>

#include "commonstructures.hpp"
#include "applicationcore.hpp"
#include "paymentdialog.hpp"
#include "recordmodel.hpp"

namespace Ui
{
	class PaymentWidget;
}

class PaymentWidget : public QWidget
{

		Q_OBJECT

	public: enum ACTION
	{
		ADD_OBJECT,
		MOD_OBJECT,
		DEL_OBJECT,

		ADD_SEGMENT,
		MOD_SEGMENT,
		DEL_SEGMENT,

		ADD_TEXT,
		MOD_TEXT,
		DEL_TEXT
	};

	public: struct RECORD
	{
		QString User;

		QDateTime Start;
		QDateTime Stop;

		int ObjectsADD = 0;
		int ObjectsMOD = 0;
		int ObjectsDEL = 0;

		int TextsADD = 0;
		int TextsMOD = 0;
		int TextsDEL = 0;

		int SegmentsADD = 0;
		int SegmentsMOD = 0;
		int SegmentsDEL = 0;
	};

	public: struct STATPART
	{
		QString User;

		QDateTime Start;
		QDateTime Stop;

		int ObjectsADD = 0;
		int ObjectsMOD = 0;
		int ObjectsDEL = 0;

		int TextsADD = 0;
		int TextsMOD = 0;
		int TextsDEL = 0;

		int SegmentsADD = 0;
		int SegmentsMOD = 0;
		int SegmentsDEL = 0;

		double Payment = 0.0;
		unsigned Time = 0;
	};

	public: struct FILTER
	{
		QString User;

		int Month = 0;
		int Day = 0;
	};

	private:

		mutable QMutex Synchronizer;

		ApplicationCore* Core;

		Ui::PaymentWidget* ui;

		QDate startDate;
		QDate stopDate;

		QList<QPair<QString, bool>> allGroups;

		double singlePayment;
		int iddleDelay;

		QList<RECORD> Records;

	public:

		explicit PaymentWidget(ApplicationCore* App, QWidget* Parent = nullptr);
		virtual ~PaymentWidget(void) override;

	private:

		QHash<QString, QVector<QPair<QDateTime, ACTION>>> loadEvents(QSqlDatabase& Db);

		static void appendCounters(RECORD& Record, ACTION Action);
		static void appendCounters(STATPART& Record, const RECORD& Action);

		static QString formatInfo(const STATPART& Record);

	private slots:

		void refreshButtonClicked(void);
		void optionsButtonClicked(void);

		void searchTextChanged(const QString& Text);
		void recordsDataLoaded(const QList<RECORD>& Data);

		void selectionChanged(void);

		void refreshData(void);

	public slots:

		void setParameters(const QDate& Start, const QDate& Stop,
					    const QList<QPair<QString, bool>>& Groups,
					    double Payment, bool Refresh, int Delay);

	signals:

		void onDetailsUpdate(const QString&);

		void onDataReloaded(const QList<RECORD>&);

};

#endif // PAYMENTWIDGET_HPP
