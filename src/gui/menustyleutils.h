// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPEEDCRUNCH_MENUSTYLEUTILS_H
#define SPEEDCRUNCH_MENUSTYLEUTILS_H

#include <QColor>
#include <QSize>

class QMenu;
class QIcon;
class QComboBox;
class QPalette;
class QStyleOption;
class QWidget;

namespace MenuStyle {
void install();
void refresh();
void apply(QWidget* widget);
QSize themedItemSize(const QStyleOption* option, const QSize& contents, const QSize& size,
                     const QWidget* widget);
QIcon themedIcon(const QIcon& source, const QPalette& palette);
QPalette systemPalette(const QWidget* widget);
QPalette systemComboPopupPalette(const QComboBox* combo);
void setThemeColors(QMenu* menu, const QColor& background, const QColor& foreground,
                    const QColor& selectedBackground, const QColor& selectedForeground);
}

#endif
