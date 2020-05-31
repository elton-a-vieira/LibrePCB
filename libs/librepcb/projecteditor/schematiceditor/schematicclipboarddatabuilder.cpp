/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematicclipboarddatabuilder.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/schematics/schematicselectionquery.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicClipboardDataBuilder::SchematicClipboardDataBuilder(
    Schematic& schematic) noexcept
  : mSchematic(schematic) {
}

SchematicClipboardDataBuilder::~SchematicClipboardDataBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<SchematicClipboardData> SchematicClipboardDataBuilder::generate(
    const Point& cursorPos) const noexcept {
  std::unique_ptr<SchematicClipboardData> data(
      new SchematicClipboardData(mSchematic.getUuid(), cursorPos));

  // get all selected items
  std::unique_ptr<SchematicSelectionQuery> query(
      mSchematic.createSelectionQuery());
  query->addSelectedSymbols();
  // query->addSelectedNetLines();
  // query->addSelectedNetLabels();
  // query->addNetPointsOfNetLines();
  // query->addNetLinesOfSymbolPins();

  // add net signals
  foreach (const NetSignal* netsignal,
           mSchematic.getProject().getCircuit().getNetSignals()) {
    data->getNetSignals().append(
        std::make_shared<SchematicClipboardData::NetSignal>(
            netsignal->getUuid(), netsignal->getName(),
            netsignal->hasAutoName()));
  }

  // add components
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    std::unique_ptr<TransactionalDirectory> dir = data->getDirectory(
        "cmp/" %
        symbol->getComponentInstance().getLibComponent().getUuid().toStr());
    if (dir->getFiles().isEmpty()) {
      symbol->getComponentInstance().getLibComponent().getDirectory().copyTo(
          *dir);
    }
    QMap<Uuid, tl::optional<Uuid>> signalMap;  // TODO
    data->getComponentInstances().append(
        std::make_shared<SchematicClipboardData::ComponentInstance>(
            symbol->getComponentInstance().getUuid(),
            symbol->getComponentInstance().getLibComponent().getUuid(),
            symbol->getComponentInstance().getSymbolVariant().getUuid(),
            symbol->getComponentInstance().getDefaultDeviceUuid(),
            symbol->getComponentInstance().getName(),
            symbol->getComponentInstance().getValue(),
            symbol->getComponentInstance().getAttributes(), signalMap));
  }

  // add symbols
  foreach (SI_Symbol* symbol, query->getSymbols()) {
    std::unique_ptr<TransactionalDirectory> dir =
        data->getDirectory("sym/" % symbol->getLibSymbol().getUuid().toStr());
    if (dir->getFiles().isEmpty()) {
      symbol->getLibSymbol().getDirectory().copyTo(*dir);
    }
    data->getSymbolInstances().append(
        std::make_shared<SchematicClipboardData::SymbolInstance>(
            symbol->getUuid(), symbol->getComponentInstance().getUuid(),
            symbol->getCompSymbVarItem().getUuid(), symbol->getPosition(),
            symbol->getRotation(), symbol->getMirrored()));
  }

  return data;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
