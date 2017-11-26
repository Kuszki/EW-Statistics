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
