import random
keynum = 1100000
keypool = random.sample(range(1, 4294967295), keynum)
file = open("keypool.txt", "w")
for key in keypool:
    file.write(str(key) + "\n")
file.close()
seednum = 10000
seedpool = random.sample(range(0,4294967295), seednum)
file = open("seedpool.txt", "w")
for seed in seedpool:
    file.write(str(seed) + "\n")
file.close()
