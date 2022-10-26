#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iostream>
#include <ranges>
#include "turing_machine.h"

using namespace std;

//------------------------ CONVERSION IMPLEMENTATION -----------------------------------------//
namespace
{
#define p(x) "(" + x + ")"

    constexpr int initialPadding = 100;
    namespace state
    {
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

        // searchers
        const std::string searchFirst = "(srchF)";
        const std::string searchSecond = "(srchS)";
        const std::string fetchFirst = "(ftchF)";
        const std::string fetchedFirst = "(ftchdF)";
        const std::string fetchSecond = "(ftchS)";

        // mutators
        const std::string mutateFirst = "(mtxF)";
        const std::string mutateSecond = "(mtxS)";

        // separator reject
        const std::string die = "(die)";

        const std::string test = "(test)";
    }
    namespace move
    {
        const std::string right = ">";
        const std::string left = "<";
        const std::string stay = "-";
    }
    namespace letter
    {
        const std::string headIndicator = "1";
        const std::string leftGuardIndicator = "LG";
        const std::string separatorIndicator = "Sep";
        const std::string rightGuardIdicator = "RG";
    }

    std::string leftGuard;
    std::string separator;
    std::string rightGuard;
    std::vector<std::string> alphabet;
    std::vector<std::string> extAlphabet;
    std::vector<std::string> extAlphabetNoSep;
    std::vector<std::string> originalStates;

    inline std::vector<std::vector<std::string>> prepareTape(TuringMachine &tm, const std::vector<std::vector<std::string>> &original)
    {
        // defining new symbols as longest letter + something ensuring uniqueness of guard and separator
        std::string longest = *std::ranges::max_element(original[0], [](std::string a, std::string b)
                                                        { return a.size() < b.size(); });
        leftGuard = p(letter::leftGuardIndicator + longest);
        rightGuard = p(letter::rightGuardIdicator + longest);
        separator = p(letter::separatorIndicator + longest);

        //"*" are head indicator
        std::vector<std::string> newTape{rightGuard, BLANK, letter::headIndicator, separator};

        // for every letter insert _ letter
        std::ranges::for_each(original[0], [&newTape](const auto &a)
                              {newTape.push_back(BLANK); newTape.push_back(a); });
        // head indicator for the first tape
        newTape[4] = letter::headIndicator;
        // some inital overhead as to not resize on small tapes
        // std::ranges::copy(std::vector<std::string>(initialPadding, BLANK), std::back_inserter(newTape));
        newTape.push_back(leftGuard);
        // we inserted everything in the reversed order so we reverse it
        std::ranges::reverse(newTape);

        return std::vector<std::vector<std::string>>{newTape};
    }

    transitions_t &&addSeparatorSeekers(const TuringMachine &tm, transitions_t &&transitions)
    {
        // seek separator from the start
        transitions[{INITIAL_STATE, {leftGuard}}] = {
            state::seekSeparator, {leftGuard}, move::right};

        std::ranges::for_each(extAlphabetNoSep, [&](const auto &letter)
                              { transitions[{state::seekSeparator, {letter}}] = {state::seekSeparator, {letter}, move::right}; });

        // TODO: some initial searchers as in general case we would have the data from the other head
        transitions[{state::seekSeparator, {separator}}] = {state::searchFirst, {separator}, move::left};

        return std::move(transitions);
    }

    transitions_t &&addResizers(transitions_t &&transitions)
    {
        std::vector<std::string> alphabetSep = alphabet;
        alphabetSep.push_back(separator);

        for (const auto &state : originalStates)
        {
            // if guard go right start shifting
            transitions[{p(state::test + state), {leftGuard}}] = {p(state::shiftInsertHead1 + state + BLANK), {leftGuard}, move::right};
            for (const auto &letter : alphabetSep)
            {
                // initial erasure + later star shift on the second tape
                std::ranges::for_each(alphabet, [&](const auto &letterToRemember)
                                      { transitions[{p(state::shiftInsertHead1 + state + letter), {letterToRemember}}] =
                                            {p(state::shiftInsertHead2 + state + letterToRemember), {letter}, move::right}; });

                // initial star insertion + later star insertion during shift on second tape
                transitions[{p(state::shiftInsertHead2 + state + letter), {BLANK}}] =
                    {p(state::shiftCopy + state + letter), {letter::headIndicator}, move::right};
                transitions[{p(state::shiftInsertHead2 + state + letter), {rightGuard}}] =
                    {p(state::shiftInsertGuard1 + state + letter), {letter::headIndicator}, move::right};

                // general case of copying letter
                std::ranges::for_each(alphabetSep, [&](const auto &letterToRemember)
                                      { transitions[{p(state::shiftCopy + state + letter), {letterToRemember}}] =
                                            {p(state::shift + state + letterToRemember), {letter}, move::right}; });

                // general case of going through the indicator field during copying
                transitions[{p(state::shift + state + letter), {BLANK}}] =
                    {p(state::shiftCopy + state + letter), {BLANK}, move::right};
                transitions[{p(state::shift + state + letter), {letter::headIndicator}}] =
                    {p(state::shiftInsertHead1 + state + letter), {BLANK}, move::right};
                transitions[{p(state::shift + state + letter), {rightGuard}}] =
                    {p(state::shiftInsertGuard1 + state + letter), {BLANK}, move::right};

                // end of copying shift guard
                std::ranges::for_each(alphabet, [&](const auto &letterToRemember)
                                      { transitions[{p(state::shiftInsertGuard1 + state + letter), {BLANK}}] =
                                            {p(state::shiftInsertGuard2 + state), {letter}, move::right}; });

                // search for the second's tape indicator
                transitions[{p(state::afterShiftSearch + state), {letter}}] = {p(state::afterShiftSearch + state), {letter}, move::left};
            }
            transitions[{p(state::shiftInsertGuard2 + state), {BLANK}}] =
                {p(state::afterShiftSearch + state), {rightGuard}, move::left};
            // we found the second star - continue if nothing happened and first star was set on blank
            transitions[{p(state::afterShiftSearch + state), {letter::headIndicator}}] = {p(state::searchSecond + state + BLANK), {letter::headIndicator}, move::stay};
        }

        return std::move(transitions);
    }

