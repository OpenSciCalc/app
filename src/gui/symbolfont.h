// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_SYMBOLFONT_H
#define GUI_SYMBOLFONT_H

#include <QFont>
#include <QString>

// When a glyph is missing, Qt builds its font fallback list by loading every
// installed family, which takes most of a second on systems with many fonts.
// Text with symbols the default UI font lacks uses a font that has them.
namespace SymbolFont {

// Returns whether the text has a symbol that Segoe UI lacks on Windows.
inline bool isNeededFor(const QString& text)
{
#ifdef Q_OS_WIN
    for (const QChar c : text) {
        switch (c.unicode()) {
        case 0x221B: // ∛
        case 0x2220: // ∠
        case 0x2327: // ⌧
        case 0x232B: // ⌫
            return true;
        default:
            break;
        }
    }
#else
    Q_UNUSED(text);
#endif
    return false;
}

// Returns the font with only its family changed. A default-constructed font
// changes only the family and inherits everything else from its widget.
inline QFont withSymbolFamily(QFont font = QFont())
{
    font.setFamily(QStringLiteral("Segoe UI Symbol"));
    return font;
}

inline QFont forText(const QFont& base, const QString& text)
{
    return isNeededFor(text) ? withSymbolFamily(base) : base;
}

} // namespace SymbolFont

#endif // GUI_SYMBOLFONT_H
