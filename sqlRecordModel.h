//#include "sqlDatahandler.h"
#include <QAbstractListModel>

#ifndef SQLRECORDMODEL_H
#define SQLRECORDMODEL_H

class SqlRecordModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum RecordRoles {
        IdRole = Qt::UserRole + 1, GbRole, DateRole, SupplierRole, ProductRole,
        SizeRole, PriceRole, QtyRole, GonggaRole, BugaRole, HapgyeRole,
        IpD1Role, IpA1Role, IpD2Role, IpA2Role, IpD3Role, IpA3Role, MijiRole, MisuRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_data.size(); }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_data.size()) return QVariant();
        const QVariantMap &map = m_data[index.row()].toMap();


        switch (role) {
        case IdRole: return map["id"]; case GbRole: return map["gb"];
        case DateRole: return map["date"]; case SupplierRole: return map["supplier"];
        case ProductRole: return map["product"]; case SizeRole: return map["size"];
        case PriceRole: return map["price"]; case QtyRole: return map["quantity"];
        case GonggaRole: return map["gongga"]; case BugaRole: return map["buga"];
        case HapgyeRole: return map["hapgye"]; case IpD1Role: return map["ipD1"];
        case IpA1Role: return map["ipA1"]; case IpD2Role: return map["ipD2"];
        case IpA2Role: return map["ipA2"]; case IpD3Role: return map["ipD3"];
        case IpA3Role: return map["ipA3"]; case MijiRole: return map["miji"];
        case MisuRole: return map["misu"];
        default: return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {IdRole, "id"}, {GbRole, "gb"}, {DateRole, "tr_date"},
            {SupplierRole, "supplier"}, {ProductRole, "product"}, {SizeRole, "size"},
            {PriceRole, "price"}, {QtyRole, "quantity"}, {GonggaRole, "gongga"},
            {BugaRole, "buga"}, {HapgyeRole, "hapgye"}, {MijiRole, "miji"}, {MisuRole, "misu"}

        };
    }


    void setRecords(const QVariantList &newList) {
        beginResetModel();
        m_data = newList;
        endResetModel();
    }

private:
    QVariantList m_data;
};

#endif // SQLRECORDMODEL_H
