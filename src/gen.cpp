#include <algorithm>
#include <climits>
#include <iostream>
#include <random>
#include <string>

int main() {


    freopen("../data/gen1.in", "w", stdout);
    std::random_device rd;
    std::mt19937 rng(rd());

    const int n = std::uniform_int_distribution<int>(8000, 12000)(rng);
    const int blockSize =
        std::uniform_int_distribution<int>(256, 1024)(rng);
    const int q = std::uniform_int_distribution<int>(100000, 300000)(rng);

    std::uniform_int_distribution<int> operationDist(0, 7);
    std::uniform_int_distribution<int> idDist(0, 1100);

    const std::string operations[] = {
        "ALLOC", "ALLOC", "APPEND", "APPEND",
        "FREE", "LOOKUP", "REQUEST", "MEMORY"
    };

    std::cout << n << ' ' << blockSize << ' ' << q << '\n';

    long long tokenBudget = static_cast<long long>(n) * blockSize;

    for (int i = 0; i < q; ++i) {
        const std::string op = operations[operationDist(rng)];
        const std::string id = "R" + std::to_string(idDist(rng));

        if (op == "ALLOC" || op == "APPEND") {
            const long long upper = static_cast<long long>(
                std::min(1000000LL, tokenBudget)
            );
            const int count =
                std::uniform_int_distribution<int>(1, upper)(rng);

            tokenBudget -= count;
            std::cout << op << ' ' << id << ' ' << count << '\n';
        } else if (op == "FREE" || op == "REQUEST" || op == "LOOKUP") {
            std::cout << op << ' ' << id << '\n';
        } else {
            std::cout << op << '\n';
        }
    }

    return 0;
}