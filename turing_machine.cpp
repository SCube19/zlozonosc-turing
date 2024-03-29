#include "turing_machine.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ranges>
#include <set>
#include <string>
#include <unordered_set>

using namespace std;

//--------------CONVERSION IMPLEMENTATION--------------------//
namespace {
// simple parentheses macro
#define p(x) "(" + x + ")"

namespace state {
// initial tape preparation
const std::string placeLeftGuard = "(pLG)";
const std::string placeRightGuard = "(pRG)";
const std::string reverseFind = "(rvF)";
const std::string reversePlace = "(rvP)";
const std::string reverseSkip = "(rvS)";
const std::string createRightTape = "(crtS)";
const std::string prepareFirstTape = "(prprF)";

// initial state to seek middle separator
const std::string seekSeparator = "(sS)";

// head movers
const std::string firstHeadLeft = "(fHL)";
const std::string firstHeadRight = "(fHR)";
const std::string secondHeadLeft = "(sHL)";
const std::string secondHeadRight = "(sHR)";

// resizers
const std::string shiftCopy = "(sftCp)";
const std::string shift = "(sft)";
const std::string shiftInsertHead1 = "(sftH1)";
const std::string shiftInsertHead2 = "(sftH2)";
const std::string shiftInsertGuard1 = "(sftG1)";
const std::string shiftInsertGuard2 = "(sftG2)";
const std::string afterShiftSearch = "(sftsrch)";
const std::string resizeRight1 = "(rsz1)";
const std::string resizeRight2 = "(rsz2)";
const std::string afterResizeSearch = "(rszsrch)";

// searchers
const std::string searchFirst = "(srchF)";
const std::string searchSecond = "(srchS)";
const std::string fetchFirst = "(ftchF)";
const std::string fetchedFirst = "(ftchdF)";
const std::string fetchSecond = "(ftchS)";
const std::string fetchedSecond = "(ftchdS)";

// mutators
const std::string mutateFirst = "(mtxF)";
const std::string mutateSecond = "(mtxS)";

// separator reject
const std::string checkFall = "(winfall)";
const std::string die = "(die)";
}  // namespace state

namespace move {
const std::string right = ">";
const std::string rightId = "rt";
const std::string left = "<";
const std::string leftId = "lt";
const std::string stay = "-";
const std::string stayId = "st";

const std::string id(const char mv) {
    std::string mvs = std::string{mv};
    return (mvs == right ? rightId : mvs == left ? leftId : stayId);
}

}  // namespace move
namespace letter {
// inidicates where the head is on the virtual tape
const std::string headIndicator = "1";
// allows to determine if we are on the 0-th cell
const std::string leftGuardIndicator = "LG";
// separates two virtual tapes
const std::string separatorIndicator = "Sep";
// determines the end of tape when resizing/shifting
const std::string rightGuardIdicator = "RG";
const std::string reversePlaceholder = "Rv";
}  // namespace letter

// actual unique letters using letter:: namespace
std::string leftGuard;
std::string separator;
std::string rightGuard;
std::string reverseIndicator;

// original working alphabet
std::vector<std::string> alphabet;
// original working alphabet + leftGuard + rightGuard + separator +
// letter::headIndicator
std::vector<std::string> extAlphabet;
// extAlphabet - separator
std::vector<std::string> extAlphabetNoSep;
// all the original machine's states
std::vector<std::string> originalStates;

void prepareGlobals(const TuringMachine &tm) {
    // define states
    originalStates = tm.set_of_states();

    // defining new symbols as longest letter + something ensuring uniqueness of
    // guard and separator
    std::string longest = *std::ranges::max_element(
        tm.input_alphabet,
        [](std::string a, std::string b) { return a.size() < b.size(); });
    leftGuard = p(letter::leftGuardIndicator + longest);
    rightGuard = p(letter::rightGuardIdicator + longest);
    separator = p(letter::separatorIndicator + longest);
    reverseIndicator = p(letter::reversePlaceholder + longest);

    // define alphabets
    alphabet = tm.working_alphabet();
    extAlphabetNoSep = alphabet;
    std::ranges::copy(std::vector<std::string>{leftGuard, rightGuard, separator,
                                               letter::headIndicator},
                      std::back_inserter(extAlphabetNoSep));
    extAlphabet = extAlphabetNoSep;
    extAlphabet.push_back(separator);
}

// creates tape for the converted machine to recognize
transitions_t &&addTapePreparators(transitions_t &&transitions) {
    for (const auto &letter : alphabet) {
        // initial guard insert and copying
        transitions[{INITIAL_STATE, {letter}}] = {
            p(state::placeLeftGuard + letter), {leftGuard}, move::right};
        std::ranges::for_each(alphabet, [&](const auto &current) {
            transitions[{p(state::placeLeftGuard + letter), {current}}] = {
                p(state::placeLeftGuard + current), {letter}, move::right};
        });
        transitions[{p(state::placeLeftGuard + letter), {BLANK}}] = {
            state::reverseFind, {letter}, move::stay};

        // reversing input
        transitions[{state::reverseFind, {letter}}] = {
            p(state::reversePlace + letter), {reverseIndicator}, move::right};
        transitions[{state::reverseFind, {reverseIndicator}}] = {
            state::reverseFind, {reverseIndicator}, move::left};

        std::ranges::for_each(alphabet, [&](const auto &current) {
            transitions[{p(state::reversePlace + letter), {current}}] = {
                p(state::reversePlace + letter), {current}, move::right};
        });
        transitions[{p(state::reversePlace + letter), {reverseIndicator}}] = {
            p(state::reversePlace + letter), {reverseIndicator}, move::right};
        transitions[{p(state::reversePlace + letter), {BLANK}}] = {
            state::reverseSkip, {letter}, move::left};

        transitions[{state::reverseSkip, {letter}}] = {
            state::reverseSkip, {letter}, move::left};

        // adding head indicator slots
        transitions[{state::prepareFirstTape, {letter}}] = {
            p(state::prepareFirstTape + letter),
            {reverseIndicator},
            move::left};
        transitions[{p(state::prepareFirstTape + letter), {reverseIndicator}}] =
            {p(state::prepareFirstTape + letter),
             {reverseIndicator},
             move::left};
        transitions[{p(state::prepareFirstTape + letter), {leftGuard}}] = {
            p(state::prepareFirstTape + letter + "1"),
            {leftGuard},
            move::right};
        transitions[{p(state::prepareFirstTape + letter), {BLANK}}] = {
            p(state::prepareFirstTape + letter + "1"), {BLANK}, move::right};
        transitions[{p(state::prepareFirstTape + letter + "1"),
                     {reverseIndicator}}] = {
            p(state::prepareFirstTape + "2"), {letter}, move::right};
        transitions[{p(state::prepareFirstTape + "2"), {reverseIndicator}}] = {
            state::prepareFirstTape, {BLANK}, move::right};
    }
    // some left-overs independent from letters
    transitions[{state::reverseFind, {leftGuard}}] = {
        state::prepareFirstTape, {leftGuard}, move::right};

    transitions[{state::reverseSkip, {reverseIndicator}}] = {
        state::reverseFind, {reverseIndicator}, move::left};

    transitions[{state::prepareFirstTape, {reverseIndicator}}] = {
        state::prepareFirstTape, {reverseIndicator}, move::right};
    transitions[{state::prepareFirstTape, {BLANK}}] = {
        state::createRightTape, {BLANK}, move::left};

    // append 1 (Sep) _ _ (Rg)
    transitions[{state::createRightTape, {BLANK}}] = {
        p(state::createRightTape + "1"), {letter::headIndicator}, move::right};
    transitions[{p(state::createRightTape + "1"), {BLANK}}] = {
        p(state::createRightTape + "2"), {separator}, move::right};
    transitions[{p(state::createRightTape + "2"), {BLANK}}] = {
        p(state::createRightTape + "3"), {letter::headIndicator}, move::right};
    transitions[{p(state::createRightTape + "3"), {BLANK}}] = {
        p(state::createRightTape + "4"), {BLANK}, move::right};
    transitions[{p(state::createRightTape + "4"), {BLANK}}] = {
        state::seekSeparator, {rightGuard}, move::left};

    // go 3 to the left
    transitions[{state::seekSeparator, {BLANK}}] = {
        state::seekSeparator, {BLANK}, move::left};
    transitions[{state::seekSeparator, {letter::headIndicator}}] = {
        state::seekSeparator, {letter::headIndicator}, move::left};

    // start simulation
    transitions[{state::seekSeparator, {separator}}] = {
        state::searchFirst, {separator}, move::left};

    // empty word cornercase
    transitions[{INITIAL_STATE, {BLANK}}] = {
        p(state::prepareFirstTape + "1"), {leftGuard}, move::right};
    transitions[{p(state::prepareFirstTape + "1"), {BLANK}}] = {
        p(state::prepareFirstTape + "2"), {BLANK}, move::right};
    transitions[{p(state::prepareFirstTape + "2"), {BLANK}}] = {
        state::prepareFirstTape, {BLANK}, move::right};

    return std::move(transitions);
}

// adds states for the purpose of resizing/shifting the tape
transitions_t &&addResizers(transitions_t &&transitions) {
    // optimize number of states by creating alphabet + separator
    std::vector<std::string> alphabetSep = alphabet;
    alphabetSep.push_back(separator);

    for (const auto &state : originalStates) {
        // if guard go right start shifting
        transitions[{p(state::mutateFirst + state), {leftGuard}}] = {
            p(state::shiftInsertHead1 + state + BLANK),
            {leftGuard},
            move::right};

        for (const auto &letter : alphabetSep) {
            // resizing right tape works a little bit different from shifting
            // so new states are necessary
            // start resizing
            transitions[{p(state::mutateSecond + state + letter),
                         {rightGuard}}] = {
                p(state::resizeRight1 + state + letter),
                {letter::headIndicator},
                move::right};
            // continue resizing
            transitions[{p(state::resizeRight1 + state + letter), {BLANK}}] = {
                p(state::resizeRight2 + state + letter), {BLANK}, move::right};
            // resizing done go back to the head
            transitions[{p(state::resizeRight2 + state + letter), {BLANK}}] = {
                p(state::afterResizeSearch + state + letter),
                {rightGuard},
                move::left};
            // we must encounter BLANK here
            transitions[{p(state::afterResizeSearch + state + letter),
                         {BLANK}}] = {
                p(state::afterResizeSearch + state + letter),
                {BLANK},
                move::left};
            // resizing done continue with the algorithm
            transitions[{p(state::afterResizeSearch + state + letter),
                         {letter::headIndicator}}] = {
                p(state::searchFirst + state + letter),
                {letter::headIndicator},
                move::left};

            // initial erasure + later head shift on the second tape
            std::ranges::for_each(alphabet, [&](const auto &letterToRemember) {
                transitions[{p(state::shiftInsertHead1 + state + letter),
                             {letterToRemember}}] = {
                    p(state::shiftInsertHead2 + state + letterToRemember),
                    {letter},
                    move::right};
            });

            // initial head insertion + later head insertion during shift on
            // second tape
            transitions[{p(state::shiftInsertHead2 + state + letter),
                         {BLANK}}] = {p(state::shiftCopy + state + letter),
                                      {letter::headIndicator},
                                      move::right};
            transitions[{p(state::shiftInsertHead2 + state + letter),
                         {rightGuard}}] = {
                p(state::shiftInsertGuard1 + state + letter),
                {letter::headIndicator},
                move::right};

            // general case of copying letter
            std::ranges::for_each(
                alphabetSep, [&](const auto &letterToRemember) {
                    transitions[{p(state::shiftCopy + state + letter),
                                 {letterToRemember}}] = {
                        p(state::shift + state + letterToRemember),
                        {letter},
                        move::right};
                });

            // general case of going through the indicator field during copying
            transitions[{p(state::shift + state + letter), {BLANK}}] = {
                p(state::shiftCopy + state + letter), {BLANK}, move::right};
            transitions[{p(state::shift + state + letter),
                         {letter::headIndicator}}] = {
                p(state::shiftInsertHead1 + state + letter),
                {BLANK},
                move::right};
            // right guard encountered, start shifting it
            transitions[{p(state::shift + state + letter), {rightGuard}}] = {
                p(state::shiftInsertGuard1 + state + letter),
                {BLANK},
                move::right};

            // continue shifting guard
            std::ranges::for_each(alphabet, [&](const auto &letterToRemember) {
                transitions[{p(state::shiftInsertGuard1 + state + letter),
                             {BLANK}}] = {
                    p(state::shiftInsertGuard2 + state), {letter}, move::right};
            });

            // search for the second's tape head after shifting
            transitions[{p(state::afterShiftSearch + state), {letter}}] = {
                p(state::afterShiftSearch + state), {letter}, move::left};
        }
        // end shifting and search for the right head
        transitions[{p(state::shiftInsertGuard2 + state), {BLANK}}] = {
            p(state::afterShiftSearch + state), {rightGuard}, move::left};
        // we found the second head - continue as if nothing happened and first
        // head was set on blank
        transitions[{p(state::afterShiftSearch + state),
                     {letter::headIndicator}}] = {
            p(state::fetchSecond + state + BLANK),
            {letter::headIndicator},
            move::right};
    }

    return std::move(transitions);
}

// add state that ensures the machine's demise in the same way as on 2 tape
// machine
transitions_t &&addSeparatorRejects(transitions_t &&transitions) {
    // we want to preserve the error message for being out of bounds so we are
    // going to produce it by going left to the -1 index
    std::ranges::for_each(extAlphabet, [&](const auto &letter) {
        transitions[{state::die, {letter}}] = {
            state::die, {letter}, move::left};
    });
    return std::move(transitions);
}

// add states that bounce between left and right head
transitions_t &&addSearchersAndFetchers(transitions_t &&transitions) {
    // intial search
    transitions[{state::searchFirst, {letter::headIndicator}}] = {
        p(state::fetchFirst + INITIAL_STATE),
        {letter::headIndicator},
        move::left};

    for (const auto &state : originalStates) {
        for (const auto &letter : alphabet) {
            // simple fetcher states to get head's letter
            transitions[{p(state::fetchFirst + state), {letter}}] = {
                p(state::fetchedFirst + state + letter), {letter}, move::right};
            transitions[{p(state::fetchSecond + state), {letter}}] = {
                p(state::fetchedSecond + state + letter), {letter}, move::left};

            // go search right head
            transitions[{p(state::fetchedFirst + state + letter),
                         {letter::headIndicator}}] = {
                p(state::searchSecond + state + letter),
                {letter::headIndicator},
                move::right};
            // go search left head
            transitions[{p(state::fetchedSecond + state + letter),
                         {letter::headIndicator}}] = {
                p(state::searchFirst + state + letter),
                {letter::headIndicator},
                move::left};

            // skip everything along the way during search
            std::ranges::for_each(extAlphabet, [&](const auto &toSkip) {
                transitions[{p(state::searchSecond + state + letter),
                             {toSkip}}] = {
                    p(state::searchSecond + state + letter),
                    {toSkip},
                    move::right};
            });
            // skip everything along the way searching the left indicator
            std::ranges::for_each(extAlphabet, [&](const auto &toSkip) {
                transitions[{p(state::searchFirst + state + letter),
                             {toSkip}}] = {
                    p(state::searchFirst + state + letter),
                    {toSkip},
                    move::left};
            });

            // found the head
            transitions[{p(state::searchSecond + state + letter),
                         {letter::headIndicator}}] = {
                p(state::fetchSecond + state + letter),
                {letter::headIndicator},
                move::right};
            transitions[{p(state::searchFirst + state + letter),
                         {letter::headIndicator}}] = {
                p(state::fetchFirst + state + letter),
                {letter::headIndicator},
                move::left};

            // fetch second letter and start transformation
            std::ranges::for_each(alphabet, [&](const auto &toFetch) {
                transitions[{p(state::fetchSecond + state + letter),
                             {toFetch}}] = {
                    p(state::mutateSecond + state + letter + toFetch),
                    {toFetch},
                    move::left};
            });
            std::ranges::for_each(alphabet, [&](const auto &toFetch) {
                transitions[{p(state::fetchFirst + state + letter),
                             {toFetch}}] = {
                    p(state::mutateFirst + state + toFetch + letter),
                    {toFetch},
                    move::right};
            });
        }
    }

    return std::move(transitions);
}

// main simulator states
transitions_t &&addMutators(const TuringMachine &tm,
                            transitions_t &&transitions) {
    // false accept states
    transitions[{p(state::checkFall + move::rightId),
                 {letter::headIndicator}}] = {
        ACCEPTING_STATE, {letter::headIndicator}, move::stay};
    transitions[{p(state::checkFall + move::stayId), {letter::headIndicator}}] =
        {ACCEPTING_STATE, {letter::headIndicator}, move::stay};
    transitions[{p(state::checkFall + move::leftId), {letter::headIndicator}}] =
        {p(state::checkFall + "1"), {letter::headIndicator}, move::right};
    std::ranges::for_each(extAlphabet, [&](const auto &letter) {
        transitions[{p(state::checkFall + "1"), {letter}}] = {
            ACCEPTING_STATE, {letter}, move::stay};
    });
    transitions[{p(state::checkFall + "1"), {separator}}] = {
        state::die, {separator}, move::left};

    // consider every combination of letters and states
    for (const auto &state : originalStates) {
        for (const auto &letter1 : alphabet) {
            for (const auto &letter2 : alphabet) {
                // check if such a transition exists (if not then it wont exist
                // in converted machine and thus will reject)
                if (auto it = tm.transitions.find({state, {letter1, letter2}});
                    it != tm.transitions.end()) {
                    // get updated values
                    auto &[newState, newLetters, moves] = it->second;
                    // accept accepting states
                    if (newState == ACCEPTING_STATE)
                        transitions[{
                            p(state::mutateFirst + state + letter1 + letter2),
                            {letter::headIndicator}}] = {
                            p(state::checkFall + move::id(moves[0])),
                            {letter::headIndicator},
                            move::stay};
                    // reject rejecting states
                    else if (newState == REJECTING_STATE)
                        transitions[{
                            p(state::mutateFirst + state + letter1 + letter2),
                            {letter::headIndicator}}] = {
                            REJECTING_STATE,
                            {letter::headIndicator},
                            move::stay};
                    // create mutator state for any other state
                    else
                        // go to the letter associated with current head with
                        // new state, new letter, and direction
                        transitions[{
                            p(state::mutateFirst + state + letter1 + letter2),
                            {letter::headIndicator}}] = {
                            p(state::mutateFirst + newState + newLetters[0] +
                              move::id(moves[0])),
                            {BLANK},
                            move::left};
                    // create mutator for the second head
                    // we dont update the saved state here as second head is
                    // always one state ahead (or equal) that's also why we need
                    // (new) label
                    transitions[{
                        p(state::mutateSecond + state + letter1 + letter2),
                        {letter::headIndicator}}] = {
                        p(state::mutateSecond + state + "(new)" +
                          newLetters[1] + move::id(moves[1])),
                        {BLANK},
                        move::right};
                }

                // first head mutators
                // direction left
                transitions[{
                    p(state::mutateFirst + state + letter1 + move::leftId),
                    {letter2}}] = {p(state::mutateFirst + state + move::leftId),
                                   {letter1},
                                   move::right};
                // going left is going right on the virtual first tape and
                // requires multiple steps
                transitions[{p(state::mutateFirst + state + move::leftId),
                             {BLANK}}] = {
                    p(state::mutateFirst + state + "2" + move::leftId),
                    {BLANK},
                    move::right};
                transitions[{p(state::mutateFirst + state + move::leftId),
                             {separator}}] = {
                    p(state::die), {separator}, move::left};

                // direction stay
                transitions[{
                    p(state::mutateFirst + state + letter1 + move::stayId),
                    {letter2}}] = {
                    p(state::mutateFirst + state), {letter1}, move::right};

                // direction right
                transitions[{
                    p(state::mutateFirst + state + letter1 + move::rightId),
                    {letter2}}] = {
                    p(state::mutateFirst + state), {letter1}, move::left};

                // second mutators
                // initial left, we remember the original letter and keep it for
                // the first head's mutator
                transitions[{p(state::mutateSecond + state + "(new)" + letter1 +
                               move::leftId),
                             {letter2}}] = {
                    p(state::mutateSecond + state + letter2 + move::leftId),
                    {letter1},
                    move::left};

                // going left requires multiple steps
                transitions[{
                    p(state::mutateSecond + state + letter1 + move::leftId),
                    {BLANK}}] = {p(state::mutateSecond + state + letter1 + "2" +
                                   move::leftId),
                                 {BLANK},
                                 move::left};
                transitions[{
                    p(state::mutateSecond + state + letter1 + move::leftId),
                    {separator}}] = {p(state::die), {separator}, move::left};

                transitions[{p(state::mutateSecond + state + letter2 + "2" +
                               move::leftId),
                             {letter1}}] = {
                    p(state::mutateSecond + state + letter2),
                    {letter1},
                    move::left};

                // initial stay, we remember the original letter and keep it for
                // the first head's mutator
                transitions[{p(state::mutateSecond + state + "(new)" + letter1 +
                               move::stayId),
                             {letter2}}] = {
                    p(state::mutateSecond + state + letter2),
                    {letter1},
                    move::left};

                // initial righ,twe remember the original letter and keep it for
                // the first head's mutator
                transitions[{p(state::mutateSecond + state + "(new)" + letter1 +
                               move::rightId),
                             {letter2}}] = {
                    p(state::mutateSecond + state + letter2),
                    {letter1},
                    move::right};
            }
            // next step places the head after right mutation on the first tape
            transitions[{p(state::mutateFirst + state + "2" + move::leftId),
                         {letter1}}] = {
                p(state::mutateFirst + state), {letter1}, move::right};

            // if we mutated the second then just go search the first head
            transitions[{p(state::mutateSecond + state + letter1), {BLANK}}] = {
                p(state::searchFirst + state + letter1),
                {letter::headIndicator},
                move::left};
        }
        // found the place to put the head
        transitions[{p(state::mutateFirst + state), {BLANK}}] = {
            p(state::fetchFirst + state), {letter::headIndicator}, move::left};
    }

    return std::move(transitions);
}
}  // namespace

