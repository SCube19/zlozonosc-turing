
# Example 2-tape Turing machine, which writes some letters from beyond the input alphabet

num-tapes: 2          
input-alphabet: a

# first, we move 2 times to the right
(start) a _ (once) _ b > >
(start) _ _ (once) _ b > >
(once)  a _ (twice) _ c > >
(once)  _ _ (twice) _ _ > >
(twice) _ _ (accept) a (THIS_IS_BEYOND_ALPHABET) < >