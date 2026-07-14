/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VERTEILERMODEL_H
#define VERTEILERMODEL_H

#include <QString>
#include <QVector>

/**
	@brief The VerteilerCircuit struct
	One circuit of the distribution-board model. Kept intentionally small for
	Milestone 3; later milestones extend it (RCD group, rating, cable type, ...).
*/
struct VerteilerCircuit
{
	QString bmk;     ///< device tag / Betriebsmittelkennzeichen, e.g. "-F1"
	QString rating;  ///< protective device rating / Kennwert, e.g. "B16A"
	QString load;    ///< consumer designation, e.g. "KG Steckdosen"
};

/// The distribution-board model: an ordered list of circuits.
using VerteilerModel = QVector<VerteilerCircuit>;

#endif // VERTEILERMODEL_H
