#pragma once

#include <string>

// Code to evaluate for an eval request. Offsets are byte positions in `text`.
// Returns the selection if non-empty, otherwise the caret's line; a blank caret line
// (e.g. after the final newline) selects the line above. If that line is part of a
// multi-line top-level ( ), [ ] or { } group, returns the whole group. Returns "" for blank code.
std::string SelectEvalCode(const std::string& text, int selStart, int selEnd, int cursor);