    transitions_t &&addSeparatorRejects(transitions_t &&transitions)
    {
        // we want to preserve the error message for being out of bounds so we are going to produce it by going left to the -1 index
        std::ranges::for_each(extAlphabet, [&](const auto &letter)
                              { transitions[{state::die, {letter}}] = {state::die, {letter}, move::left}; });
        return std::move(transitions);
    }

    transitions_t &&addSearchersAndFetchers(transitions_t &&transitions)
    {
        transitions[{state::searchFirst, {letter::headIndicator}}] = {state::fetchFirst, {letter::headIndicator}, move::left};
        std::ranges::for_each(alphabet, [&](const auto &letter)
                              { transitions[{state::fetchFirst, {letter}}] = {p(state::fetchedFirst + INITIAL_STATE + letter),
                                                                              {letter},
                                                                              move::right}; });
        for (const auto &state : originalStates)
        {
            for (const auto &letter : alphabet)
            {
                // go search right indicator
                // POSSIBLY ONLY AT THE BEGGINING
                transitions[{p(state::fetchedFirst + state + letter), {letter::headIndicator}}] = {p(state::searchSecond + state + letter),
                                                                                                   {letter::headIndicator},
                                                                                                   move::right};
                // skip everything along the way
                std::ranges::for_each(extAlphabet, [&](const auto &toSkip)
                                      { transitions[{p(state::searchSecond + state + letter), {toSkip}}] =
                                            {p(state::searchSecond + state + letter), {toSkip}, move::right}; });
                // found the indicator
                transitions[{p(state::searchSecond + state + letter), {letter::headIndicator}}] =
                    {p(state::fetchSecond + state + letter), {letter::headIndicator}, move::right};

                // fetch second and mutate second indicator
                std::ranges::for_each(alphabet, [&](const auto &toFetch)
                                      { transitions[{p(state::fetchSecond + state + letter), {toFetch}}] =
                                            {p(state::mutateSecond + state + letter + toFetch), {toFetch}, move::left}; });

                // skip everything along the way searching the left indicator
                std::ranges::for_each(extAlphabet, [&](const auto &toSkip)
                                      { transitions[{p(state::searchFirst + state + letter), {toSkip}}] =
                                            {p(state::searchFirst + state + letter), {toSkip}, move::left}; });
                // found the left indicator fetch letter
                transitions[{p(state::searchFirst + state + letter), {letter::headIndicator}}] =
                    {p(state::fetchFirst + state + letter), {letter::headIndicator}, move::left};
                // fetch first and go mutate first indicator
                std::ranges::for_each(alphabet, [&](const auto &toFetch)
                                      { transitions[{p(state::fetchFirst + state + letter), {toFetch}}] =
                                            {p(state::mutateFirst + state + toFetch + state), {toFetch}, move::right}; });
            }
        }

        return std::move(transitions);
    }

    transitions_t &&addMutators(transitions_t &&transitions)
    {

        return std::move(transitions);
    }

    transitions_t &&addTest(transitions_t &&transitions)
    {
        std::ranges::for_each(alphabet, [&](const auto &letter)
                              { transitions[{p(state::test + "(copyA)"), {letter}}] = {p(state::test + "(copyA)"), {letter}, move::left}; });
        transitions[{p(state::test + "(copyA)"), {letter::headIndicator}}] = {p(state::test + "(copyA)"), {BLANK}, move::left};
        return std::move(transitions);
    }
}

