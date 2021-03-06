#include "BFRESNode.h"

#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QStandardItem>
#include <QTableView>

#include "Common.h"
#include "Nodes/Archives/BFRESGroupNode.h"
#include "QtUtils/DynamicStandardItem.h"

BFRESNode::BFRESNode(std::shared_ptr<BFRES> bfres, QObject* parent) : Node(parent), m_bfres(bfres)
{
}

ResultCode BFRESNode::LoadFileTreeArea()
{
  emit NewStatus(ResultCode::UpdateStatusBar, "Loading file tree...");

  ResultCode res = m_bfres->ReadHeader();
  if (res != ResultCode::Success)
  {
    NewStatus(res);
    return res;
  }
  m_bfres_header = m_bfres->GetHeader();

  QStandardItemModel* file_tree_model = new QStandardItemModel(0, 1);

  DynamicStandardItem* root_item = MakeItem();
  file_tree_model->appendRow(root_item);

  m_tree_view = new QTreeView;
  // stretch out table to fit space
  m_tree_view->header()->hide();
  //  m_tree_view->setItemDelegate(new DynamicItemDelegate(DynamicItemDelegate::DelegateInfo()));
  m_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree_view, &QTreeView::customContextMenuRequested, this,
          &BFRESNode::HandleTreeCustomContextMenuRequest);

  m_tree_view->setModel(file_tree_model);
  // TODO: this only works with mouse clicking, i still have to figure out
  // keyboard arrow navigation
  connect(m_tree_view, &QTreeView::clicked, this, &BFRESNode::HandleFileTreeClick);

  QVBoxLayout* file_tree_layout = new QVBoxLayout();
  file_tree_layout->addWidget(m_tree_view);

  QScrollArea* file_tree_container = new QScrollArea();
  file_tree_container->setLayout(file_tree_layout);

  emit NewFileTreeArea(file_tree_container);

  emit NewStatus(res);
  return res;
}

DynamicStandardItem* BFRESNode::MakeItem()
{
  emit ConnectNode(this);

  DynamicStandardItem* bfres_item = new DynamicStandardItem();
  bfres_item->setData(QString(m_bfres->GetName() + " (BFRES)"), Qt::DisplayRole);
  // Store the current instance in a QVariant as an upcasted Node pointer.
  // Will be downcasted back to a BFRES Group Node later.
  bfres_item->setData(QVariant::fromValue<Node*>(static_cast<Node*>(this)), Qt::UserRole + 1);
  m_bfres->ReadDictionaries();

  // Allocate a new Node derived object.
  BFRESGroupNode<FMDL>* fmdl_group_node = new BFRESGroupNode<FMDL>(m_bfres->GetFMDLDictionary());
  connect(fmdl_group_node, &BFRESGroupNode<FMDL>::ConnectNode, this, &BFRESNode::ConnectNode);
  bfres_item->appendRow(fmdl_group_node->MakeItem());

  BFRESGroupNode<FTEX>* ftex_group_node = new BFRESGroupNode<FTEX>(m_bfres->GetFTEXDictionary());
  connect(ftex_group_node, &BFRESGroupNode<FTEX>::ConnectNode, this, &BFRESNode::ConnectNode);
  bfres_item->appendRow(ftex_group_node->MakeItem());

  return bfres_item;
}

