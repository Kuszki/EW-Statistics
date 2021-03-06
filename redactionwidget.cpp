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

#include "redactionwidget.hpp"
#include "ui_redactionwidget.h"

RedactionWidget::RedactionWidget(ApplicationCore* App, QWidget* Parent)
: QWidget(Parent), Core(App), ui(new Ui::RedactionWidget)
{
	ui->setupUi(this); QSettings Settings("EW-Statistics");

	Settings.beginGroup("Redaction");
	smbScales << Settings.value("smb500", 0.5).toDouble();
	smbScales << Settings.value("smb1000", 1.0).toDouble();
	smbScales << Settings.value("smb2000", 2.0).toDouble();
	smbScales << Settings.value("smb5000", 5.0).toDouble();
	smbExclude = Settings.value("exclude").toStringList();
	mapScale = SCALE(Settings.value("scale", 0).toInt());
	smbCompute = Settings.value("smbcmp", true).toBool();
	tolAbs = Settings.value("abstol", 0.0).toDouble();
	tolPrc = Settings.value("prctol", 0.0).toDouble();
	maxIters = Settings.value("iters", 1).toInt();
	Settings.endGroup();

	ui->scaleCombo->setCurrentIndex(mapScale);
	ui->saveButton->setEnabled(false);
	ui->progressBar->setVisible(false);

	connect(this, &RedactionWidget::onDataReloaded, this, &RedactionWidget::dataReloaded);
	connect(this, &RedactionWidget::onProgressRename, ui->progressBar, &QProgressBar::setFormat);
	connect(this, &RedactionWidget::onProgressStart, ui->progressBar, &QProgressBar::setRange);
	connect(this, &RedactionWidget::onProgresUpdate, ui->progressBar, &QProgressBar::setValue);
}

RedactionWidget::~RedactionWidget(void)
{
	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Redaction");
	Settings.setValue("scale", mapScale);
	Settings.setValue("exclude", smbExclude);
	Settings.setValue("smb500", smbScales.value(0, 0.5));
	Settings.setValue("smb1000", smbScales.value(1, 1.0));
	Settings.setValue("smb2000", smbScales.value(2, 2.0));
	Settings.setValue("smb5000", smbScales.value(3, 5.0));
	Settings.setValue("smbcmp", smbCompute);
	Settings.setValue("abstol", tolAbs);
	Settings.setValue("prctol", tolPrc);
	Settings.setValue("iters", maxIters);
	Settings.endGroup();

	delete ui;
}

void RedactionWidget::setParameters(const QVector<double>& Scales, const QStringList& Exclude, bool computeSymbols, double absValue, double prcValue, int itCount)
{
	QMutexLocker Locker(&Synchronizer);

	smbScales = Scales;
	smbExclude = Exclude;
	smbCompute = computeSymbols;
	tolAbs = absValue;
	tolPrc = prcValue;
	maxIters = itCount;
}

QList<RedactionWidget::TABLE> RedactionWidget::loadTables(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QList<TABLE> List;

	Query.prepare(
		"SELECT "
			"O.KOD, O.OPIS, O.DANE_DOD, O.OPCJE, "
			"F.NAZWA, F.TYTUL, F.TYP, "
			"D.WARTOSC, D.OPIS, "
			"BIN_AND(O.OPCJE, 266),"
			"S.WYPELNIENIE, D.SOPIS "
		"FROM "
			"EW_OB_OPISY O "
		"INNER JOIN "
			"EW_OB_DDSTR F "
		"ON "
			"O.KOD = F.KOD "
		"INNER JOIN "
			"EW_OB_DDSTR S "
		"ON "
			"F.KOD = S.KOD "
		"LEFT JOIN "
			"EW_OB_DDSL D "
		"ON "
			"S.UID = D.UIDP OR S.UIDSL = D.UIDP "
		"WHERE "
			"S.NAZWA = F.NAZWA "
		"ORDER BY "
			"O.KOD, F.NAZWA, D.OPIS");

	if (Query.exec()) while (Query.next())
	{
		const QString Field = Query.value(4).toString();
		const QString Table = Query.value(0).toString();
		const bool Dict = !Query.value(7).isNull();

		if (!hasItemByField(List, Table, &TABLE::Name)) List.append(
		{
			Table,
			Query.value(1).toString(),
			Query.value(2).toString(),
			bool(Query.value(3).toInt() & 0x100),
			Query.value(9).toInt()
		});

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		if (!hasItemByField(Tabref.Fields, Field, &FIELD::Name)) Tabref.Fields.append(
		{
			TYPE(Query.value(6).toInt()),
			Field,
			Query.value(5).toString(),
			Query.value(10).toInt() == 2
		});

		auto& Fieldref = getItemByField(Tabref.Fields, Field, &FIELD::Name);

		if (Dict) Fieldref.Dict.insert(Query.value(7),
		{
			Query.value(8).toString(),
			Query.value(11).toString()
		});
	}

	Query.prepare("SELECT KOD, '${u.' || NAZWA || '}', DEFINICJA FROM EW_OB_ZMIENNE");

	if (Query.exec()) while (Query.next())
	{
		const QString Table = Query.value(0).toString();

		if (!hasItemByField(List, Table, &TABLE::Name)) continue;

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		Tabref.Labels.insert(Query.value(1).toString(), Query.value(2).toString());
	}

	return List;
}