void TuringMachine::twoToOne(std::vector<std::vector<std::string>> &tapes)
{
    // convert tape
    tapes = prepareTape(*this, tapes);

    // define states
    originalStates = this->set_of_states();

    // define alphabets
    std::ranges::copy(originalStates, std::ostream_iterator<std::string>(std::cout, " "));
    std::cout << std::endl;
    alphabet = this->working_alphabet();
    extAlphabetNoSep = alphabet;
    std::ranges::copy(std::vector<std::string>{leftGuard, rightGuard, separator, letter::headIndicator}, std::back_inserter(extAlphabetNoSep));
    extAlphabet = extAlphabetNoSep;
    extAlphabet.push_back(separator);

    // make new states
    this->transitions =
        addTest(
            addMutators(
                addSearchersAndFetchers(
                    addSeparatorRejects(
                        addResizers(
                            addSeparatorSeekers(*this, transitions_t()))))));
}
//--------------------------------------------------------------------------------------------//

class Reader
{
public:
    bool is_next_token_available()
    {
        return next_char != '\n' && next_char != EOF;
    }

    string next_token()
    { // only in the current line
        assert(is_next_token_available());
        string res;
        while (next_char != ' ' && next_char != '\t' && next_char != '\n' && next_char != EOF)
            res += get_next_char();
        skip_spaces();
        return res;
    }

    void go_to_next_line()
    { // in particular skips empty lines
        assert(!is_next_token_available());
        while (next_char == '\n')
        {
            get_next_char();
            skip_spaces();
        }
    }

    ~Reader()
    {
        assert(fclose(input) == 0);
    }

    Reader(FILE *input_) : input(input_)
    {
        assert(input);
        get_next_char();
        skip_spaces();
        if (!is_next_token_available())
            go_to_next_line();
    }

    int get_line_num() const
    {
        return line;
    }

private:
    FILE *input;
    int next_char; // we always have the next char here
    int line = 1;

    int get_next_char()
    {
        if (next_char == '\n')
            ++line;
        int prev = next_char;
        next_char = fgetc(input);
        if (next_char == '#') // skip a comment until EOL or EOF
            while (next_char != '\n' && next_char != EOF)
                next_char = fgetc(input);
        return prev;
    }

    void skip_spaces()
    {
        while (next_char == ' ' || next_char == '\t')
            get_next_char();
    }
};

static bool is_valid_char(int ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
}

static bool is_direction(int ch)
{
    return ch == HEAD_LEFT || ch == HEAD_RIGHT || ch == HEAD_STAY;
}

// searches for an identifier starting from position pos;
// at the end pos is the position after the identifier
// (if false returned, pos remains unchanged)
static bool check_identifier(string ident, size_t &pos)
{
    if (pos >= ident.size())
        return false;
    if (is_valid_char(ident[pos]))
    {
        ++pos;
        return true;
    }
    if (ident[pos] != '(')
        return false;
    size_t pos2 = pos + 1;
    while (check_identifier(ident, pos2))
        ;
    if (pos2 == pos + 1 || pos2 >= ident.size() || ident[pos2] != ')')
        return false;
    pos = pos2 + 1;
    return true;
}

static bool is_identifier(string ident)
{
    size_t pos = 0;
    return check_identifier(ident, pos) && pos == ident.length();
}

TuringMachine::TuringMachine(int num_tapes_, vector<string> input_alphabet_, transitions_t transitions_)
    : num_tapes(num_tapes_), input_alphabet(input_alphabet_), transitions(transitions_)
{
    assert(num_tapes > 0);
    assert(!input_alphabet.empty());
    for (auto letter : input_alphabet)
        assert(is_identifier(letter) && letter != BLANK);
    for (auto transition : transitions)
    {
        auto state_before = transition.first.first;
        auto letters_before = transition.first.second;
        auto state_after = get<0>(transition.second);
        auto letters_after = get<1>(transition.second);
        auto directions = get<2>(transition.second);
        assert(is_identifier(state_before) && state_before != ACCEPTING_STATE && state_before != REJECTING_STATE && is_identifier(state_after));
        assert(letters_before.size() == (size_t)num_tapes && letters_after.size() == (size_t)num_tapes && directions.length() == (size_t)num_tapes);
        for (int a = 0; a < num_tapes; ++a)
            assert(is_identifier(letters_before[a]) && is_identifier(letters_after[a]) && is_direction(directions[a]));
    }
}

#define syntax_error(reader, message)                                                        \
    for (;;)                                                                                 \
    {                                                                                        \
        cerr << "Syntax error in line " << reader.get_line_num() << ": " << message << "\n"; \
        exit(1);                                                                             \
    }

