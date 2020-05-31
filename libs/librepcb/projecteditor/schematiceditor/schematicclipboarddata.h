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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/units/point.h>
#include <librepcb/common/uuid.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/schematics/items/si_symbol.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalFileSystem;
class TransactionalDirectory;

namespace library {
class LibraryBaseElement;
}

namespace project {

class NetSignal;

namespace editor {

/*******************************************************************************
 *  Class SchematicClipboardData
 ******************************************************************************/

/**
 * @brief The SchematicClipboardData class
 */
class SchematicClipboardData final : public SerializableObject {
public:
  // Types
  struct NetSignal : public SerializableObject {
    static constexpr const char* tagname = "netsignal";

    Uuid              uuid;
    CircuitIdentifier name;
    bool              hasAutoName;

    Signal<NetSignal> onEdited;  ///< Dummy event, not used

    NetSignal(const Uuid& uuid, const CircuitIdentifier& name, bool hasAutoName)
      : uuid(uuid), name(name), hasAutoName(hasAutoName), onEdited(*this) {}

    NetSignal(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        name(node.getValueByPath<CircuitIdentifier>("name", true)),
        hasAutoName(node.getValueByPath<bool>("auto")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("auto", hasAutoName, false);
      root.appendChild("name", name, false);
    }
  };

  struct ComponentInstance : public SerializableObject {
    static constexpr const char* tagname = "component";

    Uuid                           uuid;
    Uuid                           libComponentUuid;
    Uuid                           libVariantUuid;
    tl::optional<Uuid>             libDeviceUuid;
    CircuitIdentifier              name;
    QString                        value;
    AttributeList                  attributes;
    QMap<Uuid, tl::optional<Uuid>> signalMap;

    Signal<ComponentInstance> onEdited;  ///< Dummy event, not used

    ComponentInstance(const Uuid& uuid, const Uuid& libComponentUuid,
                      const Uuid&               libVariantUuid,
                      const tl::optional<Uuid>& libDeviceUuid,
                      const CircuitIdentifier& name, const QString& value,
                      const AttributeList&                  attributes,
                      const QMap<Uuid, tl::optional<Uuid>>& signalMap)
      : uuid(uuid),
        libComponentUuid(libComponentUuid),
        libVariantUuid(libVariantUuid),
        libDeviceUuid(libDeviceUuid),
        name(name),
        value(value),
        attributes(attributes),
        signalMap(signalMap),
        onEdited(*this) {}

    ComponentInstance(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        libComponentUuid(node.getValueByPath<Uuid>("lib_component")),
        libVariantUuid(node.getValueByPath<Uuid>("lib_variant")),
        libDeviceUuid(node.getValueByPath<tl::optional<Uuid>>("lib_device")),
        name(node.getValueByPath<CircuitIdentifier>("name", true)),
        value(node.getValueByPath<QString>("value")),
        attributes(node),
        signalMap(),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("lib_component", libComponentUuid, true);
      root.appendChild("lib_variant", libVariantUuid, true);
      root.appendChild("lib_device", libDeviceUuid, true);
      root.appendChild("name", name, true);
      root.appendChild("value", value, false);
      attributes.serialize(root);
      // serializePointerContainer(root, mSignals, "signal");
    }
  };

  struct SymbolInstance : public SerializableObject {
    static constexpr const char* tagname = "symbol";

    Uuid  uuid;
    Uuid  componentInstanceUuid;
    Uuid  symbolVariantItemUuid;
    Point position;
    Angle rotation;
    bool  mirrored;

    Signal<SymbolInstance> onEdited;  ///< Dummy event, not used

    SymbolInstance(const Uuid& uuid, const Uuid& componentInstanceUuid,
                   const Uuid& symbolVariantItemUuid, const Point& position,
                   const Angle& rotation, bool mirrored)
      : uuid(uuid),
        componentInstanceUuid(componentInstanceUuid),
        symbolVariantItemUuid(symbolVariantItemUuid),
        position(position),
        rotation(rotation),
        mirrored(mirrored),
        onEdited(*this) {}

    SymbolInstance(const SExpression& node)
      : uuid(node.getChildByIndex(0).getValue<Uuid>()),
        componentInstanceUuid(node.getValueByPath<Uuid>("component")),
        symbolVariantItemUuid(node.getValueByPath<Uuid>("lib_gate")),
        position(node.getChildByPath("position")),
        rotation(node.getValueByPath<Angle>("rotation")),
        mirrored(node.getValueByPath<bool>("mirror")),
        onEdited(*this) {}

    /// @copydoc ::librepcb::SerializableObject::serialize()
    void serialize(SExpression& root) const override {
      root.appendChild(uuid);
      root.appendChild("component", componentInstanceUuid, true);
      root.appendChild("lib_gate", symbolVariantItemUuid, true);
      root.appendChild(position.serializeToDomElement("position"), true);
      root.appendChild("rotation", rotation, false);
      root.appendChild("mirror", mirrored, false);
    }
  };

  // Constructors / Destructor
  SchematicClipboardData()                                    = delete;
  SchematicClipboardData(const SchematicClipboardData& other) = delete;
  SchematicClipboardData(const Uuid&  schematicUuid,
                         const Point& cursorPos) noexcept;
  explicit SchematicClipboardData(const QByteArray& mimeData);
  ~SchematicClipboardData() noexcept;

  // Getters
  std::unique_ptr<TransactionalDirectory> getDirectory(
      const QString& path = "") noexcept;
  const Uuid&  getSchematicUuid() const noexcept { return mSchematicUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  SerializableObjectList<NetSignal, NetSignal>& getNetSignals() noexcept {
    return mNetSignals;
  }
  SerializableObjectList<ComponentInstance, ComponentInstance>&
  getComponentInstances() noexcept {
    return mComponentInstances;
  }
  SerializableObjectList<SymbolInstance, SymbolInstance>&
  getSymbolInstances() noexcept {
    return mSymbolInstances;
  }

  // General Methods
  std::unique_ptr<QMimeData>                     toMimeData() const;
  static std::unique_ptr<SchematicClipboardData> fromMimeData(
      const QMimeData* mime);

  // Operator Overloadings
  SchematicClipboardData& operator=(const SchematicClipboardData& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  static QString getMimeType() noexcept;

private:  // Data
  std::shared_ptr<TransactionalFileSystem>     mFileSystem;
  Uuid                                         mSchematicUuid;
  Point                                        mCursorPos;
  SerializableObjectList<NetSignal, NetSignal> mNetSignals;
  SerializableObjectList<ComponentInstance, ComponentInstance>
                                                         mComponentInstances;
  SerializableObjectList<SymbolInstance, SymbolInstance> mSymbolInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_SCHEMATICCLIPBOARDDATA_H