QHash<int, QVector<double>> RedactionWidget::loadLayers(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QHash<int, QVector<double>> List;

	Query.prepare("SELECT ID, WYSOKOSC, MNOZNIK_M_S500, MNOZNIK_M_S1000, MNOZNIK_M_S2000, MNOZNIK_M_S5000 FROM EW_WARSTWA_TEXTOWA");

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0).toInt(), QVector<double>()
											 << Query.value(1).toDouble()
											 << Query.value(2).toDouble()
											 << Query.value(3).toDouble()
											 << Query.value(4).toDouble()
											 << Query.value(5).toDouble());

	return List;
}

QHash<int, QList<QVariant>> RedactionWidget::loadData(const RedactionWidget::TABLE& Table, QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true);
	QHash<int, QList<QVariant>> List; QStringList Attribs;

	for (const auto& Field : Table.Fields) Attribs.append(QString("EW_DATA.%1").arg(Field.Name));

	const auto append = [&] (QSqlQuery& Query, int Index) -> void
	{
		static QList<QVariant> Values; Values.reserve(Table.Fields.size());

		for (int j = 1; j <= Table.Fields.size(); ++j) Values.append(Query.value(j));

		if (!Values.isEmpty()) List.insert(Index, Values); Values.clear();
	};

	QString Exec = QString(
		"SELECT "
			"EW_OBIEKTY.UID, %1 "
		"FROM "
			"EW_OBIEKTY "
		"LEFT JOIN "
			"%2 EW_DATA "
		"ON "
			"EW_OBIEKTY.UID = EW_DATA.UIDO "
		"WHERE "
			"EW_OBIEKTY.KOD = '%3' AND "
			"EW_OBIEKTY.STATUS = 0")
				.arg(Attribs.join(", "))
				.arg(Table.Data)
				.arg(Table.Name);

	if (!Query.prepare(Exec)) return QHash<int, QList<QVariant>>();
	else if (Query.exec()) while (Query.next())
	{
		const int Index = Query.value(0).toInt(); append(Query, Index);
	}

	return List;
}

