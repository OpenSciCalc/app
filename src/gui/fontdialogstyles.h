// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_FONTDIALOGSTYLES_H
#define GUI_FONTDIALOGSTYLES_H

#include <QFont>
#include <QStringList>

class QFontDialog;

// Qt's font dialog lists only the styles that a font really has. So a font
// with only a regular style, such as Lucida Console, shows no Bold. But Qt
// can still draw it bold or italic. Before Qt 6.8, Windows listed these
// styles.
namespace FontDialogStyles {

// Returns the Bold, Italic and Bold Italic styles that the family does not
// have, but that the font engine can draw from its regular style.
QStringList synthesizedStyles(const QString& family);

// Adds these styles to the style list of Qt's own dialog, and makes the
// dialog return a font that uses them. Also sets the dialog's current font,
// because the style list is filled from it.
void install(QFontDialog* dialog, const QFont& currentFont);

} // namespace FontDialogStyles

#endif // GUI_FONTDIALOGSTYLES_H