static string read_identifier(Reader &reader)
{
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

TuringMachine read_tm_from_file(FILE *input)
{
    Reader reader(input);

    // number of tapes
    int num_tapes;
    if (!reader.is_next_token_available() || reader.next_token() != NUM_TAPES)
        syntax_error(reader, "\"" NUM_TAPES "\" expected");
    try
    {
        if (!reader.is_next_token_available())
            throw 0;
        string num_tapes_str = reader.next_token();
        size_t last;
        num_tapes = stoi(num_tapes_str, &last);
        if (last != num_tapes_str.length() || num_tapes <= 0)
            throw 0;
    }
    catch (...)
    {
        syntax_error(reader, "Positive integer expected after \"" NUM_TAPES "\"");
    }
    if (reader.is_next_token_available())
        syntax_error(reader, "Too many tokens in a line");
    reader.go_to_next_line();

    // input alphabet
    vector<string> input_alphabet;
    if (!reader.is_next_token_available() || reader.next_token() != INPUT_ALPHABET)
        syntax_error(reader, "\"" INPUT_ALPHABET "\" expected");
    while (reader.is_next_token_available())
    {
        input_alphabet.emplace_back(read_identifier(reader));
        if (input_alphabet.back() == BLANK)
            syntax_error(reader, "The blank letter \"" BLANK "\" is not allowed in the input alphabet");
    }
    if (input_alphabet.empty())
        syntax_error(reader, "Identifier expected");
    reader.go_to_next_line();

    // transitions
    transitions_t transitions;
    while (reader.is_next_token_available())
    {
        string state_before = read_identifier(reader);
        if (state_before == "(accept)" || state_before == "(reject)")
            syntax_error(reader, "No transition can start in the \"" << state_before << "\" state");

        vector<string> letters_before;
        for (int a = 0; a < num_tapes; ++a)
            letters_before.emplace_back(read_identifier(reader));

        if (transitions.find(make_pair(state_before, letters_before)) != transitions.end())
            syntax_error(reader, "The machine is not deterministic");

        string state_after = read_identifier(reader);

        vector<string> letters_after;
        for (int a = 0; a < num_tapes; ++a)
            letters_after.emplace_back(read_identifier(reader));

        string directions;
        for (int a = 0; a < num_tapes; ++a)
        {
            string dir;
            if (!reader.is_next_token_available() || (dir = reader.next_token()).length() != 1 || !is_direction(dir[0]))
                syntax_error(reader, "Move direction expected, which should be " << HEAD_LEFT << ", " << HEAD_RIGHT << ", or " << HEAD_STAY);
            directions += dir;
        }

        if (reader.is_next_token_available())
            syntax_error(reader, "Too many tokens in a line");
        reader.go_to_next_line();

        transitions[make_pair(state_before, letters_before)] = make_tuple(state_after, letters_after, directions);
    }

    return TuringMachine(num_tapes, input_alphabet, transitions);
}

vector<string> TuringMachine::working_alphabet() const
{
    set<string> letters(input_alphabet.begin(), input_alphabet.end());
    letters.insert(BLANK);
    for (auto transition : transitions)
    {
        auto letters_before = transition.first.second;
        auto letters_after = get<1>(transition.second);
        letters.insert(letters_before.begin(), letters_before.end());
        letters.insert(letters_after.begin(), letters_after.end());
    }
    return vector<string>(letters.begin(), letters.end());
}

vector<string> TuringMachine::set_of_states() const
{
    set<string> states;
    states.insert(INITIAL_STATE);
    states.insert(ACCEPTING_STATE);
    states.insert(REJECTING_STATE);
    for (auto transition : transitions)
    {
        states.insert(transition.first.first);
        states.insert(get<0>(transition.second));
    }
    return vector<string>(states.begin(), states.end());
}

static void output_vector(ostream &output, vector<string> v)
{
    for (string el : v)
        output << " " << el;
}

void TuringMachine::save_to_file(ostream &output) const
{
    output << NUM_TAPES << " " << num_tapes << "\n"
           << INPUT_ALPHABET;
    output_vector(output, input_alphabet);
    output << "\n";
    for (auto transition : transitions)
    {
        output << transition.first.first;
        output_vector(output, transition.first.second);
        output << " " << get<0>(transition.second);
        output_vector(output, get<1>(transition.second));
        string directions = get<2>(transition.second);
        for (int a = 0; a < num_tapes; ++a)
            output << " " << directions[a];
        output << "\n";
    }
}

vector<string> TuringMachine::parse_input(std::string input) const
{
    set<string> alphabet(input_alphabet.begin(), input_alphabet.end());
    size_t pos = 0;
    vector<string> res;
    while (pos < input.length())
    {
        size_t prev_pos = pos;
        if (!check_identifier(input, pos))
            return vector<string>();
        res.emplace_back(input.substr(prev_pos, pos - prev_pos));
        if (alphabet.find(res.back()) == alphabet.end())
            return vector<string>();
    }
    return res;
}