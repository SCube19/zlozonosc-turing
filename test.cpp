#include <algorithm>
#include <iostream>

int main() {
        std::vector<int> x{5, 4};
        std::vector<int> y{1, 2, 3};
        std::ranges::copy(std::vector<int>{1, 2, 3}, std::back_inserter(x));
        std::ranges::copy(x, std::ostream_iterator<int>(std::cout, " "));
}