void TuringMachine::twoToOne() {
    this->num_tapes = 1;

    prepareGlobals(*this);

    // make new states
    this->transitions = addMutators(
        *this, addSearchersAndFetchers(addSeparatorRejects(
                   addResizers(addTapePreparators(transitions_t())))));
}
//--------------END IMPLEMENTATION-----------------------//

class Reader {
   public:
    bool is_next_token_available() {
        return next_char != '\n' && next_char != EOF;
    }

    string next_token() {  // only in the current line
        assert(is_next_token_available());
        string res;
        while (next_char != ' ' && next_char != '\t' && next_char != '\n' &&
               next_char != EOF)
            res += get_next_char();
        skip_spaces();
        return res;
    }

    void go_to_next_line() {  // in particular skips empty lines
        assert(!is_next_token_available());
        while (next_char == '\n') {
            get_next_char();
            skip_spaces();
        }
    }

    ~Reader() { assert(fclose(input) == 0); }

    Reader(FILE *input_) : input(input_) {
        assert(input);
        get_next_char();
        skip_spaces();
        if (!is_next_token_available()) go_to_next_line();
    }

    int get_line_num() const { return line; }

   private:
    FILE *input;
    int next_char;  // we always have the next char here
    int line = 1;

    int get_next_char() {
        if (next_char == '\n') ++line;
        int prev = next_char;
        next_char = fgetc(input);
        if (next_char == '#')  // skip a comment until EOL or EOF
            while (next_char != '\n' && next_char != EOF)
                next_char = fgetc(input);
        return prev;
    }

