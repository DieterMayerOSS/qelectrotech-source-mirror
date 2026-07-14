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
#ifndef VERTEILERGENERATOR_H
#define VERTEILERGENERATOR_H

#include <QCoreApplication>
#include <QHash>
#include <QString>

#include "verteilermodel.h"

class QETProject;
class Diagram;
class Element;

/**
	@brief The VerteilerGenerator class
	Milestone 2: builds one demonstration folio using a "rail-and-drop" layout
	from a fixed mini-model (N circuits: incoming supply -> fuse -> load,
	auto-placed, auto-wired, auto-labelled with the BMK). Later milestones
	replace the fixed model with a real distribution-board data model and a
	rule engine. Reuses QET's own element factory, conductors and undo commands.
*/
class VerteilerGenerator
{
		Q_DECLARE_TR_FUNCTIONS(VerteilerGenerator)

	public:
		explicit VerteilerGenerator(QETProject *project);

			/// Generate a folio from the given model and project settings.
			/// Returns it, or nullptr (no/read-only project, or an empty model).
		Diagram *generate(const VerteilerModel &model, const VerteilerConfig &config);

	private:
		Element *createElement(const QString &common_path);

		QETProject *m_project = nullptr;
			/// common:// path -> project-embedded path, to import each element only once.
		QHash<QString, QString> m_embed_cache;
};

#endif // VERTEILERGENERATOR_H
