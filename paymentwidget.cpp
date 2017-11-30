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
	static const QStringList defaultGroups =
	{
		tr("User"), tr("Month"), tr("Day")
	};

	ui->setupUi(this); QSet<QString> Used;

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	startDate = Settings.value("start", QDate::currentDate()).toDate();
	stopDate = Settings.value("stop", QDate::currentDate()).toDate();
	singlePayment = Settings.value("payment", 15.0).toDouble();
	iddleDelay = Settings.value("delay", 15).toInt();

	QStringList List = Settings.value("groups").toStringList();
	QVariantList On = Settings.value("enabled").toList();

	Settings.endGroup();

	for (int i = 0; i < List.size() && i < On.size(); ++i)
	{
		if (!Used.contains(List[i]) && defaultGroups.contains(List[i]))
		{
			allGroups.append({ List[i], On[i].toBool() }); Used.insert(List[i]);
		}
	}

	for (const auto& Key : defaultGroups) if (!Used.contains(Key))
	{
		allGroups.append({ Key, false }); Used.insert(Key);
	}


	connect(this, &PaymentWidget::onDataReloaded, this, &PaymentWidget::recordsDataLoaded);
}

PaymentWidget::~PaymentWidget(void)
{
	QStringList Groups; QVariantList On;

	for (const auto& Group : allGroups)
	{
		Groups.append(Group.first);
		On.append(Group.second);
	}

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	Settings.setValue("start", startDate);
	Settings.setValue("stop", stopDate);
	Settings.setValue("payment", singlePayment);
	Settings.setValue("delay", iddleDelay);
	Settings.setValue("groups", Groups);
	Settings.setValue("enabled", On);
	Settings.endGroup();

	delete ui;
}

QHash<QString, QVector<QPair<QDateTime, PaymentWidget::ACTION>>> PaymentWidget::loadEvents(QSqlDatabase& Db)
{
	auto isDateBetween = [] (const QDateTime& D, const QDate& A, const QDate& B)
	{
		const QDate& C = D.date(); return (A <= C) && (B >= C);
	};

	QSqlQuery selectExistingObjects(Db),
			selectExistingTexts(Db),
			selectExistingLines(Db);

	QSqlQuery selectUsers(Db), selectObjects(Db);
	QSqlQuery selectTexts(Db), selectLines(Db);

	QSet<int> Objects, Texts, Lines;
	QHash<int, QString> Users;

	QHash<int, QList<QPair<QDateTime, ACTION>>> Events;
	QHash<QString, QVector<QPair<QDateTime, ACTION>>> Finall;

	selectExistingObjects.prepare("SELECT ID FROM EW_OBIEKTY WHERE STATUS = 0");
	selectExistingTexts.prepare("SELECT ID FROM EW_TEXT WHERE STAN_ZMIANY = 0");
	selectExistingLines.prepare("SELECT ID FROM EW_POLYLINE WHERE STAN_ZMIANY = 0");

	if (selectExistingObjects.exec()) while (selectExistingObjects.next())
	{
		Objects.insert(selectExistingObjects.value(0).toInt());
	}

	if (selectExistingTexts.exec()) while (selectExistingTexts.next())
	{
		Texts.insert(selectExistingTexts.value(0).toInt());
	}

	if (selectExistingLines.exec()) while (selectExistingLines.next())
	{
		Lines.insert(selectExistingLines.value(0).toInt());
	}

	selectUsers.prepare("SELECT ID, NAME FROM EW_USERS ORDER BY ID ASCENDING");

	if (selectUsers.exec()) while (selectUsers.next())
	{
		const int ID = selectUsers.value(0).toInt();

		Users.insert(ID, selectUsers.value(1).toString());

		Events.insert(ID, QList<QPair<QDateTime, ACTION>>());
	}

	selectObjects.prepare(
		"SELECT "
			"ID, "
			"OSOU, OSOW, OSOR, "
			"DTU, DTW, DTR "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"DTU BETWEEN :from AND :to OR "
			"DTW BETWEEN :from AND :to OR "
			"DTR BETWEEN :from AND :to");

	selectObjects.bindValue(":from", startDate);
	selectObjects.bindValue(":to", stopDate.addDays(1));

	selectTexts.prepare(
		"SELECT "
			"ID, TYP, "
			"USER_CREATE, USER_MODIFY, USER_DELETE, "
			"CREATE_TS, MODIFY_TS, DELETE_TS "
		"FROM "
			"EW_TEXT "
		"WHERE "
			"CREATE_TS BETWEEN :from AND :to OR "
			"MODIFY_TS BETWEEN :from AND :to OR "
			"DELETE_TS BETWEEN :from AND :to");

	selectTexts.bindValue(":from", startDate);
	selectTexts.bindValue(":to", stopDate.addDays(1));

	selectLines.prepare(
		"SELECT "
			"ID, "
			"USER_CREATE, USER_MODIFY, USER_DELETE, "
			"CREATE_TS, MODIFY_TS, DELETE_TS "
		"FROM "
			"EW_POLYLINE "
		"WHERE "
			"CREATE_TS BETWEEN :from AND :to OR "
			"MODIFY_TS BETWEEN :from AND :to OR "
			"DELETE_TS BETWEEN :from AND :to");

	selectLines.bindValue(":from", startDate);
	selectLines.bindValue(":to", stopDate.addDays(1));

	if (selectObjects.exec()) while (selectObjects.next())
	{
		const int ID = selectObjects.value(0).toInt();

		const int uADD = selectObjects.value(1).toInt();
		const int uMOD = selectObjects.value(2).toInt();
		const int uDEL = selectObjects.value(3).toInt();

		const QVariant dADD = selectObjects.value(4);
		const QVariant dMOD = selectObjects.value(5);
		const QVariant dDEL = selectObjects.value(6);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, ADD_OBJECT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, MOD_OBJECT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, DEL_OBJECT });
			}
		}
	}

	if (selectTexts.exec()) while (selectTexts.next())
	{
		const int ID = selectTexts.value(0).toInt();
		const bool Text = selectTexts.value(1).toInt() != 4;

		const int uADD = selectTexts.value(2).toInt();
		const int uMOD = selectTexts.value(3).toInt();
		const int uDEL = selectTexts.value(4).toInt();

		const QVariant dADD = selectTexts.value(5);
		const QVariant dMOD = selectTexts.value(6);
		const QVariant dDEL = selectTexts.value(7);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, Text ? ADD_TEXT : ADD_SEGMENT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, Text ? MOD_TEXT : MOD_SEGMENT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, Text ? DEL_TEXT : DEL_SEGMENT });
			}
		}
	}

	if (selectLines.exec()) while (selectLines.next())
	{
		const int ID = selectLines.value(0).toInt();

		const int uADD = selectLines.value(1).toInt();
		const int uMOD = selectLines.value(2).toInt();
		const int uDEL = selectLines.value(3).toInt();

		const QVariant dADD = selectLines.value(4);
		const QVariant dMOD = selectLines.value(5);
		const QVariant dDEL = selectLines.value(6);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, ADD_SEGMENT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, MOD_SEGMENT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, DEL_SEGMENT });
			}
		}
	}

	for (auto i = Events.constBegin(); i != Events.constEnd(); ++i)
	{
		Finall.insert(Users[i.key()], i.value().toVector());
	}

	return Finall;
}