ResultCode BFRESNode::LoadAttributeArea()
{
  emit NewStatus(ResultCode::UpdateStatusBar, "Loading file info...");

  QStandardItemModel* header_attributes_model = new QStandardItemModel();
  //  m_delegate_group = DynamicItemDelegate::DelegateInfo();

  int row = 0;

  QVector<Node::AttributeTableRow> test{
      {"Magic File Identifier", m_bfres_header.magic},
      {"Unknown A", QString("0x" + QString::number(m_bfres_header.unknown_a, 16))},
      {"Unknown B", QString("0x" + QString::number(m_bfres_header.unknown_b, 16))},
      {"Unknown C", QString("0x" + QString::number(m_bfres_header.unknown_c, 16))},
      {"Unknown D", QString("0x" + QString::number(m_bfres_header.unknown_d, 16))},
      {"Unknown E", QString("0x" + QString::number(m_bfres_header.unknown_e, 16))},
      {"Unknown F", QString("0x" + QString::number(m_bfres_header.unknown_f, 16))},
      {"Endianess", QVariant::fromValue(Node::AttributeComboBoxData{[this] {
         QVector<QString> endian_names_vector;
         auto endian_name_map = m_bfres->GetEndianNames();
         for (auto const& endian_name : endian_name_map)
           endian_names_vector << endian_name.second;

         return Node::AttributeComboBoxData(
             endian_names_vector, std::distance(endian_name_map.begin(),
                                                endian_name_map.find(static_cast<BFRES::Endianness>(
                                                    m_bfres_header.bom))));
       }()})}};

  // TODO: Add proper entries for the unknowns, as some are found now.

  // Length
  header_attributes_model->setItem(row, 0, new QStandardItem("Length"));
  header_attributes_model->setItem(row, 1,
                                   new QStandardItem(QString::number(m_bfres_header.length)));
  //  m_delegate_group.spin_box_delegates << row;
  ++row;

  // Alignment
  header_attributes_model->setItem(row, 0, new QStandardItem("Alignment"));
  header_attributes_model->setItem(
      row, 1, new QStandardItem("0x" + QString::number(m_bfres_header.alignment, 16)));
  ++row;

  // File name offset
  header_attributes_model->setItem(row, 0, new QStandardItem("File Name Offset"));
  header_attributes_model->setItem(
      row, 1, new QStandardItem("0x" + QString::number(m_bfres_header.file_name_offset, 16)));
  // TODO: hex spinbox delegate maybe?
  ++row;

  // String table length
  header_attributes_model->setItem(row, 0, new QStandardItem("String Table Length"));
  header_attributes_model->setItem(
      row, 1, new QStandardItem("0x" + QString::number(m_bfres_header.string_table_length, 16)));
  ++row;

  // String table offset
  header_attributes_model->setItem(row, 0, new QStandardItem("String Table Offset"));
  header_attributes_model->setItem(
      row, 1, new QStandardItem("0x" + QString::number(m_bfres_header.string_table_offset, 16)));
  ++row;

  // Preserve the current row so it can be revisited later.
  int original_row = row;

  // Group Offsets
  for (qint32 i = 0; i < m_bfres_header.file_offsets.size(); ++i)
  {
    header_attributes_model->setItem(row, 0,
                                     new QStandardItem("Group " + QString::number(i) + " Offset"));
    header_attributes_model->setItem(
        row, 1, new QStandardItem("0x" + QString::number(m_bfres_header.file_offsets[i], 16)));
    // Skip ahead 2 rows instead of 1 to make room for the number of files items.
    row += 2;
  }
  // Go back to the start of the group items.
  row = original_row;
  // Skip one ahead.
  row++;

  // Group File Counts
  for (qint32 i = 0; i < m_bfres_header.file_counts.size(); ++i)
  {
    header_attributes_model->setItem(
        row, 0, new QStandardItem("Number of files in group " + QString::number(i)));
    header_attributes_model->setItem(
        row, 1, new QStandardItem(QString::number(m_bfres_header.file_counts[i])));
    //    m_delegate_group.spin_box_delegates << row;
    // Skip ahead 2 to get around the file group offsets.
    row += 2;
  }
  // The file group count loop will leave one empty space at the end.
  row--;

  header_attributes_model->setRowCount(row);
  header_attributes_model->setColumnCount(2);

  connect(header_attributes_model, &QStandardItemModel::itemChanged, this,
          &BFRESNode::HandleAttributeItemChange);

  emit NewAttributeArea(
      MakeAttributeSection(test, [this]() { m_bfres->SetHeader(m_bfres_header); }));
  return ResultCode::Success;
}

void BFRESNode::HandleAttributeItemChange(QStandardItem* item)
{
  // Colum 1 is the only editable row
  if (item->column() != 1)
    return;
#ifdef DEBUG
  qDebug() << "Item changed. Row " << item->row() << " column " << item->column();
#endif

  DynamicStandardItem* custom_item = dynamic_cast<DynamicStandardItem*>(item);
  if (custom_item)
    custom_item->ExecuteFunction();
}
