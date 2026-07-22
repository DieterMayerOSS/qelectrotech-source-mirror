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
#include "../borderproperties.h"
#include "../bordertitleblock.h"
#include "../diagram.h"
#include "../diagramcontext.h"
#include "../titleblockproperties.h"
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
		// 90-20-0002 hat sein Terminal OBEN (orientation "n"); die Variante -0001 hat
		// es unten. Da der Verbraucher unterhalb der Sicherung sitzt und von oben
		// angefahren wird, wuerde -0001 den Leiter am Geraet vorbei und wieder
		// zurueck fuehren -> uebereinanderliegende Segmente.
	const QString LOAD_PATH = QStringLiteral("common://10_electric/10_allpole/130_terminals_terminal_strips/90_terminal_strips_diagram/90-20-0002.elmt");

		// Verbindungspunkt auf der Potentialschiene: gefuellter Punkt mit vier
		// Terminals (N/O/S/W) bei +/-2. Die Schiene laeuft dadurch gerade durch und
		// der Abgang zweigt senkrecht ab -- anders als "thruright", das eine
		// Diagonale zeichnet und die Schiene an jedem Abzweig knicken laesst.
	const QString TAP_PATH  = QStringLiteral("common://10_electric/10_allpole/114_connections/bod.elmt");

		// Fehlerstromschutzschalter, einpolige Darstellung (Terminals N/S bei +/-20)
		// aus 11_singlepole -- passend zu unserem einpolig gezeichneten Plan.
	const QString RCD_PATH  = QStringLiteral("common://10_electric/11_singlepole/200_fuses_protective_gears/50_residual_current_circuit_breaker/ddr1.elmt");

		// rail-and-drop geometry (scene coordinates), von oben nach unten:
		//   Y_RAIL  Hauptschiene (Einspeisung)
		//   Y_RCD   Fehlerstromschutzschalter einer Gruppe
		//   Y_SUB   Gruppenschiene hinter dem RCD
		//   Y_FUSE  Leitungsschutz
		//   Y_LOAD  Verbraucher
		// Direktabgaenge ohne Gruppe ueberspringen Y_RCD und Y_SUB.
	const qreal DX = 140.0, X0 = 120.0;
	const qreal Y_RAIL = 100.0, Y_RCD = 190.0, Y_SUB = 280.0,
				Y_FUSE = 370.0, Y_LOAD = 500.0;
		// Abstand der Einspeisung links vom ersten Abzweig.
	const qreal X_SRC_OFFSET = 70.0;
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
Diagram *VerteilerGenerator::generate(const VerteilerModel &model, const VerteilerConfig &config)
{
	if (model.isEmpty() || !m_project) {
		return nullptr;
	}
	Diagram *folio = m_project->addNewDiagram();
	if (!folio) {         // nullptr on read-only
		return nullptr;
	}

		// Project-level settings -> folio title block (empty fields untouched).
		// The default title block renders no address field, so the address is
		// folded into the (multi-line) title, like stromlaufplan.de's cartouche.
	TitleBlockProperties tbp = folio->border_and_titleblock.exportTitleBlock();
	QString title = config.title;
	if (!config.address.isEmpty()) {
		title = title.isEmpty()
				? config.address
				: (title + QStringLiteral("\n") + config.address);
	}
	if (!title.isEmpty()) {
		tbp.title = title;
	}
	if (!config.author.isEmpty()) {
		tbp.author = config.author;
	}
	if (!config.drawingNumber.isEmpty()) {
		tbp.folio = config.drawingNumber;
	}
	folio->border_and_titleblock.importTitleBlock(tbp);

		// Paper format: A4 uses a smaller folio grid; A3 keeps the default.
	if (config.paperSize == QLatin1String("A4")) {
		BorderProperties bp = folio->border_and_titleblock.exportBorder();
		bp.columns_count = 12;
		bp.rows_count    = 6;
		folio->border_and_titleblock.importBorder(bp);
	}

	QUndoStack &stack = folio->undoStack();
	stack.beginMacro(tr("Générer le tableau"));

	auto place = [&](Element *e, qreal x, qreal y) {
			// The command's redo() adds the item to the scene and sets its pos,
			// so after push() the element is placed and its terminals resolve.
		stack.push(new AddGraphicsObjectCommand(e, folio, QPointF(x, y)));
	};
	auto setInfo = [&](Element *e, const QString &key, const QString &value) {
		if (value.isEmpty()) {
			return;
		}
		const DiagramContext old_info = e->elementInformations();
		DiagramContext new_info = old_info;
		new_info.addValue(key, value);
		stack.push(new ChangeElementInformationCommand(e, old_info, new_info));
	};
	auto wire = [&](Terminal *a, Terminal *b) {
		if (a && b) {
			stack.push(new AddGraphicsObjectCommand(new Conductor(a, b), folio));
		}
	};
		// Terminals ueber ihre Orientierung waehlen statt ueber den Listenindex -
		// die Reihenfolge in der .elmt-Datei ist nicht garantiert.
	auto terminalAt = [](Element *e, Qet::Orientation o) -> Terminal * {
		if (e) {
			for (Terminal *t : e->terminals()) {
				if (t->orientation() == o) {
					return t;
				}
			}
		}
		return nullptr;
	};

		// Stromkreise nach FI-Gruppe buendeln, Gruppen in der Reihenfolge ihres
		// ersten Auftretens. Der leere Schluessel sammelt die Direktabgaenge.
	QList<QString> group_order;
	QHash<QString, QList<VerteilerCircuit>> by_group;
	for (const VerteilerCircuit &c : model) {
		if (!by_group.contains(c.group)) {
			group_order.append(c.group);
		}
		by_group[c.group].append(c);
	}

		// Einen kompletten Abgang aufbauen: Sicherung + Verbraucher unter einem
		// bereits gesetzten Abzweigpunkt. Gibt false zurueck, wenn ein Element fehlt.
	auto buildOutgoing = [&](Element *tap, qreal x, const VerteilerCircuit &c,
							 const QString &bmk) -> bool
	{
		Element *fuse = createElement(FUSE_PATH);
		Element *load = createElement(LOAD_PATH);
		if (!fuse || !load) {
			delete fuse; delete load;
			return false;
		}
		place(fuse, x, Y_FUSE);
		place(load, x, Y_LOAD);
		setInfo(fuse, QStringLiteral("label"),   bmk);
		setInfo(fuse, QStringLiteral("comment"), c.rating);
		setInfo(load, QStringLiteral("label"),   c.load);
		wire(terminalAt(tap,  Qet::South), terminalAt(fuse, Qet::North));
		wire(terminalAt(fuse, Qet::South), terminalAt(load, Qet::North));
		return true;
	};

	QList<Element *> main_taps;   // Abzweigpunkte auf der Hauptschiene
	int column = 0;               // fortlaufende Spalte ueber alle Gruppen hinweg
	int direct_counter = 0;       // Zaehler fuer die Auto-BMK der Direktabgaenge

	for (const QString &key : group_order)
	{
		const QList<VerteilerCircuit> circuits = by_group.value(key);

		if (key.isEmpty())
		{
				// Direktabgang: haengt unmittelbar an der Hauptschiene.
			for (const VerteilerCircuit &c : circuits)
			{
				const qreal x = X0 + column * DX;
				Element *tap = createElement(TAP_PATH);
				if (tap) {
					place(tap, x, Y_RAIL);
					main_taps.append(tap);
						// Direktabgaenge werden als -F0.x gezaehlt, damit sie sich nicht
						// mit den Gruppen-BMK (-F1, -F2, ...) ueberschneiden.
					const QString bmk = c.bmk.isEmpty()
							? QStringLiteral("-F0.%1").arg(++direct_counter)
							: c.bmk;
					buildOutgoing(tap, x, c, bmk);
				}
				++column;
			}
			continue;
		}

			// FI-Gruppe: Hauptschiene -> RCD -> Gruppenschiene -> Abgaenge.
		const qreal x_first = X0 + column * DX;
		Element *rcd_tap = createElement(TAP_PATH);
		Element *rcd     = createElement(RCD_PATH);
		if (!rcd_tap || !rcd) {
			delete rcd_tap; delete rcd;
			column += circuits.size();
			continue;
		}
		place(rcd_tap, x_first, Y_RAIL);
		place(rcd,     x_first, Y_RCD);
		main_taps.append(rcd_tap);
		wire(terminalAt(rcd_tap, Qet::South), terminalAt(rcd, Qet::North));

			// BMK des RCD: der Gruppenname, mit fuehrendem Bindestrich.
		const QString group_base = key.startsWith(QLatin1Char('-')) ? key.mid(1) : key;
		setInfo(rcd, QStringLiteral("label"), QStringLiteral("-%1").arg(group_base));

		QList<Element *> sub_taps;
		int member = 0;
		for (const VerteilerCircuit &c : circuits)
		{
			const qreal x = X0 + column * DX;
			Element *tap = createElement(TAP_PATH);
			if (tap) {
				place(tap, x, Y_SUB);
				sub_taps.append(tap);
					// Hierarchische Auto-BMK innerhalb der Gruppe: -F1.1, -F1.2, ...
				const QString bmk = c.bmk.isEmpty()
						? QStringLiteral("-%1.%2").arg(group_base).arg(member + 1)
						: c.bmk;
				buildOutgoing(tap, x, c, bmk);
			}
			++column;
			++member;
		}

		if (!sub_taps.isEmpty())
		{
				// RCD speist die Gruppenschiene, dann Punkt zu Punkt weiter.
			wire(terminalAt(rcd, Qet::South), terminalAt(sub_taps.first(), Qet::North));
			for (int t = 1; t < sub_taps.size(); ++t) {
				wire(terminalAt(sub_taps.at(t - 1), Qet::East),
					 terminalAt(sub_taps.at(t),     Qet::West));
			}
		}
	}

		// Waagrechte Potentialschiene: Einspeisung -> erster Abzweig, dann von
		// Abzweig zu Abzweig. Alle Punkte liegen auf Y_RAIL, die Leiter werden
		// daher gerade und koennen sich nicht ueberlagern.
	if (!main_taps.isEmpty())
	{
		if (Element *src = createElement(SRC_PATH))
		{
			place(src, X0 - X_SRC_OFFSET, Y_RAIL);
			wire(terminalAt(src, Qet::East), terminalAt(main_taps.first(), Qet::West));
		}
		for (int t = 1; t < main_taps.size(); ++t) {
			wire(terminalAt(main_taps.at(t - 1), Qet::East),
				 terminalAt(main_taps.at(t),     Qet::West));
		}
	}

	stack.endMacro();
	return folio;
}