void PaymentWidget::appendCounters(PaymentWidget::RECORD& Record, PaymentWidget::ACTION Action)
{
	switch (Action)
	{
		case ADD_OBJECT: ++Record.ObjectsADD; break;
		case MOD_OBJECT: ++Record.ObjectsMOD; break;
		case DEL_OBJECT: ++Record.ObjectsDEL; break;

		case ADD_TEXT: ++Record.TextsADD; break;
		case MOD_TEXT: ++Record.TextsMOD; break;
		case DEL_TEXT: ++Record.TextsDEL; break;

		case ADD_SEGMENT: ++Record.SegmentsADD; break;
		case MOD_SEGMENT: ++Record.SegmentsMOD; break;
		case DEL_SEGMENT: ++Record.SegmentsDEL; break;
	}
}

void PaymentWidget::appendCounters(PaymentWidget::STATPART& Record, const PaymentWidget::RECORD& Action)
{
	const unsigned Time = Record.Start.secsTo(Record.Stop);

	Record.ObjectsADD += Action.ObjectsADD;
	Record.ObjectsMOD += Action.ObjectsMOD;
	Record.ObjectsDEL += Action.ObjectsDEL;

	Record.TextsADD += Action.TextsADD;
	Record.TextsMOD += Action.TextsMOD;
	Record.TextsDEL += Action.TextsDEL;

	Record.SegmentsADD += Action.SegmentsADD;
	Record.SegmentsMOD += Action.SegmentsMOD;
	Record.SegmentsDEL += Action.SegmentsDEL;

	Record.Start = qMin(Record.Start, Action.Start);
	Record.Stop = qMax(Record.Stop, Action.Stop);

	Record.Payment += (Time / 3600.0) * singlePayment;
	Record.Time += Time;
}