    void skip_spaces() {
        while (next_char == ' ' || next_char == '\t') get_next_char();
    }
};

static bool is_valid_char(int ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
}

static bool is_direction(int ch) {
    return ch == HEAD_LEFT || ch == HEAD_RIGHT || ch == HEAD_STAY;
}

// searches for an identifier starting from position pos;
// at the end pos is the position after the identifier
// (if false returned, pos remains unchanged)
static bool check_identifier(string ident, size_t &pos) {
    if (pos >= ident.size()) return false;
    if (is_valid_char(ident[pos])) {
        ++pos;
        return true;
    }
    if (ident[pos] != '(') return false;
    size_t pos2 = pos + 1;
    while (check_identifier(ident, pos2))
        ;
    if (pos2 == pos + 1 || pos2 >= ident.size() || ident[pos2] != ')')
        return false;
    pos = pos2 + 1;
    return true;
}

static bool is_identifier(string ident) {
    size_t pos = 0;
    return check_identifier(ident, pos) && pos == ident.length();
}

TuringMachine::TuringMachine(int num_tapes_, vector<string> input_alphabet_,
                             transitions_t transitions_)
    : num_tapes(num_tapes_),
      input_alphabet(input_alphabet_),
      transitions(transitions_) {
    assert(num_tapes > 0);
    assert(!input_alphabet.empty());
    for (auto letter : input_alphabet)
        assert(is_identifier(letter) && letter != BLANK);
    for (auto transition : transitions) {
        auto state_before = transition.first.first;
        auto letters_before = transition.first.second;
        auto state_after = get<0>(transition.second);
        auto letters_after = get<1>(transition.second);
        auto directions = get<2>(transition.second);
        assert(is_identifier(state_before) && state_before != ACCEPTING_STATE &&
               state_before != REJECTING_STATE && is_identifier(state_after));
        assert(letters_before.size() == (size_t)num_tapes &&
               letters_after.size() == (size_t)num_tapes &&
               directions.length() == (size_t)num_tapes);
        for (int a = 0; a < num_tapes; ++a)
            assert(is_identifier(letters_before[a]) &&
                   is_identifier(letters_after[a]) &&
                   is_direction(directions[a]));
    }
}

