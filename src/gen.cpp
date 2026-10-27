#include <algorithm>
#include <climits>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

int main(int argc, char* argv[]) {

    char* turn = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if(std::string(argv[i]) == "--turn")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for --turn" << std::endl;
                return 1;
            }
            turn = argv[i + 1];
            ++i;
        }
    }
    std::string turn_str = "../data/gen";
    turn_str += turn ? std::string(turn) : "";
    turn_str += ".in";
    freopen(turn_str.c_str(), "w", stdout);
    std::random_device rd;
    const int seed = 12345;
    //std::mt19937 rng(rd());
    std::mt19937 rng(seed);

    int n = std::uniform_int_distribution<int>(7000000, 10000000)(rng);
    //const int n = 8;
    
    const int blockSize = (1<<(2*(1+std::stoi(std::string(turn))))); 
        //std::uniform_int_distribution<int>(256, 1024)(rng);
    n = n / blockSize;
    const int q = std::uniform_int_distribution<int>(100000, 100000)(rng);

    std::uniform_int_distribution<int> operationDist(0, 7);
    std::uniform_int_distribution<int> idDist(0, 150);

    const std::string operations[] = {
        "ALLOC", "ALLOC", "APPEND", "APPEND",
        "FREE", "LOOKUP", "REQUEST", "MEMORY"
    };

    std::cout << n << ' ' << blockSize << ' ' << q << '\n';

    long long tokenBudget = static_cast<long long>(n) * blockSize;
    //tokenBudget *= sqrt(tokenBudget);

    for (int i = 0; i < q; ++i) {
        const std::string op = operations[operationDist(rng)];
        const std::string id = "R" + std::to_string(idDist(rng));

        if (op == "ALLOC" || op == "APPEND") {
            const long long upper = static_cast<long long>(
                std::max(1LL,std::min((long long)(1.5 * n * blockSize / (q / 7)) , tokenBudget))
            );
            const int count =
                std::uniform_int_distribution<int>(1, upper)(rng);

            tokenBudget -= count;
            std::cout << op << ' ' << id << ' ' << count << '\n';
        } else if (op == "FREE" || op == "REQUEST" || op == "LOOKUP") {
            if (op == "FREE") {
                const long long upper = static_cast<long long>(
                std::max(1LL,(long long)(3 * n * blockSize / (q / 7)))
            );
            const int count =
                std::uniform_int_distribution<int>(1, upper)(rng);

            tokenBudget += count;
            }
            std::cout << op << ' ' << id << '\n';
        } else {
            std::cout << op << '\n';
        }
    }

    return 0;
}