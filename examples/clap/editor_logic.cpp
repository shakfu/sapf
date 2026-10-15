#include "editor_logic.hpp"

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

namespace {

bool IsBlank(const std::string& s)
{
	return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

// [begin, end) of the line containing pos, excluding its newline.
std::pair<size_t, size_t> LineAt(const std::string& text, size_t pos)
{
	size_t begin = 0;
	if (pos > 0) {
		size_t newline = text.rfind('\n', pos - 1);
		if (newline != std::string::npos) begin = newline + 1;
	}
	size_t end = text.find('\n', pos);
	return {begin, end == std::string::npos ? text.size() : end};
}

// [open, close] offsets of each top-level ( ), [ ] or { } group, skipping ';' comments and "..." strings.
std::vector<std::pair<size_t, size_t>> TopLevelBlocks(const std::string& text)
{
	std::vector<std::pair<size_t, size_t>> blocks;
	int depth = 0;
	size_t open = 0;
	bool inString = false;
	for (size_t i = 0; i < text.size(); ++i) {
		const char c = text[i];
		if (inString) {
			if (c == '\\') {
				++i;
			} else if (c == '"') {
				if (i + 1 < text.size() && text[i + 1] == '"') ++i; // "" is a literal quote
				else inString = false;
			}
		} else if (c == ';') {
			while (i + 1 < text.size() && text[i + 1] != '\n') ++i;
		} else if (c == '"') {
			inString = true;
		} else if (c == '(' || c == '[' || c == '{') {
			if (depth++ == 0) open = i;
		} else if ((c == ')' || c == ']' || c == '}') && depth > 0) {
			if (--depth == 0) blocks.emplace_back(open, i);
		}
	}
	return blocks;
}

} // namespace

std::string SelectEvalCode(const std::string& text, int selStart, int selEnd, int cursor)
{
	const int size = (int)text.size();
	auto clamp = [size](int v) { return (size_t)std::clamp(v, 0, size); };

	if (selStart != selEnd) {
		size_t a = clamp(std::min(selStart, selEnd)), b = clamp(std::max(selStart, selEnd));
		std::string code = text.substr(a, b - a);
		return IsBlank(code) ? std::string() : code;
	}

	auto [begin, end] = LineAt(text, clamp(cursor));
	if (IsBlank(text.substr(begin, end - begin)) && begin > 0) {
		std::tie(begin, end) = LineAt(text, begin - 1);
	}
	// A line inside a multi-line bracket group evaluates the whole group, through the ends of its lines.
	for (auto [open, close] : TopLevelBlocks(text)) {
		const bool multiLine = text.find('\n', open) < close;
		if (multiLine && open < end && close >= begin) {
			begin = LineAt(text, open).first;
			end = LineAt(text, close).second;
			break;
		}
	}
	std::string code = text.substr(begin, end - begin);
	return IsBlank(code) ? std::string() : code;
}
