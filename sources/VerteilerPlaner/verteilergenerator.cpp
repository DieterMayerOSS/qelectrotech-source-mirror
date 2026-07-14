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
#include "verteilergenerator.h"

#include "../ElementsCollection/elementslocation.h"
#include "../diagram.h"
#include "../diagramcontext.h"
#include "../factory/elementfactory.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/changeelementinformationcommand.h"

#include <QUndoStack>
#include <QVector>

namespace {
		// Built-in (common://) library elements used by the demo model.
	const QString SRC_PATH  = QStringLiteral("common://10_electric/10_allpole/100_folio_referencing/01coming_arrow.elmt");
	const QString FUSE_PATH = QStringLiteral("common://10_electric/10_allpole/200_fuses_protective_gears/10_fuses/pojistka1p.elmt");
	const QString LOAD_PATH = QStringLiteral("common://10_electric/10_allpole/130_terminals_terminal_strips/90_terminal_strips_diagram/90-20-0001.elmt");

		// rail-and-drop geometry (scene coordinates)
	const qreal DX = 140.0, X0 = 120.0, Y_SRC = 100.0, Y_FUSE = 240.0, Y_LOAD = 400.0;

	struct Circuit { QString bmk; QString load_label; };
}

VerteilerGenerator::VerteilerGenerator(QETProject *project) :
	m_project(project)
{}

/**
	@brief VerteilerGenerator::createElement
	Import (once, cached) the built-in element into the project collection, then
	instantiate a fresh copy from the embedded location.
	@return the new element, or nullptr if it cannot be built.
*/
Element *VerteilerGenerator::createElement(const QString &common_path)
{
	QString embed = m_embed_cache.value(common_path);
	if (embed.isEmpty())
	{
			// common:// is a built-in collection path: do NOT pass the project
			// (the project argument is only for embed:// project-local locations).
		ElementsLocation loc(common_path);
		ElementsLocation imported = m_project->importElement(loc);
		if (!imported.exist()) {
			return nullptr;
		}
		embed = imported.projectCollectionPath();
		m_embed_cache.insert(common_path, embed);
	}

	int state = 0;
	Element *e = ElementFactory::Instance()->createElement(
				ElementsLocation(embed), nullptr, &state);
	if (state) {          // build failed
		delete e;
		return nullptr;
	}
	return e;
}

/**
	@brief VerteilerGenerator::generate
	Create one folio and fill it with the demo model, as a single undo macro.
	NB: the folio creation itself is not undoable (QET has no folio undo command).
*/
Diagram *VerteilerGenerator::generate()
{
	Diagram *folio = m_project ? m_project->addNewDiagram() : nullptr;
	if (!folio) {         // nullptr on read-only / no project
		return nullptr;
	}

		// Fixed demo model (replaced by a real data model in a later milestone).
	const QVector<Circuit> model {
		{ QStringLiteral("-F1"), QStringLiteral("KG Steckdosen") },
		{ QStringLiteral("-F2"), QStringLiteral("EG Licht") },
		{ QStringLiteral("-F3"), QStringLiteral("OG Rolladen") },
	};

	QUndoStack &stack = folio->undoStack();
	stack.beginMacro(tr("Générer le tableau"));

	auto place = [&](Element *e, qreal x, qreal y) {
			// The command's redo() adds the item to the scene and sets its pos,
			// so after push() the element is placed and its terminals resolve.
		stack.push(new AddGraphicsObjectCommand(e, folio, QPointF(x, y)));
	};
	auto setLabel = [&](Element *e, const QString &label) {
		const DiagramContext old_info = e->elementInformations();
		DiagramContext new_info = old_info;
		new_info.addValue(QStringLiteral("label"), label);
		stack.push(new ChangeElementInformationCommand(e, old_info, new_info));
	};
	auto wire = [&](Terminal *a, Terminal *b) {
		if (a && b) {
			stack.push(new AddGraphicsObjectCommand(new Conductor(a, b), folio));
		}
	};

	int i = 0;
	for (const Circuit &c : model)
	{
		const qreal x = X0 + i * DX;
		Element *src  = createElement(SRC_PATH);
		Element *fuse = createElement(FUSE_PATH);
		Element *load = createElement(LOAD_PATH);
		if (!src || !fuse || !load)   // a library element is missing -> skip cleanly
		{
			delete src; delete fuse; delete load;
			++i;
			continue;
		}

		place(src,  x, Y_SRC);
		place(fuse, x, Y_FUSE);
		place(load, x, Y_LOAD);
		setLabel(fuse, c.bmk);
		setLabel(load, c.load_label);

			// Rail-and-drop wiring: supply -> fuse(top) ; fuse(bottom) -> load.
		const QList<Terminal *> st = src->terminals();
		const QList<Terminal *> ft = fuse->terminals();
		const QList<Terminal *> lt = load->terminals();
		if (!st.isEmpty() && ft.size() >= 2) {
			wire(st.first(), ft.at(0));
		}
		if (ft.size() >= 2 && !lt.isEmpty()) {
			wire(ft.at(1), lt.first());
		}

		++i;
	}

	stack.endMacro();
	return folio;
}
