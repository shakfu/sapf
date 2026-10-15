#include <gtest/gtest.h>

#include "editor_logic.hpp"

TEST(SelectEvalCode, SelectionWins) {
    EXPECT_EQ(SelectEvalCode("1 2 +\n3 4 *\n", 2, 9, 9), "2 +\n3 4");
    EXPECT_EQ(SelectEvalCode("1 2 +\n3 4 *\n", 9, 2, 2), "2 +\n3 4"); // reversed selection
}

TEST(SelectEvalCode, CaretLine) {
    const std::string text = "1 2 +\n3 4 *\n5 6 -";
    EXPECT_EQ(SelectEvalCode(text, 0, 0, 0), "1 2 +");
    EXPECT_EQ(SelectEvalCode(text, 8, 8, 8), "3 4 *");
    EXPECT_EQ(SelectEvalCode(text, 11, 11, 11), "3 4 *"); // caret at end of line, before '\n'
    EXPECT_EQ(SelectEvalCode(text, 17, 17, 17), "5 6 -"); // caret at end of text, no trailing newline
}

TEST(SelectEvalCode, BlankLineUsesLineAbove) {
    EXPECT_EQ(SelectEvalCode("440 0 sinosc .1 * play\n", 23, 23, 23), "440 0 sinosc .1 * play");
    EXPECT_EQ(SelectEvalCode("a\n   \nb", 4, 4, 4), "a");
}

TEST(SelectEvalCode, BlankCodeIsEmpty) {
    EXPECT_EQ(SelectEvalCode("", 0, 0, 0), "");
    EXPECT_EQ(SelectEvalCode("\n\n", 1, 1, 1), "");
    EXPECT_EQ(SelectEvalCode("1 2 +\n", 5, 6, 6), ""); // selection is only a newline
}

TEST(SelectEvalCode, OutOfRangeOffsetsClamp) {
    EXPECT_EQ(SelectEvalCode("1 2 +", 0, 0, 99), "1 2 +");
    EXPECT_EQ(SelectEvalCode("1 2 +", -5, 99, 0), "1 2 +");
}

namespace {
const std::string kBlock =
    "(\n"
    "    [.4 [8 7.23]]   ; LFO freq (Hz)\n"
    "    0 lfsaw\n"
    "    play\n"
    ")\n";
}

TEST(SelectEvalCode, BlankLineAfterBlockEvaluatesBlock) {
    // Caret on the empty last line, as in the dev-host session: previously sent only ")".
    EXPECT_EQ(SelectEvalCode(kBlock, (int)kBlock.size(), (int)kBlock.size(), (int)kBlock.size()),
              kBlock.substr(0, kBlock.size() - 1));
}

TEST(SelectEvalCode, AnyLineInsideBlockEvaluatesBlock) {
    const std::string block = kBlock.substr(0, kBlock.size() - 1);
    for (int caret : {0, 10, (int)kBlock.find("lfsaw"), (int)kBlock.rfind(')')}) {
        EXPECT_EQ(SelectEvalCode(kBlock, caret, caret, caret), block) << "caret " << caret;
    }
}

TEST(SelectEvalCode, BlockIncludesCodeOnItsEdgeLines) {
    const std::string text = "1 (2\n3) +\n4";
    EXPECT_EQ(SelectEvalCode(text, 0, 0, 0), "1 (2\n3) +");
    EXPECT_EQ(SelectEvalCode(text, 11, 11, 11), "4"); // line after the block is unaffected
}

TEST(SelectEvalCode, SingleLineParensEvaluateLine) {
    EXPECT_EQ(SelectEvalCode("(1 2 +) 3 *\n(4)", 2, 2, 2), "(1 2 +) 3 *");
}

TEST(SelectEvalCode, ParensInCommentsAndStringsIgnored) {
    const std::string text = "1 ; (\n2 \")\" 3\n4";
    EXPECT_EQ(SelectEvalCode(text, 7, 7, 7), "2 \")\" 3");
}

TEST(SelectEvalCode, UnbalancedBlockFallsBackToLine) {
    EXPECT_EQ(SelectEvalCode("(\n1 2 +", 3, 3, 3), "1 2 +");
}

TEST(SelectEvalCode, SquareBracketDefinitionEvaluatesWhole) {
    // From the dev-host session: Cmd+Return inside \[ ... ] = daze evaluated one line, so daze was undefined.
    const std::string text =
        "\\[\n"
        "    #[-5 3 0 3] 12 / = pattern\n"
        "    g .375 0 5 combn g reverse + .5 *\n"
        "] = daze\n"
        "daze play";
    const std::string def = text.substr(0, text.find("\ndaze play"));
    const int inside = (int)text.find("combn");
    EXPECT_EQ(SelectEvalCode(text, inside, inside, inside), def);
    const int play = (int)text.rfind("play");
    EXPECT_EQ(SelectEvalCode(text, play, play, play), "daze play");
}

TEST(SelectEvalCode, BraceFormEvaluatesWhole) {
    const std::string text = "{\n  :a 1\n  :b 2\n} = form";
    EXPECT_EQ(SelectEvalCode(text, 5, 5, 5), text);
}
