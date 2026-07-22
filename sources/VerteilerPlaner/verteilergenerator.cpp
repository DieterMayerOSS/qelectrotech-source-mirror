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
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetproject.h"
#include "../TerminalStrip/terminalstrip.h"
#include "../TerminalStrip/UndoCommand/addterminalstripcommand.h"
#include "../TerminalStrip/UndoCommand/addterminaltostripcommand.h"
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

		// Reihenklemme mit link_type="terminal" -- solche Elemente erkennt QET als
		// echte Klemme (TerminalElement), sie stehen damit dem Klemmleisten-Editor
		// und dem Klemmenplan zur Verfuegung. Terminals symmetrisch bei +/-10.
	const QString TERM_PATH = QStringLiteral("common://10_electric/10_allpole/130_terminals_terminal_strips/borne_continuite.elmt");

		// rail-and-drop geometry (scene coordinates), von oben nach unten:
		//   Y_RAIL  Hauptschiene (Einspeisung)
		//   Y_RCD   Fehlerstromschutzschalter einer Gruppe
		//   Y_SUB   Gruppenschiene hinter dem RCD
		//   Y_FUSE  Leitungsschutz
		//   Y_TERM  Reihenklemme (Uebergabe an die Feldleitung)
		//   Y_LOAD  Verbraucher
		// Direktabgaenge ohne Gruppe ueberspringen Y_RCD und Y_SUB.
	const qreal DX = 140.0, X0 = 120.0;
		// Drei Phasenschienen uebereinander. Ein Abgang zweigt von genau einer
		// Schiene ab; der Abgang kreuzt die darunterliegenden Schienen ohne
		// Verbindungspunkt und ist damit korrekt als "nicht verbunden" lesbar.
	const qreal Y_L1 = 60.0, Y_L2 = 90.0, Y_L3 = 120.0;
	const qreal Y_RCD = 190.0, Y_SUB = 260.0,
				Y_FUSE = 335.0, Y_TERM = 415.0, Y_LOAD = 500.0;
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

		// Fortlaufende Klemmennummer ueber alle Abgaenge des Folios hinweg.
	int terminal_number = 0;
		// Die erzeugten Klemmen werden am Ende zu einer Klemmleiste zusammengefasst.
	QVector<QSharedPointer<RealTerminal>> real_terminals;

		// Einen kompletten Abgang aufbauen: Sicherung, Reihenklemme und Verbraucher
		// unter einem bereits gesetzten Abzweigpunkt.
		// Gibt false zurueck, wenn ein Element fehlt.
	auto buildOutgoing = [&](Element *tap, qreal x, const VerteilerCircuit &c,
							 const QString &bmk) -> bool
	{
		Element *fuse = createElement(FUSE_PATH);
		Element *term = createElement(TERM_PATH);
		Element *load = createElement(LOAD_PATH);
		if (!fuse || !term || !load) {
			delete fuse; delete term; delete load;
			return false;
		}
		place(fuse, x, Y_FUSE);
		place(term, x, Y_TERM);
		place(load, x, Y_LOAD);
		setInfo(fuse, QStringLiteral("label"),   bmk);
		setInfo(fuse, QStringLiteral("comment"), c.rating);
			// Klemmen werden je Leiste fortlaufend ab 1 nummeriert.
		setInfo(term, QStringLiteral("label"),
				QString::number(++terminal_number));
			// borne_continuite traegt link_type="terminal", QET erzeugt dafuer ein
			// TerminalElement -- dessen RealTerminal sammeln wir fuer die Leiste.
		if (auto *term_element = dynamic_cast<TerminalElement *>(term)) {
			if (auto real_t = term_element->realTerminal()) {
				real_terminals.append(real_t);
			}
		}
		setInfo(load, QStringLiteral("label"),   c.load);
		wire(terminalAt(tap,  Qet::South), terminalAt(fuse, Qet::North));
		wire(terminalAt(fuse, Qet::South), terminalAt(term, Qet::North));
		wire(terminalAt(term, Qet::South), terminalAt(load, Qet::North));
		return true;
	};

		// Abzweigpunkte je Phasenschiene, in Spaltenreihenfolge gesammelt.
	const QStringList PHASES { QStringLiteral("L1"), QStringLiteral("L2"),
							   QStringLiteral("L3") };
	QHash<QString, qreal> rail_y {
		{ PHASES.at(0), Y_L1 }, { PHASES.at(1), Y_L2 }, { PHASES.at(2), Y_L3 } };
	QHash<QString, QList<Element *>> rail_taps;

	int auto_phase = 0;           // reihum fuer Abgaenge ohne Phasenvorgabe
		// Phase bestimmen: manuelle Angabe schlaegt die Automatik. Der Zaehler
		// laeuft nur bei automatischer Vergabe weiter, damit manuell gesetzte
		// Abgaenge die gleichmaessige Verteilung nicht verschieben.
	auto choosePhase = [&](const QString &manual) -> QString {
		const QString up = manual.trimmed().toUpper();
		if (rail_y.contains(up)) {
			return up;
		}
		return PHASES.at(auto_phase++ % PHASES.size());
	};

	int column = 0;               // fortlaufende Spalte ueber alle Gruppen hinweg
	int direct_counter = 0;       // Zaehler fuer die Auto-BMK der Direktabgaenge

	for (const QString &key : group_order)
	{
		const QList<VerteilerCircuit> circuits = by_group.value(key);

		if (key.isEmpty())
		{
				// Direktabgang: haengt unmittelbar an einer Phasenschiene.
			for (const VerteilerCircuit &c : circuits)
			{
				const qreal x = X0 + column * DX;
				Element *tap = createElement(TAP_PATH);
				if (tap) {
					const QString phase = choosePhase(c.phase);
					place(tap, x, rail_y.value(phase));
					rail_taps[phase].append(tap);
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
			// Die Gruppe haengt als Ganzes an einer Phase; ihre Abgaenge erben sie
			// ueber die Gruppenschiene. Massgeblich ist die Vorgabe des ersten
			// Stromkreises der Gruppe.
		const QString group_phase = choosePhase(circuits.first().phase);
		place(rcd_tap, x_first, rail_y.value(group_phase));
		place(rcd,     x_first, Y_RCD);
		rail_taps[group_phase].append(rcd_tap);
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
	for (const QString &phase : PHASES)
	{
		const QList<Element *> taps = rail_taps.value(phase);
		if (taps.isEmpty()) {
			continue;           // unbenutzte Phase bekommt keine Schiene
		}
		if (Element *src = createElement(SRC_PATH))
		{
			place(src, X0 - X_SRC_OFFSET, rail_y.value(phase));
			setInfo(src, QStringLiteral("label"), phase);
			wire(terminalAt(src, Qet::East), terminalAt(taps.first(), Qet::West));
		}
		for (int t = 1; t < taps.size(); ++t) {
			wire(terminalAt(taps.at(t - 1), Qet::East),
				 terminalAt(taps.at(t),     Qet::West));
		}
	}

		// Die erzeugten Klemmen zu einer Klemmleiste zusammenfassen. Ohne diesen
		// Schritt blieben sie freie Klemmen und tauchten im Klemmenplan nicht auf.
	if (!real_terminals.isEmpty())
	{
		auto *strip = new TerminalStrip(m_project);
		strip->setName(config.title.isEmpty()
					   ? tr("Tableau")
					   : config.title);
		stack.push(new AddTerminalStripCommand(strip, m_project));
		stack.push(new AddTerminalToStripCommand(real_terminals, strip));
	}

	stack.endMacro();
	return folio;
}