#define syntax_error(reader, message)                                    \
    for (;;) {                                                           \
        cerr << "Syntax error in line " << reader.get_line_num() << ": " \
             << message << "\n";                                         \
        exit(1);                                                         \
    }

static string read_identifier(Reader &reader) {
    if (!reader.is_next_token_available())
        syntax_error(reader, "Identifier expected");
    string ident = reader.next_token();
    size_t pos = 0;
    if (!check_identifier(ident, pos) || pos != ident.length())
        syntax_error(reader, "Invalid identifier \"" << ident << "\"");
    return ident;
}

#define NUM_TAPES "num-tapes:"
#define INPUT_ALPHABET "input-alphabet:"

TuringMachine read_tm_from_file(FILE *input) {
    Reader reader(input);

    // number of tapes
    int num_tapes;
    if (!reader.is_next_token_available() || reader.next_token() != NUM_TAPES)
        syntax_error(reader, "\"" NUM_TAPES "\" expected");
    try {
        if (!reader.is_next_token_available()) throw 0;
        string num_tapes_str = reader.next_token();
        size_t last;
        num_tapes = stoi(num_tapes_str, &last);
        if (last != num_tapes_str.length() || num_tapes <= 0) throw 0;
    } catch (...) {
        syntax_error(reader,
                     "Positive integer expected after \"" NUM_TAPES "\"");
    }
    if (reader.is_next_token_available())
        syntax_error(reader, "Too many tokens in a line");
    reader.go_to_next_line();

    // input alphabet
    vector<string> input_alphabet;
    if (!reader.is_next_token_available() ||
        reader.next_token() != INPUT_ALPHABET)
        syntax_error(reader, "\"" INPUT_ALPHABET "\" expected");
    while (reader.is_next_token_available()) {
        input_alphabet.emplace_back(read_identifier(reader));
        if (input_alphabet.back() == BLANK)
            syntax_error(reader, "The blank letter \"" BLANK
                                 "\" is not allowed in the input alphabet");
    }
    if (input_alphabet.empty()) syntax_error(reader, "Identifier expected");
    reader.go_to_next_line();

    // transitions
    transitions_t transitions;
    while (reader.is_next_token_available()) {
        string state_before = read_identifier(reader);
        if (state_before == "(accept)" || state_before == "(reject)")
            syntax_error(reader, "No transition can start in the \""
                                     << state_before << "\" state");

        vector<string> letters_before;
        for (int a = 0; a < num_tapes; ++a)
            letters_before.emplace_back(read_identifier(reader));

        if (transitions.find(make_pair(state_before, letters_before)) !=
            transitions.end())
            syntax_error(reader, "The machine is not deterministic");

        string state_after = read_identifier(reader);

        vector<string> letters_after;
        for (int a = 0; a < num_tapes; ++a)
            letters_after.emplace_back(read_identifier(reader));

        string directions;
        for (int a = 0; a < num_tapes; ++a) {
            string dir;
            if (!reader.is_next_token_available() ||
                (dir = reader.next_token()).length() != 1 ||
                !is_direction(dir[0]))
                syntax_error(reader, "Move direction expected, which should be "
                                         << HEAD_LEFT << ", " << HEAD_RIGHT
                                         << ", or " << HEAD_STAY);
            directions += dir;
        }

        if (reader.is_next_token_available())
            syntax_error(reader, "Too many tokens in a line");
        reader.go_to_next_line();

        transitions[make_pair(state_before, letters_before)] =
            make_tuple(state_after, letters_after, directions);
    }

    return TuringMachine(num_tapes, input_alphabet, transitions);
}

