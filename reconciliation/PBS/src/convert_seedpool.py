#!/usr/bin/env python3

if __name__ == "__main__":
    with open("./seedpool.txt", "r") as inf, open("./seedpool.h", "w") as ouf:
        ouf.write("#ifndef SEED_POOL_H_\n#define SEED_POOL_H_\n\n"
              "#include <vector>\n\nnamespace pbsutil {\n")
        ouf.write("std::vector<uint> loadSeedPool() {\n"
              "\treturn {")
        seeds = [line.strip() for line in inf]
        ouf.write(', '.join(seeds) + "};\n}\n}// namespace pbsutil\n#endif // SEED_POOL_H_\n")
