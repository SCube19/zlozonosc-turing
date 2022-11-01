
# Example 2-tape Turing machine, which falls from tape after some actions

num-tapes: 2          
input-alphabet: a

# first, we move 2 times to the right
(start) a _ (once) _ _ > >
(start) _ _ (once) _ _ > >
(once)  a _ (twice) _ _ > <
(once)  _ _ (twice) _ _ > <
(twice) a _ (back) a _ < >
(twice) _ _ (back) a _ < >

(back) _ _ (back) a _ < -

(back) _ a (accept) _ _ - - 