vector<string> TuringMachine::working_alphabet() const {
    set<string> letters(input_alphabet.begin(), input_alphabet.end());
    letters.insert(BLANK);
    for (auto transition : transitions) {
        auto letters_before = transition.first.second;
        auto letters_after = get<1>(transition.second);
        letters.insert(letters_before.begin(), letters_before.end());
        letters.insert(letters_after.begin(), letters_after.end());
    }
    return vector<string>(letters.begin(), letters.end());
}

vector<string> TuringMachine::set_of_states() const {
    set<string> states;
    states.insert(INITIAL_STATE);
    states.insert(ACCEPTING_STATE);
    states.insert(REJECTING_STATE);
    for (auto transition : transitions) {
        states.insert(transition.first.first);
        states.insert(get<0>(transition.second));
    }
    return vector<string>(states.begin(), states.end());
}

static void output_vector(ostream &output, vector<string> v) {
    for (string el : v) output << " " << el;
}

void TuringMachine::save_to_file(ostream &output) const {
    output << NUM_TAPES << " " << num_tapes << "\n" << INPUT_ALPHABET;
    output_vector(output, input_alphabet);
    output << "\n";
    for (auto transition : transitions) {
        output << transition.first.first;
        output_vector(output, transition.first.second);
        output << " " << get<0>(transition.second);
        output_vector(output, get<1>(transition.second));
        string directions = get<2>(transition.second);
        for (int a = 0; a < num_tapes; ++a) output << " " << directions[a];
        output << "\n";
    }
}

vector<string> TuringMachine::parse_input(std::string input) const {
    set<string> alphabet(input_alphabet.begin(), input_alphabet.end());
    size_t pos = 0;
    vector<string> res;
    while (pos < input.length()) {
        size_t prev_pos = pos;
        if (!check_identifier(input, pos)) return vector<string>();
        res.emplace_back(input.substr(prev_pos, pos - prev_pos));
        if (alphabet.find(res.back()) == alphabet.end())
            return vector<string>();
    }
    return res;
}