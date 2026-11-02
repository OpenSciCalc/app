// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/fontdialogstyles.h"

#include <QFontDatabase>
#include <QFontDialog>
#include <QFontInfo>
#include <QHash>
#include <QIntValidator>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QSignalBlocker>
#include <QStringListModel>

#include <memory>
#include <utility>

namespace FontDialogStyles {

namespace {

struct StyleFlags {
    bool bold = false;
    bool italic = false;
};

// Uses the same names that QFontDialog gives a font without a style name.
// So when the dialog opens with a saved bold font, it selects our Bold entry.
QString styleName(StyleFlags flags)
{
    QFont font;
    font.setWeight(flags.bold ? QFont::Bold : QFont::Normal);
    font.setItalic(flags.italic);
    return QFontDatabase::styleString(font);
}

// DirectWrite makes a style bold only if it is lighter than DemiBold. So a
// DemiBold style already counts as Bold.
bool isBoldWeight(int weight)
{
    return weight >= QFont::DemiBold;
}

// Returns the non-italic style closest to Regular, which the font engine can
// make bold or italic. Returns an empty string if the family has none.
QString regularStyle(const QString& family)
{
    QString regular;
    int regularDistance = 0;
    for (const QString& style : QFontDatabase::styles(family)) {
        const int weight = QFontDatabase::weight(family, style);
        if (QFontDatabase::italic(family, style) || isBoldWeight(weight))
            continue;
        const int distance = qAbs(weight - QFont::Normal);
        if (regular.isEmpty() || distance < regularDistance) {
            regular = style;
            regularDistance = distance;
        }
    }
    return regular;
}

QList<std::pair<QString, StyleFlags>> synthesizedStyleFlags(const QString& family)
{
    QList<std::pair<QString, StyleFlags>> result;
    if (regularStyle(family).isEmpty())
        return result;

    const QStringList styles = QFontDatabase::styles(family);
    bool hasBold = false;
    bool hasItalic = false;
    bool hasBoldItalic = false;
    for (const QString& style : styles) {
        const bool bold = isBoldWeight(QFontDatabase::weight(family, style));
        if (QFontDatabase::italic(family, style))
            (bold ? hasBoldItalic : hasItalic) = true;
        else if (bold)
            hasBold = true;
    }

    const auto addUnless = [&](bool present, StyleFlags flags) {
        const QString name = styleName(flags);
        if (!present && !styles.contains(name))
            result.append({name, flags});
    };
    addUnless(hasBold, {true, false});
    addUnless(hasItalic, {false, true});
    addUnless(hasBoldItalic, {true, true});
    return result;
}

QString currentText(const QListView* list)
{
    const QModelIndex index = list->currentIndex();
    return index.isValid() ? index.data().toString() : QString();
}

struct DialogState {
    QFontDialog* dialog = nullptr;
    QListView* familyList = nullptr;
    QListView* styleList = nullptr;
    QListView* sizeList = nullptr;
    QLineEdit* sizeEdit = nullptr;
    QLineEdit* sampleEdit = nullptr;
    QHash<QString, StyleFlags> synthesized;
    QString regular;
    int size = 0;
    bool syncing = false;
};

QStringListModel* stringListModel(const QListView* list)
{
    return qobject_cast<QStringListModel*>(list->model());
}

void addSynthesizedStyles(DialogState& state)
{
    const QString family = currentText(state.familyList);
    state.synthesized.clear();
    state.regular = regularStyle(family);
    const QList<std::pair<QString, StyleFlags>> styles = synthesizedStyleFlags(family);
    if (styles.isEmpty())
        return;

    // QFontDialog can then also select the style it remembers from these rows.
    QStringListModel* model = stringListModel(state.styleList);
    const int firstRow = model->rowCount();
    model->insertRows(firstRow, styles.size());
    for (int i = 0; i < styles.size(); ++i) {
        model->setData(model->index(firstRow + i), styles.at(i).first);
        state.synthesized.insert(styles.at(i).first, styles.at(i).second);
    }
}

// QFontDialog builds its preview from the style name, and returns the preview
// font. For one of our added styles, it finds only the regular style, and its
// size list is empty. So we fix the preview and the size list after every
// change.
//
// When the dialog opens with one of our styles selected, QFontDialog also
// remembers that the style cannot be scaled freely. Then, after a change to a
// real style, it leaves the size box empty and uses the default font size. So
// we fix the size in that case too.
void syncSynthesizedStyle(DialogState& state)
{
    // The size box is empty in these cases, so remember the last size it
    // showed.
    if (state.sizeEdit->text().toInt() > 0)
        state.size = state.sizeEdit->text().toInt();
    if (state.syncing)
        return;
    const QString family = currentText(state.familyList);
    const QString style = currentText(state.styleList);
    const auto flags = state.synthesized.constFind(style);
    const bool synthesized = flags != state.synthesized.constEnd();
    if (!synthesized
        && (!state.sizeEdit->text().isEmpty() || !QFontDatabase::isSmoothlyScalable(family, style)))
        return;
    state.syncing = true;

    const QString face = synthesized ? state.regular : style;
    QStringList sizes;
    for (int size : QFontDatabase::pointSizes(family, face))
        sizes.append(QString::number(size));
    const int size = state.size;
    const QString sizeText = QString::number(size);

    QStringListModel* sizeModel = stringListModel(state.sizeList);
    if (sizeModel->stringList() != sizes)
        sizeModel->setStringList(sizes);
    {
        const QSignalBlocker blocker(state.sizeList);
        const int row = sizes.indexOf(sizeText);
        if (row >= 0)
            state.sizeList->setCurrentIndex(sizeModel->index(row));
        else
            state.sizeList->clearSelection();
    }
    if (state.sizeEdit->text() != sizeText)
        state.sizeEdit->setText(sizeText);

    const QFont sample = state.sampleEdit->font();
    QFont font = QFontDatabase::font(family, face, size);
    if (synthesized) {
        font.setStyleName(QString());
        if (flags->bold)
            font.setWeight(QFont::Bold);
        font.setItalic(flags->italic);
    }
    font.setStrikeOut(sample.strikeOut());
    font.setUnderline(sample.underline());
    if (font != sample) {
        state.sampleEdit->setFont(font);
        emit state.dialog->currentFontChanged(font);
    }
    state.syncing = false;
}

} // namespace

QStringList synthesizedStyles(const QString& family)
{
    QStringList result;
    for (const auto& style : synthesizedStyleFlags(family))
        result.append(style.first);
    return result;
}

void install(QFontDialog* dialog, const QFont& currentFont)
{
    auto state = std::make_shared<DialogState>();
    state->dialog = dialog;
    state->size = QFontInfo(currentFont).pointSize();

    // QFontDialog creates its family, style and size lists in this order.
    const QList<QListView*> lists = dialog->findChildren<QListView*>(Qt::FindDirectChildrenOnly);
    if (lists.size() == 3) {
        state->familyList = lists.at(0);
        state->styleList = lists.at(1);
        state->sizeList = lists.at(2);
    }
    for (QLineEdit* edit : dialog->findChildren<QLineEdit*>(Qt::FindDirectChildrenOnly)) {
        if (qobject_cast<const QIntValidator*>(edit->validator()))
            state->sizeEdit = edit;
    }
    state->sampleEdit = dialog->findChild<QLineEdit*>(QStringLiteral("qt_fontDialog_sampleEdit"));

    // If the dialog does not have the parts we expect, leave it unchanged.
    const bool recognized = state->familyList && state->sizeEdit && state->sampleEdit
        && stringListModel(state->styleList) && stringListModel(state->sizeList);
    if (recognized) {
        QObject::connect(stringListModel(state->styleList), &QAbstractItemModel::modelReset,
                         dialog, [state]() { addSynthesizedStyles(*state); });
        QObject::connect(dialog, &QFontDialog::currentFontChanged,
                         dialog, [state]() { syncSynthesizedStyle(*state); });
        // Going from Regular to an added style does not change the font Qt
        // builds, so the dialog does not report a font change.
        QObject::connect(state->styleList->selectionModel(), &QItemSelectionModel::currentChanged,
                         dialog, [state]() { syncSynthesizedStyle(*state); });
    }
    dialog->setCurrentFont(currentFont);
}

} // namespace FontDialogStyles