QString PaymentWidget::formatInfo(const PaymentWidget::STATPART& Record)
{
	return tr(
		"<h1>User: %1</h1>"
		"<table>"
		"<th>"
		"<tr>");
}

void PaymentWidget::refreshButtonClicked(void)
{
	setEnabled(false); QtConcurrent::run(this, &PaymentWidget::refreshData);
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

void PaymentWidget::recordsDataLoaded(const QList<PaymentWidget::RECORD>& Data)
{
	static const QStringList Headers =
	{
		tr("User"), tr("Day"), tr("Month"), tr("From"), tr("To"), tr("Hours"), tr("Payment")
	};

	RecordModel* Model = new RecordModel(Headers, this);
	QStringList enabledGroups;

	for (int i = 0; i < Data.size(); ++i)
	{
		const RECORD& Row = Data[i];
		QHash<int, QVariant> Record;

		const unsigned Time = Row.Start.secsTo(Row.Stop);

		const QString Formated = QString("%1:%2:%3")
							.arg(Time / 3600, 2, 10, QChar('0'))
							.arg((Time / 60) % 60, 2, 10, QChar('0'))
							.arg(Time % 60, 2, 10, QChar('0'));

		const double Payment = (Time / 3600.0) * singlePayment;

		Record[0] = Row.User;
		Record[1] = Row.Start.date();
		Record[2] = Row.Start.date().toString("MMMM yyyy");
		Record[3] = Row.Start;
		Record[4] = Row.Stop;
		Record[5] = Formated;
		Record[6] = QString::number(Payment, 'f', 2);

		Model->addItem(i, Record);
	}

	auto oldModel = ui->treeView->model();
	auto oldSelect = ui->treeView->selectionModel();

	ui->treeView->setModel(Model);

	for (const auto& Group : allGroups) if (Group.second)
	{
		enabledGroups.append(Group.first);
	}

	Model->groupByStr(enabledGroups);

	delete oldModel;
	delete oldSelect;

	setEnabled(true);

	Records = Data;
}

void PaymentWidget::selectionChanged(void)
{
	const auto Selected = ui->treeView->selectionModel()->selectedRows();
	const auto Model = qobject_cast<RecordModel*>(ui->treeView->model());

	const auto List = Model->getUids(Selected);

	QHash<QString, STATPART> Stats;
	QStringList Infos;

	for (const auto& i : List)
	{
		const auto& Rec = Records[i];
		appendCounters(Stats[Rec.User], Rec);
	}

	for (auto i = Stats.constBegin(); i != Stats.constEnd(); ++i)
	{

	}
}

void PaymentWidget::refreshData(void)
{
	QMutexLocker Locker(&Synchronizer);

	QHash<QString, QVector<QPair<QDateTime, ACTION>>> Data;
	auto Databases = Core->getDatabases();
	QList<RECORD> Records;

	const int Secs = iddleDelay * 60;

	for (int i = 0; i < Databases.size(); ++i)
	{
		const auto Part = loadEvents(Databases[i]);

		for (auto j = Part.constBegin(); j != Part.constEnd(); ++j)
		{
			if (Data.contains(j.key()))
			{
				Data[j.key()].append(j.value());
			}
			else Data.insert(j.key(), j.value());
		}
	}

	QtConcurrent::blockingMap(Data,
	[] (QVector<QPair<QDateTime, ACTION>>& List) -> void
	{
		std::stable_sort(List.begin(), List.end());
	});

	for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
	{		
		const auto& User = i.key();
		const auto& List = i.value();

		if (List.size() < 2) continue;

		RECORD Record = { User, List.first().first, List.first().first };

		for (const auto& Rec : List)
		{
			const int Diff = Record.Stop.secsTo(Rec.first);

			if (Diff < Secs)
			{
				appendCounters(Record, Rec.second);
				Record.Stop = Rec.first;
			}
			else
			{
				if (Record.Start.secsTo(Record.Stop))
				{
					Records.append(Record);
				}

				Record = { User, Rec.first, Rec.first };
			}
		}

		if (Record.Start.secsTo(Record.Stop)) Records.append(Record);
	}

	emit onDataReloaded(Records);
}

void PaymentWidget::setParameters(const QDate& Start, const QDate& Stop, const QList<QPair<QString, bool>>& Groups, double Payment, bool Refresh, int Delay)
{
	startDate = Start;
	stopDate = Stop;

	allGroups = Groups;

	singlePayment = Payment;
	iddleDelay = Delay;

	if (Refresh)
	{
		refreshButtonClicked();
	}
}