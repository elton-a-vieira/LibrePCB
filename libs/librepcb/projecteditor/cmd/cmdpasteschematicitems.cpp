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
#include "cmdpasteschematicitems.h"

#include "../schematiceditor/schematicclipboarddata.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdcomponentinstanceadd.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdsymbolinstanceadd.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/project/settings/projectsettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteSchematicItems::CmdPasteSchematicItems(
    Schematic& schematic,
    std::unique_ptr<librepcb::project::editor::SchematicClipboardData> data,
    const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Schematic Elements")),
    mProject(schematic.getProject()),
    mSchematic(schematic),
    mData(std::move(data)),
    mPosOffset(posOffset) {
  Q_ASSERT(mData);
}

CmdPasteSchematicItems::~CmdPasteSchematicItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteSchematicItems::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // Notes:
  //
  //  - If the UUID is already existing, or the destination schematic is
  //    different to the source schematic, generate a new random UUID.
  //    Otherwise use the same UUID to avoid modifications after cut+paste
  //    within one schematic.
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  // Copy new components to project library
  std::unique_ptr<TransactionalDirectory> cmpDir = mData->getDirectory("cmp");
  foreach (const QString& dirname, cmpDir->getDirs()) {
    if (!mProject.getLibrary().getComponent(Uuid::fromString(dirname))) {
      QScopedPointer<library::Component> cmp(
          new library::Component(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*cmpDir, dirname))));
      execNewChildCmd(new CmdProjectLibraryAddElement<library::Component>(
          mProject.getLibrary(), *cmp.take()));
    }
  }

  // Copy new symbols to project library
  std::unique_ptr<TransactionalDirectory> symDir = mData->getDirectory("sym");
  foreach (const QString& dirname, symDir->getDirs()) {
    if (!mProject.getLibrary().getSymbol(Uuid::fromString(dirname))) {
      QScopedPointer<library::Symbol> cmp(
          new library::Symbol(std::unique_ptr<TransactionalDirectory>(
              new TransactionalDirectory(*symDir, dirname))));
      execNewChildCmd(new CmdProjectLibraryAddElement<library::Symbol>(
          mProject.getLibrary(), *cmp.take()));
    }
  }

  // Paste components
  QHash<Uuid, Uuid> componentInstanceMap;
  for (const SchematicClipboardData::ComponentInstance& cmp :
       mData->getComponentInstances()) {
    const library::Component* libCmp =
        mProject.getLibrary().getComponent(cmp.libComponentUuid);
    if (!libCmp) throw LogicError(__FILE__, __LINE__);

    CircuitIdentifier name = cmp.name;
    if (mProject.getCircuit().getComponentInstanceByName(*name)) {
      name = CircuitIdentifier(
          mProject.getCircuit().generateAutoComponentInstanceName(
              libCmp->getPrefixes().value(
                  mProject.getSettings().getLocaleOrder())));
    }
    QScopedPointer<ComponentInstance> copy(
        new ComponentInstance(mProject.getCircuit(), *libCmp,
                              cmp.libVariantUuid, name, cmp.libDeviceUuid));
    componentInstanceMap.insert(cmp.uuid, copy->getUuid());
    execNewChildCmd(
        new CmdComponentInstanceAdd(mProject.getCircuit(), copy.take()));
  }

  // Paste symbols
  for (const SchematicClipboardData::SymbolInstance& sym :
       mData->getSymbolInstances()) {
    ComponentInstance* cmpInst =
        mProject.getCircuit().getComponentInstanceByUuid(
            componentInstanceMap.value(sym.componentInstanceUuid,
                                       Uuid::createRandom()));
    if (!cmpInst) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<SI_Symbol> copy(
        new SI_Symbol(mSchematic, *cmpInst, sym.symbolVariantItemUuid,
                      sym.position, sym.rotation, sym.mirrored));
    copy->setSelected(true);
    execNewChildCmd(new CmdSymbolInstanceAdd(*copy.take()));
  }

  undoScopeGuard.dismiss();  // no undo required
  return getChildCount() > 0;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
