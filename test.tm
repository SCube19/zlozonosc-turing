
# Example 2-tape Turing machine, recognizing the language of palindromes over {a,b}

# Text after "#" is a comment
# For information on possible names of states and letters check a comment in turing_machine.h

num-tapes: 2          # mendatory - number of tapes
input-alphabet: a b c  # mendatory - letters allowed to occur on input

# next, we have a list of transitions, in the format:
# <p> <a_1> ... <a_k> <q> <b_1> ... <b_k> <d_1> ... <d_k>
# where:
# * p and q are states before / after the transition
# * a_1, ..., a_k are letters under the head on tapes 1,...,k before the transition
# * b_1, ..., b_k - likewise, but after the transition
# * d_1, ..., d_k - directions in which each head is moved; can be <, >, -

# first, we copy the input to the second tape, but we shift it one cell right
(start) _ _ (accept) _ _ - -
(start) a _ (next) b _ - -
(start) b _ (next) c _ - -
(start) c _ (decide) c _ > -
(next) b _ (next) c _ - -
(next) c _ (decide) c _ > -
(decide) a _ (almost) b _ > -
(almost) _ _ (almost) a _ - -
(almost) a _ (accept) a _ - -

