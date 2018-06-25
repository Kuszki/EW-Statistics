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

#include "commonstructures.hpp"

uint qHash(const DBINFO& K)
{
	return qHash(K.Name + K.Server + K.Path);
}

bool operator == (const DBINFO& A, const DBINFO& B)
{
	return A.Path == B.Path && A.Server == B.Server;
}

bool operator != (const DBINFO& A, const DBINFO& B)
{
	return A.Path != B.Path || A.Server != B.Server;
}


QDataStream& operator << (QDataStream& Stream, const DBINFO& V)
{
	Stream << V.Name << V.Server << V.Path << V.Enabled; return Stream;
}

QDataStream& operator >> (QDataStream& Stream, DBINFO& V)
{
	Stream >> V.Name;
	Stream >> V.Server;
	Stream >> V.Path;
	Stream >> V.Enabled;

	return Stream;
}

bool pointComp(const QPointF& A, const QPointF& B, double d)
{
	return (qAbs(A.x() - B.x()) <= d) && (qAbs(A.y() - B.y()) <= d);
}

bool isVariantEmpty(const QVariant& Value)
{
	if (Value.isNull()) return true;
	else switch (Value.type())
	{
		case QVariant::Int: return Value == QVariant(int(0));
		case QVariant::Double: return Value == QVariant(double(0.0));
		case QVariant::Date: return Value == QVariant(QDate());
		case QVariant::DateTime: return Value == QVariant(QDateTime());
		case QVariant::String: return Value == QVariant(QString());
		case QVariant::List: return Value == QVariantList();

		default: return false;
	}
}

double getSurface(const QPolygonF& P)
{
	double sum(0.0); for (int i = 0; i < P.size(); ++i)
	{
		const double yn = (i + 1) == P.size() ? P[0].y() : P[i + 1].y();
		const double yp = i == 0 ? P.last().y() : P[i - 1].y();

		const double xi = P[i].x();

		sum += xi * (yn - yp);
	}

	return qAbs(sum / 2.0);
}