QHash<int, QList<RedactionWidget::LABEL>> RedactionWidget::loadLabels(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QHash<int, QList<LABEL>> List;

	Query.prepare(
		"SELECT "
			"O.UID, T.UID, T.POS_X, T.POS_Y, T.TEXT, "
			"T.ID_WARSTWY, T.JUSTYFIKACJA, T.KAT "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 6");

	if (Query.exec()) while (Query.next()) List[Query.value(0).toInt()].append(
	{
		Db.databaseName(),
		Query.value(1).toInt(),
		{
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		},
		Query.value(4).toString(),
		Query.value(5).toInt(),
		Query.value(6).toInt() & 0b1111,
		qRadiansToDegrees(Query.value(7).toDouble())
	});

	return List;
}

QList<QPointF> RedactionWidget::loadSymbols(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QList<QPointF> List;

	Query.prepare(
		"SELECT "
			"T.TEXT, T.POS_X, T.POS_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4");

	if (Query.exec()) while (Query.next()) if (!smbExclude.contains(Query.value(0).toString())) List.append(
	{
		Query.value(1).toDouble(),
		Query.value(2).toDouble()
	});

	return List;
}

QHash<QString, QList<RedactionWidget::SYMBOL>> RedactionWidget::loadSmbLabels(void)
{
	QHash<QString, QList<SYMBOL>> List;

	auto Databases = Core->getDatabases();

	for (int i = 0; i < Databases.size(); ++i)
	{
		QSqlQuery Query(Databases[i]); Query.setForwardOnly(true);
		QHash<int, SYMBOL> Current;

		Query.prepare(
			"SELECT "
				"O.UID, P.P0_X, P.P0_Y, "
				"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
				"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y) "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 0) "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"(P.ID = E.IDE AND P.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 2");

		if (Query.exec()) while (Query.next()) Current.insert(Query.value(0).toInt(),
		{
			QPointF(Query.value(1).toDouble(), Query.value(2).toDouble())
		});

		Query.prepare(
			"SELECT "
				"O.UID, T.UID "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 0) "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"(T.ID = E.IDE AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 2 AND T.TYP = 6");

		if (Query.exec()) while (Query.next()) Current[Query.value(0).toInt()].Labels.insert(Query.value(1).toInt());

		Query.prepare(
			"SELECT "
				"O.UID, T.UID "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 1) "
			"INNER JOIN "
				"EW_OBIEKTY P "
			"ON "
				"(P.ID = E.IDE AND P.STATUS = 0) "
			"INNER JOIN "
				"EW_OB_ELEMENTY K "
			"ON "
				"(K.UIDO = P.UID AND K.TYP = 0) "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"(T.ID = K.IDE AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 2 AND T.TYP = 6");

		if (Query.exec()) while (Query.next()) Current[Query.value(0).toInt()].Labels.insert(Query.value(1).toInt());

		List.insert(Databases[i].databaseName(), Current.values());
	}

	return List;
}

QHash<QString, QList<RedactionWidget::LINE>> RedactionWidget::loadLinLabels(void)
{
	QHash<QString, QList<LINE>> List;

	auto Databases = Core->getDatabases();

	for (int i = 0; i < Databases.size(); ++i)
	{
		QSqlQuery Query(Databases[i]); Query.setForwardOnly(true);
		QHash<int, LINE> Current;

		Query.prepare(
			"SELECT "
				"O.UID, T.POS_X, T.POS_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 0) "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"(T.ID = E.IDE AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 4 AND T.TYP = 4");

		if (Query.exec()) while (Query.next()) Current[Query.value(0).toInt()].Lines.append(
		{
			{ Query.value(1).toDouble(), Query.value(2).toDouble() },
			{ Query.value(3).toDouble(), Query.value(4).toDouble() }
		});

		Query.prepare(
			"SELECT "
				"O.UID, T.UID "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 0) "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"(T.ID = E.IDE AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 4 AND T.TYP = 6");

		if (Query.exec()) while (Query.next()) Current[Query.value(0).toInt()].Labels.insert(Query.value(1).toInt());

		Query.prepare(
			"SELECT "
				"O.UID, T.UID "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"(O.UID = E.UIDO AND E.TYP = 1) "
			"INNER JOIN "
				"EW_OBIEKTY P "
			"ON "
				"(P.ID = E.IDE AND P.STATUS = 0) "
			"INNER JOIN "
				"EW_OB_ELEMENTY K "
			"ON "
				"(K.UIDO = P.UID AND K.TYP = 0) "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"(T.ID = K.IDE AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND O.RODZAJ = 4 AND T.TYP = 6");

		if (Query.exec()) while (Query.next()) Current[Query.value(0).toInt()].Labels.insert(Query.value(1).toInt());

		List.insert(Databases[i].databaseName(), Current.values());
	}

	return List;
}

void RedactionWidget::parseLabels(QHash<int, QList<RedactionWidget::LABEL>>& Labels, const RedactionWidget::TABLE& Tab, const QHash<int, QList<QVariant>>& Values) const
{
	const auto parseLabel = [&Tab] (LABEL& Label, const QList<QVariant>& Values) -> void
	{
		QRegExp Exp("\\$\\{e\\.(\\w+)((?:,[^\\}]+)*)\\}"); int i(0); QStringList Str;

		if (Tab.Labels.contains(Label.Text)) Label.Text = Tab.Labels[Label.Text];

		while ((i = Exp.indexIn(Label.Text, i)) != -1)
		{
			const QStringList Cap = Exp.capturedTexts();

			const QString Var = Cap.value(1).remove("_SK");
			const bool SK = Cap.value(1).endsWith("_SK");

			const QStringList Args = Cap.value(2).split(',', QString::SkipEmptyParts);

			QStringList Not; for (const auto& Arg : Args) if (Arg.startsWith('N'))
			{
				Not.append(Arg.mid(1));
			}

			for (int j = 0; j < Tab.Fields.size(); ++j) if (Var == Tab.Fields[j].Name)
			{
				const auto Val = getDataFromDict(Values[j], Tab.Fields[j].Dict, Tab.Fields[j].Type, SK);

				if (isVariantEmpty(Val)) Str.append(QString());
				else if (Not.contains(Val.toString())) Str.append(QString());
				else if (Args.contains("Z2N")) Str.append(QString::number(Val.toDouble(), 'f', 2));
				else if (Args.contains("Z2")) Str.append(QString::number(Val.toDouble(), 'f', 2));
				else if (Args.contains("Z1N")) Str.append(QString::number(Val.toDouble(), 'f', 1));
				else if (Args.contains("Z1")) Str.append(QString::number(Val.toDouble(), 'f', 1));
				else Str.append(Val.toString());
			}

			i += Exp.matchedLength();
		}

		if (!Str.isEmpty()) Label.Text.replace(Exp, "$");

		while (Str.size() && (i = Label.Text.indexOf('$')) != -1)
		{
			Label.Text.replace(i, 1, Str.takeFirst());
		}

		if (!Label.Text.isEmpty()) Label.Text = Label.Text.trimmed();
	};

	auto Keys = Labels.keys().toSet() & Values.keys().toSet();

	QtConcurrent::blockingMap(Keys, [&Labels, &Values, parseLabel] (int UID) -> void
	{
		for (auto& Label : Labels[UID]) parseLabel(Label, Values[UID]);
	});
}

void RedactionWidget::surfLabels(QHash<int, QList<RedactionWidget::LABEL>>& Labels, SCALE Sc, const QHash<int, QVector<double>>& Layers) const
{
	const auto maxSize = [] (const QString& Str) -> int
	{
		int Max = 0; auto List = Str.split('|', QString::SkipEmptyParts);

		for (const auto& L : List) Max = qMax(Max, L.size()); return Max;
	};

	const auto surfLabel = [Sc, maxSize] (LABEL& Label, const QVector<double>& Layer) -> void
	{
		const double dy = (2.0/3.0) * Layer[0] * Layer[int(Sc) + 1];
		const double dx = Layer[0] * Layer[int(Sc) + 1];

		const int corr = Label.Text.endsWith('|') ? 1 : 0;
		const int lines = Label.Text.count('|') + 1 - corr;

		const double wY = ((lines > 1 ? maxSize(Label.Text) : Label.Text.size() - corr) * dy) / 2.0;
		const double wX = ((lines > 1 ? lines * (4.0/3.0) : 1) * dx) / 2.0;

		const QPointF Org = Label.Point; if (Label.Org == QPointF()) Label.Org = Org;

		const QVector<QPointF> Move =
		{
			{ wX, wY }, { wX, 0.0 }, { wX, -wY },
			{ 0, wY }, { 0.0, 0.0 }, { 0, -wY },
			{ -wX, wY }, { -wX, 0 }, { -wX, -wY }
		};

		Label.Point += Move.value(Label.Just - 1);

		Label.Pol.append({ Label.Point.x() - wX, Label.Point.y() + wY });
		Label.Pol.append({ Label.Point.x() + wX, Label.Point.y() + wY });
		Label.Pol.append({ Label.Point.x() + wX, Label.Point.y() - wY });
		Label.Pol.append({ Label.Point.x() - wX, Label.Point.y() - wY });

		Label.Point = QPointF(); for (auto& P : Label.Pol)
		{
			QLineF Line(Org, P);
			Line.setAngle(Line.angle() - Label.Ang);
			P = Line.p2();

			Label.Point += P / 4.0;
		}

		Label.Sur = 4.0 * wY * wX;
		Label.Rad = qMax(wY, wX);
		Label.Just = 5;
	};

	QtConcurrent::blockingMap(Labels, [&Layers, surfLabel] (QList<LABEL>& List) -> void
	{
		for (auto& Label : List) if (Layers.contains(Label.Idw))
		{
			surfLabel(Label, Layers[Label.Idw]);
		}
	});
}

QSet<QStringList> RedactionWidget::getColisions(QList<LABEL>& Labels) const
{
	if (Labels.isEmpty()) return QSet<QStringList>();

	QMutex Locker; QSet<QStringList> Res; int Step(0);

	emit onProgressRename(tr("Computing labels colisions (%p%)"));
	emit onProgressStart(0, Labels.size()); emit onProgresUpdate(0);

	QtConcurrent::blockingMap(Labels, [this, &Labels, &Res, &Locker, &Step] (LABEL& Lab) -> void
	{
		for (const auto& Oth : Labels) if (Lab.Uid != Oth.Uid || Lab.Dbn != Oth.Dbn)
		{
			if ((Lab.Rad + Oth.Rad) < QLineF(Lab.Point, Oth.Point).length()) continue;

			const QPolygonF Int = Lab.Pol.intersected(Oth.Pol);

			if (!Int.isEmpty())
			{
				const double Surf = getSurface(Int); bool OK(true);

				if (tolPrc != 0.0 && (tolPrc * Lab.Sur > Surf)) OK = false;
				if (tolAbs != 0.0 && (tolAbs > Surf)) OK = false;

				if (OK)
				{
					int S = Int.size(); double x(0.0), y(0.0);
					const double rat = Surf / qMin(Lab.Sur, Oth.Sur);

					if (Int.isClosed()) S -= 1;

					for (int i = 0; i < S; ++i)
					{
						x += Int[i].x() / S;
						y += Int[i].y() / S;
					}

					QLineF Vect({ x, y }, Lab.Point);
					QLineF Dmm = Vect; QPointF B;

					for (int i = 1; i < S; ++i) if (Vect.p2() == Lab.Point)
					{
						auto t = Vect.intersect({ Int[i - 1], Int[i] }, &B);
						if (t == QLineF::BoundedIntersection)
						{
							if (qAbs(QLineF(Vect.p1(), B).angle() - Vect.angle()) < 1.0) Vect.setP2(B);
						}
					}

					if (Vect.p2() == Lab.Point) for (int i = 1; i < S; ++i)
					{
						Vect.intersect({ Int[i - 1], Int[i] }, &B);

						const QLineF Dummy = QLineF(Vect.p1(), B);
						if (Dummy.length() < Lab.Rad && Dmm.length() < Dummy.length())
						{
							if (qAbs(Dummy.angle() - Dmm.angle()) < 1.0) Dmm.setP2(B);
						}
					}

					if (Vect.p2() == Lab.Point && Vect != Dmm) Vect.setP2(Dmm.p2());

					if (Lab.Vect == QPointF()) Lab.Vect = Vect.p2() - Vect.p1();
					else Lab.Vect = (Lab.Vect + Vect.p2() - Vect.p1()) / 2.0;

					QStringList Row = QStringList()
						<< QString::number(x, 'f', 3)
						<< QString::number(y, 'f', 3)
						<< QString::number(Surf, 'f', 2)
						<< QString::number(rat * 100.0, 'f', 0);

					Locker.lock();
					Res.insert(Row);
					Locker.unlock();
				}
			}
		}

		Locker.lock();
		emit onProgresUpdate(++Step);
		Locker.unlock();
	});

	return Res;
}

QSet<QStringList> RedactionWidget::getColisions(QList<LABEL>& Labels, const QList<QPointF>& Symbols) const
{
	if (Labels.isEmpty() || Symbols.isEmpty()) return QSet<QStringList>();

	QMutex Locker; QSet<QStringList> Res; const double Scl = smbScales.value(mapScale) / 2.0; int Step(0);

	static const auto pdistance = [] (const QLineF& L, const QPointF& P) -> double
	{
		const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
		const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
		const double l = L.length();

		if ((a * a <= l * l + b * b) &&
		    (b * b <= a * a + l * l))
		{
			const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
			const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

			return qAbs(A * D - C * B) / qSqrt(C * C + D * D);
		}
		else return INFINITY;
	};

	emit onProgressRename(tr("Computing symbols colisions (%p%)"));
	emit onProgressStart(0, Labels.size()); emit onProgresUpdate(0);

	QtConcurrent::blockingMap(Labels, [this, &Symbols, &Res, &Locker, &Step, Scl] (LABEL& Lab) -> void
	{
		for (const auto& Smb : Symbols)
		{
			if ((Lab.Rad + Scl) < QLineF(Lab.Point, Smb).length()) continue;

			bool iOK = Lab.Pol.containsPoint(Smb, Qt::OddEvenFill);
			double r(INFINITY), Surf(0.0); bool oOK(false), OK(true);

			for (int i = 1; i < Lab.Pol.size(); ++i)
			{
				const double dst = pdistance(QLineF(Lab.Pol[i - 1], Lab.Pol[i]), Smb);

				oOK = oOK || (dst <= Scl); r = qMin(r, dst);
			}

			if (iOK && r >= Scl) Surf = M_PI * Scl * Scl;
			else if (r <= Scl) Surf = Surf = M_PI * (Scl - r) * (Scl - r);

			if (tolPrc != 0.0 && (tolPrc * Lab.Sur > Surf)) OK = false;
			if (tolAbs != 0.0 && (tolAbs > Surf)) OK = false;

			if ((iOK || oOK) && OK)
			{
				QStringList Row = QStringList()
					<< QString::number(Smb.x(), 'f', 3)
					<< QString::number(Smb.y(), 'f', 3)
					<< QString::number(Surf, 'f', 2)
					<< QString::number((Surf / Lab.Sur) * 100.0, 'f', 0);

				QLineF Vect(Smb, Lab.Point); Vect.setLength(iOK ? -r : r);

				if (Lab.Vect == QPointF()) Lab.Vect = Vect.p2() - Vect.p1();
				else Lab.Vect = (Lab.Vect + Vect.p2() - Vect.p1()) / 2.0;

				Locker.lock();
				Res.insert(Row);
				Locker.unlock();
			}
		}

		Locker.lock();
		emit onProgresUpdate(++Step);
		Locker.unlock();
	});

	return Res;
}

int RedactionWidget::fixColisions(QList<RedactionWidget::LABEL>& Labels, const QHash<QString, QList<SYMBOL>>& Symbols, const QHash<QString, QList<LINE>>& Lines)
{
	QMutex Locker; int Count(0); emit onProgressStart(0, 0); const double Scl = smbScales.value(mapScale) / 2.0;

	QtConcurrent::blockingMap(Labels, [&Symbols, &Lines, &Locker, &Count, Scl] (LABEL& Lab) -> void
	{
		if (Lab.Vect != QPointF())
		{
			Lab.Point += Lab.Vect; Lab.Org += Lab.Vect;

			for (auto& P : Lab.Pol) P += Lab.Vect;

			Lab.Vect = QPointF(); Lab.Mod = true;

			Locker.lock();
			Count += 1;
			Locker.unlock();
		}
	});

	return Count;
}

QSet<QStringList> RedactionWidget::saveChanges(const QList<RedactionWidget::LABEL>& Labels)
{
	QSet<QStringList> List;

	auto Databases = Core->getDatabases();

	emit onProgressRename(tr("Saving changes (%p%)"));
	emit onProgressStart(0, Labels.size());
	emit onProgresUpdate(0); int Step(0);

	for (int i = 0; i < Databases.size(); ++i)
	{
		QSqlQuery Query(Databases[i]); Query.setForwardOnly(true);

		Query.prepare("UPDATE EW_TEXT SET "
				    "POS_X = :x, POS_Y = :y, "
				    "ODN_X = ODN_X - (:x - POS_X), "
				    "ODN_Y = ODN_Y - (:y - POS_Y) "
				    "WHERE UID = :id");

		for (const auto& L : Labels) if (L.Dbn == Databases[i].databaseName())
		{
			if (L.Mod)
			{
				List.insert(QStringList()
						  << QString::number(L.Point.x(), 'f', 3)
						  << QString::number(L.Point.y(), 'f', 3)
						  << QString::number(L.Sur, 'f', 2)
						  << QString::number(NAN, 'f', 0));

				Query.bindValue(":x", float(L.Org.x()));
				Query.bindValue(":y", float(L.Org.y()));
				Query.bindValue(":id", int(L.Uid));

				Query.exec();
			}

			emit onProgresUpdate(++Step);
		}
	}

	return List;
}

void RedactionWidget::refreshButtonClicked(void)
{
	setEnabled(false); ui->progressBar->setVisible(true);

	QtConcurrent::run(this, &RedactionWidget::refreshData);
}

void RedactionWidget::optionsButtonClicked(void)
{
	RedactionDialog* Dialog = new RedactionDialog(this); Dialog->open();

	connect(Dialog, &RedactionDialog::accepted, Dialog, &RedactionDialog::deleteLater);
	connect(Dialog, &RedactionDialog::rejected, Dialog, &RedactionDialog::deleteLater);

	connect(Dialog, &RedactionDialog::onDialogAccepted, this, &RedactionWidget::setParameters);

	Dialog->setParameters(smbScales, smbExclude, smbCompute, tolAbs, tolPrc, maxIters);
}

void RedactionWidget::saveButtonClicked(void)
{
	QStandardItemModel* Model = dynamic_cast<QStandardItemModel*>(ui->treeView->model());

	const QString Path = QFileDialog::getSaveFileName(this, tr("Save coordinates"),
											QString(), tr("Text files (*.txt)"));

	if (Path.isEmpty()) return; QFile File(Path); QTextStream Stream(&File);

	const auto Selection = ui->treeView->selectionModel()->selectedRows();

	if (!File.open(QFile::WriteOnly | QFile::Text)) return;

	for (int j = 0; j < Model->rowCount(); ++j) if (Selection.contains(Model->index(j, 0)))
	{
		Stream << Model->data(Model->index(j, 0)).toString() << '\t'
			  << Model->data(Model->index(j, 1)).toString() << '\n';
	}
}

void RedactionWidget::fixButtonClicked(void)
{
	setEnabled(false); ui->progressBar->setVisible(true);

	QtConcurrent::run(this, &RedactionWidget::fixRedaction);
}

void RedactionWidget::scaleValueChanged(int Scale)
{
	mapScale = SCALE(Scale);
}

void RedactionWidget::dataReloaded(const QSet<QStringList>& Data)
{
	QStandardItemModel* Model = new QStandardItemModel(this);

	Model->setHorizontalHeaderLabels(
	{
		tr("X"), tr("Y"), tr("Surface"), tr("Ratio")
	});

	for (const auto& R : Data)
	{
		QList<QStandardItem*> List;

		List << new QStandardItem(R.value(0));
		List << new QStandardItem(R.value(1));
		List << new QStandardItem(R.value(2) + " m²");
		List << new QStandardItem(R.value(3) + "%");

		List[0]->setData(R.value(0).toDouble(), Qt::UserRole);
		List[1]->setData(R.value(1).toDouble(), Qt::UserRole);
		List[2]->setData(R.value(2).toDouble(), Qt::UserRole);
		List[3]->setData(R.value(3).toDouble(), Qt::UserRole);

		Model->appendRow(List);
	}

	auto oldModel = ui->treeView->model();
	auto oldSelect = ui->treeView->selectionModel();

	Model->setSortRole(Qt::UserRole);
	ui->treeView->setModel(Model);

	delete oldModel;
	delete oldSelect;

	connect(ui->treeView->selectionModel(),
		   &QItemSelectionModel::selectionChanged,
		   this, &RedactionWidget::selectionChanged);

	setEnabled(true); ui->progressBar->setVisible(false);
}

void RedactionWidget::selectionChanged(void)
{
	ui->saveButton->setEnabled(ui->treeView->selectionModel()->selectedRows().size());
}

void RedactionWidget::refreshData(void)
{
	QMutexLocker Locker(&Synchronizer);
	QSet<QStringList> List;
	QList<LABEL> allLabels;
	QList<QPointF> allSmbs;

	auto Databases = Core->getDatabases();

	emit onProgressRename(tr("Loading data"));
	emit onProgressStart(0, 0);
	emit onProgresUpdate(0);

	for (int i = 0; i < Databases.size(); ++i)
	{
		const auto Tables = loadTables(Databases[i]);
		const auto Layers = loadLayers(Databases[i]);

		auto Labels = loadLabels(Databases[i]);

		for (const auto& Tab : Tables)
		{
			parseLabels(Labels, Tab, loadData(Tab, Databases[i]));
		}

		if (Labels.size()) surfLabels(Labels, mapScale, Layers);

		for (const auto& L : Labels) allLabels.append(L);

		if (smbCompute) allSmbs.append(loadSymbols(Databases[i]));
	}

	List += getColisions(allLabels);
	List += getColisions(allLabels, allSmbs);

	emit onDataReloaded(List);
}

void RedactionWidget::fixRedaction(void)
{
	QMutexLocker Locker(&Synchronizer);
	QList<LABEL> allLabels;
	QList<QPointF> allSmbs;
	int fixes(1);

	auto Databases = Core->getDatabases();

	emit onProgressRename(tr("Loading data"));
	emit onProgressStart(0, 0);
	emit onProgresUpdate(0);

	for (int i = 0; i < Databases.size(); ++i)
	{
		const auto Tables = loadTables(Databases[i]);
		const auto Layers = loadLayers(Databases[i]);

		auto Labels = loadLabels(Databases[i]);

		for (const auto& Tab : Tables)
		{
			parseLabels(Labels, Tab, loadData(Tab, Databases[i]));
		}

		if (Labels.size()) surfLabels(Labels, mapScale, Layers);

		for (const auto& L : Labels) allLabels.append(L);

		if (smbCompute) allSmbs.append(loadSymbols(Databases[i]));
	}

	const auto Symbols = loadSmbLabels();
	const auto Lines = loadLinLabels();

	for (int i = 0; i < maxIters && fixes > 0; ++i)
	{
		getColisions(allLabels);
		getColisions(allLabels, allSmbs);

		fixes = fixColisions(allLabels, Symbols, Lines);
	}

	emit onDataReloaded(saveChanges(allLabels));
}

bool operator == (const RedactionWidget::FIELD& One, const RedactionWidget::FIELD& Two)
{
	return
	(
		One.Type == Two.Type &&
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Dict == Two.Dict
	);
}

bool operator == (const RedactionWidget::TABLE& One, const RedactionWidget::TABLE& Two)
{
	return
	(
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Data == Two.Data &&
		One.Fields == Two.Fields
	);
}

QVariant getDataFromDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh)
{
	if (!Value.isValid()) return QVariant();

	if (Type == RedactionWidget::BOOL && Dict.isEmpty())
	{
		return Value.toBool() ? RedactionWidget::tr("Yes") : RedactionWidget::tr("No");
	}

	if (Type == RedactionWidget::MASK && !Dict.isEmpty())
	{
		QStringList Values; const int Bits = Value.toInt();

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (Bits & (1 << i.key().toInt())) Values.append(Sh ? i.value().second : i.value().first);
		}

		return Values.join(", ");
	}

	if (Dict.isEmpty()) return Value;

	if (Dict.contains(Value)) return Sh ? Dict[Value].second : Dict[Value].first;
	else return QVariant();
}

QVariant getDataByDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh)
{
	if (!Value.isValid() || Value.toString() == "NULL") return QVariant();

	if (Type == RedactionWidget::BOOL && Dict.isEmpty())
	{
		if (Value.toString() == RedactionWidget::tr("Yes")) return true;
		else if (Value.toString() == RedactionWidget::tr("No")) return false;
		else return Value.toBool();
	}

	if (Type == RedactionWidget::MASK && !Dict.isEmpty())
	{
		QString List = Value.toString(); int Mask = 0;

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (List.contains(Sh ? i.value().second : i.value().first)) Mask |= (1 << i.key().toInt());
		}

		return Mask;
	}

	if (Dict.isEmpty() || Dict.keys().contains(Value)) return Value;

	for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		if ((Sh ? i.value().second : i.value().first) == Value) return i.key();
	}

	return 0;
}

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return true; return false;
}
