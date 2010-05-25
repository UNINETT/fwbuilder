/*

                          Firewall Builder

                 Copyright (C) 2009 NetCitadel, LLC

  Author:  Illiya Yalovoy <yalovoy@gmail.com>

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../../config.h"
#include "global.h"
#include "utils.h"

#include "FWBSettings.h"

#include "RuleSetViewDelegate.h"
#include "RuleNode.h"
#include "ColDesc.h"
#include "FWObjectSelectionModel.h"

#include "fwbuilder/Firewall.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/NAT.h"
#include "fwbuilder/Routing.h"
#include "fwbuilder/RuleElement.h"
#include "fwbuilder/Interface.h"

#include <string>

#include <QtDebug>
#include <QtGui>
#include<QStringList>

using namespace libfwbuilder;
using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// RuleSetViewDelegate
//////////////////////////////////////////////////////////////////////////////////////////////////////////

RuleSetViewDelegate::RuleSetViewDelegate(QObject *parent, FWObjectSelectionModel *selectionModel) : QItemDelegate(parent)
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::RuleSetViewDelegate";
    this->sectionModel = selectionModel;
}
void RuleSetViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paint";

    QStyleOptionViewItem newOpt = option;
    QFont font = st->getRulesFont();
    newOpt.font = font;

    RuleNode * node;
    if (index.isValid())
    {
        node = static_cast<RuleNode *>(index.internalPointer());
    }
    else
    {
        QItemDelegate::paint(painter, newOpt, index);
        return;
    }

    painter->save();
    painter->setFont(font);
    if (node->type == RuleNode::Group)
    {
        paintGroup(painter, newOpt, index, node);
    }
    else if (index.column() == 0)
    {
        paintRowHeader(painter, newOpt, index, node);
    }
    else
    {
        paintRule(painter, newOpt, index, node);
    }
    painter->restore();
}

void RuleSetViewDelegate::paintRowHeader (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, RuleNode * node) const
{
    Q_UNUSED(node);
    QStyleOptionViewItem newOpt = option;
    newOpt.displayAlignment = Qt::AlignRight;
    QItemDelegate::paint(painter, newOpt, index);
    painter->setPen( QColor("lightgray") );
    painter->drawRect(option.rect);
}

void RuleSetViewDelegate::paintGroup(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, RuleNode * node) const
{
    Q_UNUSED(node);
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintGroup";
    painter->fillRect(option.rect,QColor("lightgray"));
    QItemDelegate::paint(painter, option, index);
}

void RuleSetViewDelegate::drawIcons(QPainter *painter, QRect rect,
                                    const QStringList &icons) const
{
    int x = rect.left();
    int y = rect.top();
    int iconWidth = 0;

    for (int i=0; i<icons.size(); i++)
    {
        QPixmap pm;
        QString icon = calculateIconName(icons[i], false);
        LoadPixmap(icon, pm);
        painter->drawPixmap(x, y, pm);
        iconWidth = pm.width();
        x += iconWidth + ICON_TEXT_GAP;
    }
}

void RuleSetViewDelegate::drawIconAndText(QPainter *painter,
                                          QRect rect,
                                          QString icon,
                                          QString text,
                                          bool negation) const
{
    int x = rect.left();
    int y = rect.top();
    int iconWidth = 0;

    if (!icon.isEmpty() && st->getShowIconsInRules())
    {
        QPixmap pm;
        if (!icon.isEmpty()) LoadPixmap(calculateIconName(icon, negation), pm);
        painter->drawPixmap(x,y,pm);
        iconWidth = pm.width();
        x += iconWidth + ICON_TEXT_GAP;
    }

    if (!text.isEmpty())
    {
        painter->drawText( x, y , rect.width() - iconWidth - ICON_TEXT_GAP, rect.height(), Qt::AlignLeft|Qt::AlignVCenter, text );
    }
}

void RuleSetViewDelegate::drawSelectedFocus(QPainter *painter, const QStyleOptionViewItem &option,QRect &rect) const
{
    if (option.state & QStyle::State_HasFocus)
    {
        painter->fillRect(rect, option.palette.brush( QPalette::Highlight ));
        painter->setPen( option.palette.highlightedText().color() );
    }
    else
    {
        painter->setPen( option.palette.text().color() );
    }
}

void RuleSetViewDelegate::paintRule(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index,
                                    RuleNode * node) const
{
    QVariant v = index.data(Qt::DisplayRole);
    if (!v.isValid()) return;

    // if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintRule"
    //                        << v;

    if (node != 0)
    {
        FWOptions *ropt = node->rule->getOptionsObject();
        QString color = ropt->getStr("color").c_str();
        if (!color.isEmpty())
            painter->fillRect(option.rect,QColor(color));
    }
    ColDesc colDesc = index.data(Qt::UserRole).value<ColDesc>();
    switch (colDesc.type)
    {
        case ColDesc::Object :
            paintObject(painter, option, v);
            break;
        case ColDesc::Direction :
            paintDirection(painter, option, v);
            break;
        case ColDesc::Action :
            paintAction(painter, option, v);
            break;
        case ColDesc::Time :
            paintObject(painter, option, v);
            break;
        case ColDesc::Options :
            paintOptions(painter, option, v);
            break;
        case ColDesc::Comment :
            paintComment(painter, option, v);
            break;
        case ColDesc::Metric :
            paintMetric(painter, option, v);
            break;
        default:
            QItemDelegate::paint(painter, option, index);
    }
    painter->setPen( QColor("lightgray") );
    painter->drawRect(option.rect);
}

void RuleSetViewDelegate::paintDirection(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintDirection";
    DrawingContext ctx = initContext(option.rect);
    QString dir = v.value<QString>();
    drawSelectedFocus(painter, option, ctx.objectRect);
    QString text = (st->getShowDirectionText())?dir:"";
    drawIconAndText(painter, ctx.drawRect, dir,text);
}

void RuleSetViewDelegate::paintAction(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintAction";
    DrawingContext ctx = initContext(option.rect);
    ActionDesc actionDesc = v.value<ActionDesc>();
    drawSelectedFocus(painter, option, ctx.objectRect);
    QString text = constructActionText(actionDesc);
    drawIconAndText(painter, ctx.drawRect, actionDesc.name, text);
}

void RuleSetViewDelegate::paintOptions(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintOptions";
    DrawingContext ctx = initContext(option.rect);
    drawSelectedFocus(painter, option, ctx.objectRect);
    QStringList icons;
    icons = v.value<QStringList>();
    QStringList text;

    if (st->getShowIconsInRules())
    {
        drawIcons(painter, ctx.drawRect, icons);
    } else
    {
        for (int i=0; i<icons.size(); i++)
        {
            if (icons[i].contains("Log")) text.push_back(tr("log"));
            if (icons[i].contains("Options")) text.push_back(tr("(options)"));
        }
        drawIconAndText(painter, ctx.drawRect, "", text.join(", "), false);
    }
}


void RuleSetViewDelegate::paintComment(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintComment";
    DrawingContext ctx = initContext(option.rect, true);
    QString comment = v.value<QString>();
    drawSelectedFocus(painter, option, ctx.objectRect);
    painter->drawText( ctx.drawRect, Qt::AlignLeft|Qt::AlignTop, comment);
}

void RuleSetViewDelegate::paintMetric(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintMetric";
    DrawingContext ctx = initContext(option.rect, true);
    QString metric = v.value<QString>();
    drawSelectedFocus(painter, option, ctx.objectRect);
    drawIconAndText(painter, ctx.drawRect, QString(), metric);
}

void RuleSetViewDelegate::paintObject(QPainter *painter, const QStyleOptionViewItem &option, const QVariant &v) const
{
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::paintObject";
    RuleElement *re = (RuleElement *)v.value<void *>();

    if (re==NULL) return;

    DrawingContext ctx = initContext(option.rect, true);

    int y = ctx.objectRect.top();

    for (FWObject::iterator i=re->begin(); i!=re->end(); i++)
    {
        FWObject *o1= *i;
        if (FWReference::cast(o1)!=NULL)
            o1=FWReference::cast(o1)->getPointer();
        if (o1==NULL)
            continue ;

        QRect itemRect = QRect(ctx.objectRect.left(), y, ctx.objectRect.width(), ctx.itemHeight);

        if (sectionModel->selectedObject == o1)
        {
            drawSelectedFocus(painter, option, itemRect);
        }
        else
        {
            painter->setPen( option.palette.text().color() );
        }

        QString icon;
        if (!re->isAny()) icon = QString(o1->getTypeName().c_str()); // + "/icon";
        QString text = objectText(re, o1);

        drawIconAndText(painter,
                        itemRect.adjusted(HORIZONTAL_MARGIN, VERTICAL_MARGIN, -HORIZONTAL_MARGIN, -VERTICAL_MARGIN),
                        icon, text, re->getNeg());

        if ((sectionModel->selectedObject == o1) && !(option.state & QStyle::State_HasFocus) && !re->isAny())
        {
            painter->setPen( QColor("red") );
            painter->drawRect(itemRect.left()+1, itemRect.top()+1, itemRect.width()-2, itemRect.height()-2);
        }

        y += ctx.itemHeight;
    }
}

QSize RuleSetViewDelegate::drawIconInRule(QPainter *p, int x, int y, QString name, bool neg) const
{
    if (!st->getShowIconsInRules()) return QSize();

    QPixmap pm;
    if (FWBSettings::SIZE16X16 == st->getIconsInRulesSize())
    {
        if (!neg)
        {
            pm = getPixmap(name, Tree);
        }
        else
        {
            pm = getPixmap(name, NegTree);
        }
    }

    if (FWBSettings::SIZE25X25 == st->getIconsInRulesSize())
    {
        if (!neg)
        {
            pm = getPixmap(name, Normal);
        }
        else
        {
            pm = getPixmap(name, Neg);
        }
    }

    p->drawPixmap( x, y + RULE_ITEM_GAP/2, pm );

    return pm.size();
}

QPixmap RuleSetViewDelegate::getPixmap(QString name, PixmapAttr pmattr) const
{
    string icn = "icon";
    if (pmattr == Neg)  icn="icon-neg";
    if (pmattr == Ref)  icn="icon-ref";
    if (pmattr == Tree) icn="icon-tree";
    if (pmattr == NegTree) icn="icon-neg-tree";

    QString icn_file = ":/Icons/" + name + "/" + icn.c_str();
    QPixmap pm;
    LoadPixmap(icn_file, pm);

    return pm;
}

QSize RuleSetViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyleOptionViewItem newOpt = option;
    QFont font = st->getRulesFont();
    newOpt.font = font;

    QSize res;
    RuleNode * node;
    if (index.isValid())
    {
        node = static_cast<RuleNode *>(index.internalPointer());
    }
    else
    {
        return QItemDelegate::sizeHint(newOpt, index);
    }

    if (node->type == RuleNode::Rule)
    {
        if (node->sizes[index.column()].isValid())
        {
            return node->sizes[index.column()];
        }
        res = calculateCellSizeForRule(newOpt, index, node) + QSize(1,1);
        node->sizes[index.column()] = res;
        return res;
    }

    //Fix for older Qt versions where width of spanned column is taken into accoun
    res = QItemDelegate::sizeHint(newOpt, index);
    res.setWidth(20);
    return res;
}

int RuleSetViewDelegate::getItemHeight(QString s, int flag, bool text)
{
    QSize iconSize = getIconSize();
    QSize textSize = text?getTextSize(s,flag):QSize(0,0);

    return qMax(iconSize.height(), textSize.height()) + 2*VERTICAL_MARGIN;
}

QSize RuleSetViewDelegate::getIconSize()
{
    if (st->getShowIconsInRules())
    {
        if (FWBSettings::SIZE16X16 == st->getIconsInRulesSize()) return QSize(16,16);
        if (FWBSettings::SIZE25X25 == st->getIconsInRulesSize()) return QSize(25,25);
    }
    return QSize();
}

QSize RuleSetViewDelegate::getTextSize(QString s, int flag)
{
    QFontMetrics fontMetrics(st->getRulesFont());
    return fontMetrics.size(flag,s);
}

QSize RuleSetViewDelegate::calculateCellSizeForRule(const QStyleOptionViewItem & option, const QModelIndex & index, RuleNode * node ) const
{
    Q_UNUSED(option);
    Q_UNUSED(node);
    //if (fwbdebug) qDebug() << "RuleSetViewDelegate::calculateCellSizeForRule";

    QSize iconSize = getIconSize();
    int itemHeight = getItemHeight();

    QSize result = QSize(50,itemHeight);

    ColDesc colDesc = index.data(Qt::UserRole).value<ColDesc>();

    QSize calculated;

    switch (colDesc.type)
    {
        case ColDesc::Object :
            calculated = calculateCellSizeForObject(index);
            break;
        case ColDesc::Time :
            calculated = calculateCellSizeForObject(index);
            break;
        case ColDesc::Comment :
            calculated = calculateCellSizeForComment(index);
            break;
        case ColDesc::Action :
            calculated = calculateCellSizeForIconAndText(index);
            break;
        case ColDesc::Direction :
            calculated = calculateCellSizeForIconAndText(index);
            break;
        default :
            calculated = QSize(0,0);
    }

    result = result.expandedTo(calculated);

    return result;
}

QSize RuleSetViewDelegate::calculateCellSizeForComment(const QModelIndex & index) const
{
    QString text = index.data(Qt::DisplayRole).value<QString>();
    if (text.isNull() && text.isEmpty()) return QSize(0,0);

    if (st->getClipComment()) return QSize(0,0);

    QSize res = getTextSize(text,0);
    return res;
}

QSize RuleSetViewDelegate::calculateCellSizeForObject(const QModelIndex & index) const
{
    RuleElement *re = (RuleElement *)index.data(Qt::DisplayRole).value<void *>();
    if (re == 0) return QSize(0,0);

    int itemHeight = getItemHeight();
    QSize iconSize = getIconSize();

    int h = 0;
    int w = 0;

    for (FWObject::iterator j=re->begin(); j!=re->end(); j++)
    {
        FWObject *o1= *j;
        FWObject *o2 = o1;
        string o1ref = "";
        if (FWReference::cast(o1)!=NULL)
        {
            o1ref = FWReference::cast(o1)->getPointerId();
            o2=FWReference::cast(o1)->getPointer();
        }
        if (o2!=NULL)
        {
            QString ot = objectText(re,o2);

            QSize size = getTextSize(ot,Qt::TextSingleLine);

            h += itemHeight;
            w = qMax(w,iconSize.width() + size.width()+ICON_TEXT_GAP);
        }
    }
    QSize res = QSize(w+HORIZONTAL_MARGIN*2,h);
    QModelIndex idx = index;
    return res;
}

QString RuleSetViewDelegate::constructActionText(ActionDesc &actionDesc) const
{
    QString text;
    if (st->getShowDirectionText())
    {
        text = actionDesc.displayName + (actionDesc.argument.isEmpty()?"":(":"+actionDesc.argument));
    } else if (!actionDesc.argument.isEmpty())
    {
        text = actionDesc.argument;
    }

    if (text.length() > 20)
        text = text.left(17) + "...";

    return text;
}

QSize RuleSetViewDelegate::calculateCellSizeForIconAndText(const QModelIndex & index) const
{
    QVariant v = index.data(Qt::DisplayRole);
    ActionDesc actionDesc = v.value<ActionDesc>();
    QString text = constructActionText(actionDesc);

    if (text == "Undefined") text = "Both";

    QSize iconSize = getIconSize();
    QSize textSize = getTextSize(text,Qt::TextSingleLine);

    int h = qMax(iconSize.height(), textSize.height());
    int w = iconSize.width() + ICON_TEXT_GAP + textSize.width() + HORIZONTAL_MARGIN*2;

    return QSize(w, h);
}


QString RuleSetViewDelegate::objectText(RuleElement *re,FWObject *obj) const
{
    QString any_object_name = QString(tr("Any"));
    if (re->isAny())
    {
        if (RuleElementTSrc::isA(re) ||
            RuleElementTDst::isA(re) ||
            RuleElementTSrv::isA(re)) return QString(tr("Original"));
        if (RuleElementRDst::isA(re)) return QString(tr("Default"));
        if (RuleElementRGtw::isA(re) ||
            RuleElementRItf::isA(re))  return QString("");
        if (RuleElementItf::isA(re))   return QString(tr("All"));
        return any_object_name;
    }

    if (Interface::isA(obj))
    {
        QString lbl= Interface::cast(obj)->getLabel().c_str();
        if ( !lbl.isEmpty() ) return lbl;
    }
    if (obj->getName() == "Any") return any_object_name;
    else return QString::fromUtf8(obj->getName().c_str());
}

DrawingContext RuleSetViewDelegate::initContext(QRect rect, bool useEnireSpace) const
{
    DrawingContext ctx;

    ctx.iconSize = getIconSize();
    ctx.itemHeight = getItemHeight();
    ctx.objectRect = QRect(rect.left()+1, rect.top()+1, rect.width()-1, useEnireSpace?rect.height()-1:ctx.itemHeight);
    ctx.drawRect = ctx.objectRect.adjusted(HORIZONTAL_MARGIN, VERTICAL_MARGIN, -HORIZONTAL_MARGIN, -VERTICAL_MARGIN);

    return ctx;
}
