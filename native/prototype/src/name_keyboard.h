#pragma once
namespace moss {
// The same character panel is used for every editable object name.
constexpr char NameCharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";
constexpr int NameLimit = 24, NameColumns = 10;
constexpr int NameDelete = sizeof(NameCharacters) - 1;
constexpr int NameSave = NameDelete + 1, NameKeyCount = NameSave + 1;
} // namespace moss
