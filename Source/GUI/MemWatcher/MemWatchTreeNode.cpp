#include "MemWatchTreeNode.h"

#include <QJsonArray>

#include <sstream>

MemWatchTreeNode::MemWatchTreeNode(MemWatchEntry* entry, MemWatchTreeNode* parent,
                                   const bool isGroup, const QString& groupName)
    : m_entry(entry), m_parent(parent), m_isGroup(isGroup), m_groupName(groupName)
{
}

MemWatchTreeNode::~MemWatchTreeNode()
{
  qDeleteAll(m_children);
}

bool MemWatchTreeNode::isGroup() const
{
  return m_isGroup;
}

bool MemWatchTreeNode::hasChildren() const
{
  return (m_children.count() >= 1);
}

int MemWatchTreeNode::childrenCount() const
{
  return m_children.count();
}

void MemWatchTreeNode::deleteAllChildren()
{
  qDeleteAll(m_children);
}

bool MemWatchTreeNode::isValueEditing() const
{
  return m_isValueEditing;
}

void MemWatchTreeNode::setValueEditing(const bool valueEditing)
{
  m_isValueEditing = valueEditing;
}

QString MemWatchTreeNode::getGroupName() const
{
  return m_groupName;
}

void MemWatchTreeNode::setGroupName(const QString& groupName)
{
  m_groupName = groupName;
}

MemWatchEntry* MemWatchTreeNode::getEntry() const
{
  return m_entry;
}

void MemWatchTreeNode::setEntry(MemWatchEntry* entry)
{
  delete m_entry;
  m_entry = entry;
}

QVector<MemWatchTreeNode*> MemWatchTreeNode::getChildren() const
{
  return m_children;
}

MemWatchTreeNode* MemWatchTreeNode::getParent() const
{
  return m_parent;
}

int MemWatchTreeNode::getRow() const
{
  if (m_parent)
    return m_parent->m_children.indexOf(const_cast<MemWatchTreeNode*>(this));

  return 0;
}

void MemWatchTreeNode::appendChild(MemWatchTreeNode* node)
{
  m_children.append(node);
  node->m_parent = this;
}

void MemWatchTreeNode::insertChild(const int row, MemWatchTreeNode* node)
{
  m_children.insert(row, node);
  node->m_parent = this;
}

void MemWatchTreeNode::removeChild(const int row)
{
  m_children.remove(row);
}

void MemWatchTreeNode::readFromJson(const QJsonObject& json, MemWatchTreeNode* parent)
{
  m_parent = parent;
  if (json["watchList"] != QJsonValue::Undefined)
  {
    m_isGroup = false;
    QJsonArray watchList = json["watchList"].toArray();
    for (auto i : watchList)
    {
      QJsonObject node = i.toObject();
      MemWatchTreeNode* childNode = new MemWatchTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else if (json["groupName"] != QJsonValue::Undefined)
  {
    m_isGroup = true;
    m_groupName = json["groupName"].toString();
    QJsonArray groupEntries = json["groupEntries"].toArray();
    for (auto i : groupEntries)
    {
      QJsonObject node = i.toObject();
      MemWatchTreeNode* childNode = new MemWatchTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else
  {
    m_isGroup = false;
    m_entry = new MemWatchEntry();
    m_entry->setLabel(json["label"].toString().toStdString());
    std::stringstream ss(json["address"].toString().toStdString());
    u32 address = 0;
    ss >> std::hex >> std::uppercase >> address;
    m_entry->setConsoleAddress(address);
    m_entry->setType(static_cast<Common::MemType>(json["typeIndex"].toInt()));
    m_entry->setSignedUnsigned(json["unsigned"].toBool());
    if (m_entry->getType() == Common::MemType::type_string ||
        m_entry->getType() == Common::MemType::type_byteArray)
    {
      m_entry->setLength(static_cast<unsigned int>(json["length"].toDouble()));
    }
    m_entry->setBase(static_cast<Common::MemBase>(json["baseIndex"].toInt()));
    if (json["pointerOffsets"] != QJsonValue::Undefined)
    {
      m_entry->setBoundToPointer(true);
      QJsonArray pointerOffsets = json["pointerOffsets"].toArray();
      for (auto i : pointerOffsets)
      {
        std::stringstream ssOffset(i.toString().toStdString());
        int offset = 0;
        ssOffset >> std::hex >> std::uppercase >> offset;
        m_entry->addOffset(offset);
      }
    }
    else
    {
      m_entry->setBoundToPointer(false);
    }
  }
}

void MemWatchTreeNode::writeToJson(QJsonObject& json) const
{
  if (isGroup())
  {
    json["groupName"] = m_groupName;
    QJsonArray entries;
    for (auto i : m_children)
    {
      QJsonObject theNode;
      i->writeToJson(theNode);
      entries.append(theNode);
    }
    json["groupEntries"] = entries;
  }
  else
  {
    if (m_parent == nullptr)
    {
      QJsonArray watchList;
      for (auto i : m_children)
      {
        QJsonObject theNode;
        i->writeToJson(theNode);
        watchList.append(theNode);
      }
      json["watchList"] = watchList;
    }
    else
    {
      json["label"] = QString::fromStdString(m_entry->getLabel());
      std::stringstream ss;
      ss << std::hex << std::uppercase << m_entry->getConsoleAddress();
      json["address"] = QString::fromStdString(ss.str());
      json["typeIndex"] = static_cast<double>(m_entry->getType());
      json["unsigned"] = m_entry->isUnsigned();
      if (m_entry->getType() == Common::MemType::type_string ||
          m_entry->getType() == Common::MemType::type_byteArray)
      {
        json["length"] = static_cast<double>(m_entry->getLength());
      }
      json["baseIndex"] = static_cast<double>(m_entry->getBase());
      if (m_entry->isBoundToPointer())
      {
        QJsonArray offsets;
        for (int i = 0; i < m_entry->getPointerOffsets().size(); ++i)
        {
          std::stringstream ssOffset;
          ssOffset << std::hex << std::uppercase << m_entry->getPointerOffset(i);
          offsets.append(QString::fromStdString(ssOffset.str()));
        }
        json["pointerOffsets"] = offsets;
      }
    }
  }
}